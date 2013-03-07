//=====================================================================================//
//
// npc_zombie_marker
//
// Entidad encarga de crear zombis alrededor de si mismo.
// Usado por el Director para la creación de zombis.
//
//=====================================================================================//

#include "cbase.h"

#include "ai_basenpc.h"
//#include "ai_route.h"
//#include "ai_navigator.h"
//#include "ai_pathfinder.h"
//#include "ai_node.h"
//#include "ai_moveprobe.h"
#include "ai_behavior_passenger.h"

//#include "nav_mesh.h"
#include "director.h"

#include "monstermaker.h"
#include "director_zombie_maker.h"

#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_zombie_maker, CNPCZombieMaker );

BEGIN_DATADESC( CNPCZombieMaker )

	DEFINE_KEYFIELD( SpawnClassicZombie,	FIELD_BOOLEAN,	"SpawnClassicZombie" ),
	DEFINE_KEYFIELD( SpawnFastZombie,		FIELD_BOOLEAN,	"SpawnFastZombie" ),
	DEFINE_KEYFIELD( SpawnPoisonZombie,		FIELD_BOOLEAN,	"SpawnPoisonZombie" ),
	DEFINE_KEYFIELD( SpawnZombine,			FIELD_BOOLEAN,	"SpawnZombine" ),
	DEFINE_KEYFIELD( SpawnGrunt,			FIELD_BOOLEAN,	"SpawnGrunt" ),
	DEFINE_KEYFIELD( SpawnRadius,			FIELD_FLOAT,	"Radius" ),

	/* INPUTS */
	DEFINE_INPUTFUNC( FIELD_VOID, "Spawn",	InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",	InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",	InputToggle ),

	/* OUTPUTS */
	DEFINE_OUTPUT( OnNPCDead,		"OnNPCDead" ),
	DEFINE_OUTPUT( OnSpawnNPC,		"OnSpawnNPC" ),

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CNPCZombieMaker::CNPCZombieMaker()
{
	Childs			= 0;
	ChildsAlive		= 0;
	ChildsKilled	= 0;
	LastSpawn		= 0;

	Enabled			= true;
}

//=========================================================
// Aparecer
//=========================================================
void CNPCZombieMaker::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CNPCZombieMaker::Precache()
{
	BaseClass::Precache();

	if ( SpawnClassicZombie )
		UTIL_PrecacheOther("npc_zombie");

	if ( SpawnZombine)
		UTIL_PrecacheOther("npc_zombine");

	if ( SpawnFastZombie )
		UTIL_PrecacheOther("npc_fastzombie");

	if ( SpawnPoisonZombie )
		UTIL_PrecacheOther("npc_poisonzombie");

	if ( SpawnGrunt )
		UTIL_PrecacheOther("npc_grunt");
}

//=========================================================
// Activa InDirector
//=========================================================
void CNPCZombieMaker::Enable()
{
	Enabled = true;
}

//=========================================================
// Desactiva InDirector
//=========================================================
void CNPCZombieMaker::Disable()
{
	Enabled = false;
}

//=========================================================
// Verifica las condiciones y devuelve si es
// conveniente/posible crear un NPC en las coordenadas.
//=========================================================
bool CNPCZombieMaker::CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult)
{
	// Desactivado
	if ( !Enabled )
	{
		UTIL_RemoveImmediate(pNPC);
		return false;
	}

	Vector origin;
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	ConVarRef indirector_force_spawn_outview("indirector_force_spawn_outview");
	
	// Verificamos si es posible crear el NPC en el radio especificado.
	if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, indirector_force_spawn_outview.GetBool()) )
	{
		if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, false) )
		{
			DevWarning("[DIRECTOR ZOMBIE MAKER] No se encontro un lugar valido para crear un zombie. \r\n");

			UTIL_RemoveImmediate(pNPC);
			return false;
		}
	}

	// Crear en la misma altura que el spawn (Y así evitamos que se cree por debajo del suelo)
	origin.z = GetAbsOrigin().z;

	// Si esta activado la opción de forzar la creación fuera de la visibilidad del usuario.
	// Hay que asegurarnos de que el usuario no esta viendo el lugar de creación...
	if ( indirector_force_spawn_outview.GetBool() )
	{
		CInDirector *pDirector = GetDirector();

		if( !pDirector )
			return false;

		if ( pDirector->GetStatus() == 4 )
		{
			if ( pPlayer->FVisible(origin) )
			{
				DevWarning("[DIRECTOR ZOMBIE MAKER] El lugar de creacion estaba en el campo de vision. \r\n");
				UTIL_RemoveImmediate(pNPC);

				return false;
			}
		}
		else
		{
			if ( pPlayer->FInViewCone(origin) || pPlayer->FVisible(origin) )
			{
				Warning("[DIRECTOR ZOMBIE MAKER] El lugar de creacion estaba en el campo de vision. \r\n");
				UTIL_RemoveImmediate(pNPC);

				return false;
			}
		}
	}

	*pResult = origin;
}

//=========================================================
// Verificaciones después de crear al NPC.
//=========================================================
void CNPCZombieMaker::ChildPostSpawn(CAI_BaseNPC *pNPC)
{
	bool bStuck = true;

	while ( bStuck )
	{
		trace_t tr;
		UTIL_TraceHull( pNPC->GetAbsOrigin(), pNPC->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_NPCSOLID, pNPC, COLLISION_GROUP_NONE, &tr );

		if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			// Nos hemos atorado en un objeto con fisicas.
			if (FClassnameIs(tr.m_pEnt, "prop_physics"))
			{
				// Lo ajustamos como "No solido" para que el bucle lo ignore.
				tr.m_pEnt->AddSolidFlags(FSOLID_NOT_SOLID );
				// Removemos el objeto.
				UTIL_RemoveImmediate(tr.m_pEnt);
				continue;
			}

			// Nos hemos atorado con una pared o algo del mundo.
			if (tr.m_pEnt->IsWorld())
			{
				// No... no podemos eliminar una pared...
				// Removemos el NPC para evitar eventos sobrenaturales.
				UTIL_RemoveImmediate(pNPC);
				continue;
			}
		}

		bStuck = false;
	}
}

//=========================================================
// Crea un Zombi.
//=========================================================
CAI_BaseNPC *CNPCZombieMaker::MakeNPC(bool Horde, bool disclosePlayer)
{
	if( !Enabled )
		return NULL;

	// Seleccionamos una clase de zombi para crear.
	CAI_BaseNPC *pZombie = (CAI_BaseNPC *)CreateEntityByName(SelectRandomZombie());

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pZombie )
	{
		Warning("[DIRECTOR ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un zombie. \r\n");
		return NULL;
	}
	

	/*
		TODO Con esto susupone que tendríamos la posibilidad de que InDirector pueda crear zombis en las areas
		de navegación computadas por el motor. O en cristiano... crear zombis sin tener que poner tantos "npc_zombie_maker"
		claro, no funciona.

		[NOTA] Esto hace que los zombis se creen justo atras del usuario o en una zona computada cercana en caso de no estar en una.

	Vector center, center_portal, delta;

	NavDirType dir;
	float hwidth;

	CNavArea *pArea = TheNavMesh->GetNearestNavArea(pPlayer->GetAbsOrigin());

	center	= pArea->GetCenter();
	dir		= pArea->ComputeDirection(&center);

	pArea->ComputePortal(pArea, dir, &center_portal, &hwidth);
	pArea->ComputeClosestPointInPortal(pArea, dir, pArea->GetCenter(), &origin);

	origin.z = pArea->GetZ(origin);
	*/

	Vector origin;

	if ( !CanMakeNPC(pZombie, &origin) )
		return NULL;

	// Lugar de creación.
	pZombie->SetAbsOrigin(origin);

	// Nombre del zombie.
	// [¡NO CAMBIAR!] Es utilizado por otras entidades para referirse a los zombis creados por el director.
	pZombie->SetName(MAKE_STRING("director_zombie"));

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pZombie->SetAbsAngles(angles);

	// Nombre del esquadron.
	pZombie->SetSquadName(MAKE_STRING("DIRECTOR_ZOMBIES"));
	// Tiene que caer al suelo.
	pZombie->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	// Su cuerpo tiene que desaparecer al morir.
	pZombie->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	// Creamos al zombi, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pZombie);
	pZombie->SetOwnerEntity(this);
	DispatchActivate(pZombie);

	// Es un zombi para la horda ¡woot!
	if ( Horde )
	{
		// Más salud.
		pZombie->SetMaxHealth(pZombie->GetMaxHealth() + 20);
		pZombie->SetHealth(pZombie->GetMaxHealth());

		// Más rápido.
		pZombie->SetAddAccel(40);
	}

	// Debe conocer la ubicación del jugador (Su enemigo)
	if ( disclosePlayer )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		pZombie->SetEnemy(pPlayer);
		pZombie->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
	}

	Childs++;
	ChildsAlive++;
	LastSpawn = gpGlobals->curtime;

	OnSpawnNPC.FireOutput(pZombie, this);
	return pZombie;
}

//=========================================================
// Crea un NPC Grunt.
//=========================================================
CAI_BaseNPC *CNPCZombieMaker::MakeGrunt()
{
	if( !Enabled )
		return NULL;

	CAI_BaseNPC *pGrunt = (CAI_BaseNPC *)CreateEntityByName("npc_grunt");

	// Ocurrio algún problema.
	if ( !pGrunt )
	{
		Warning("[DIRECTOR ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un grunt. \r\n");
		return NULL;
	}

	Vector origin;

	if ( !CanMakeNPC(pGrunt, &origin) )
		return NULL;

	// Lugar de creación.
	pGrunt->SetAbsOrigin(origin);

	// Nombre del Grunt.
	// [¡NO CAMBIAR!] Es utilizado por otras entidades para referirse a los zombis creados por el director.
	pGrunt->SetName(MAKE_STRING("director_grunt"));

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pGrunt->SetAbsAngles(angles);

	// Tiene que caer al suelo.
	pGrunt->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	// Creamos al grunt, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pGrunt);
	pGrunt->SetOwnerEntity(this);
	DispatchActivate(pGrunt);

	// Debe conocer la ubicación del jugador (Su enemigo)
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	pGrunt->SetEnemy(pPlayer);
	pGrunt->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());

	// Preparaciones después de la creación.
	ChildPostSpawn(pGrunt);

	return pGrunt;
}

//=========================================================
// Ha muerto un NPC que he creado.
//=========================================================
void CNPCZombieMaker::DeathNotice(CBaseEntity *pVictim)
{
	ChildsAlive--;
	ChildsKilled++;

	OnNPCDead.FireOutput(pVictim, this);
}

//=========================================================
// Selecciona una clase de zombi.
//=========================================================
const char *CNPCZombieMaker::SelectRandomZombie()
{
	int pRandom = random->RandomInt(1, 7);

	if ( pRandom == 1 )
		return (SpawnClassicZombie) ? "npc_zombie" : SelectRandomZombie();

	if ( pRandom == 2 )
		return (SpawnZombine) ? "npc_zombine" : SelectRandomZombie();

	if ( pRandom == 3 )
		return (SpawnFastZombie) ? "npc_fastzombie" : SelectRandomZombie();

	if ( pRandom == 4 )
		return (SpawnPoisonZombie) ? "npc_poisonzombie" : SelectRandomZombie();

	return SelectRandomZombie();
}

int CNPCZombieMaker::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char message[512];
		Q_snprintf(message, sizeof(message), 
			"Zombis creados: %i", 
		Childs);
		EntityText(text_offset++, message, 0);
	}

	return text_offset;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS)
//=========================================================
//=========================================================

void CNPCZombieMaker::InputSpawn(inputdata_t &inputdata)
{
	MakeNPC();
}

void CNPCZombieMaker::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

void CNPCZombieMaker::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

void CNPCZombieMaker::InputToggle(inputdata_t &inputdata)
{
	if( Enabled )
		Disable();
	else
		Enable();
}