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
#include "ai_behavior_passenger.h"

#include "director.h"
#include "director_zombie_spawn.h"

#include "env_sound.h"
#include "npc_grunt.h";

#include "tier0/memdbgon.h"

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

#define SCHED_ZOMBIE_WANDER_ANGRILY 101
#define SCHED_ZOMBIE_WANDER_MEDIUM	95

//=========================================================
//=========================================================
// DIRECTOR ZOMBIE MAKER
//=========================================================
//=========================================================

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_zombie_maker, CDirectorZombieSpawn );

BEGIN_DATADESC( CDirectorZombieSpawn )

	DEFINE_KEYFIELD( Disabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_KEYFIELD( SpawnClassicZombie,	FIELD_BOOLEAN,	"SpawnClassicZombie" ),
	DEFINE_KEYFIELD( SpawnFastZombie,		FIELD_BOOLEAN,	"SpawnFastZombie" ),
	DEFINE_KEYFIELD( SpawnPoisonZombie,		FIELD_BOOLEAN,	"SpawnPoisonZombie" ),
	DEFINE_KEYFIELD( SpawnZombine,			FIELD_BOOLEAN,	"SpawnZombine" ),
	DEFINE_KEYFIELD( SpawnGrunt,			FIELD_BOOLEAN,	"SpawnGrunt" ),
	DEFINE_KEYFIELD( SpawnRadius,			FIELD_FLOAT,	"Radius" ),

	/* INPUTS */
	DEFINE_INPUTFUNC( FIELD_VOID,		"Spawn",					InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SpawnCount",				InputSpawnCount ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Enable",					InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Disable",					InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Toggle",					InputToggle ),

	/* OUTPUTS */
	DEFINE_OUTPUT( OnChildDead,		"OnChildDead" ), // @TODO
	DEFINE_OUTPUT( OnSpawnNPC,		"OnSpawnNPC" ),

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CDirectorZombieSpawn::CDirectorZombieSpawn()
{
	Childs			= 0;
	ChildsAlive		= 0;
	ChildsKilled	= 0;
	LastSpawn		= 0;
}

//=========================================================
// Aparecer
//=========================================================
void CDirectorZombieSpawn::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirectorZombieSpawn::Precache()
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
void CDirectorZombieSpawn::Enable()
{
	Disabled = false;
}

//=========================================================
// Desactiva InDirector
//=========================================================
void CDirectorZombieSpawn::Disable()
{
	Disabled = true;
}

//=========================================================
// Verifica las condiciones y devuelve si es
// conveniente/posible crear un NPC en las coordenadas.
//=========================================================
bool CDirectorZombieSpawn::CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult)
{
	// Desactivado
	if ( Disabled || g_pGameRules->IsMultiplayer() )
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

		if ( pDirector->GetStatus() == pDirector->CLIMAX )
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
	return true;
}

//=========================================================
// Verificaciones después de crear al NPC.
//=========================================================
void CDirectorZombieSpawn::ChildPostSpawn(CAI_BaseNPC *pNPC)
{
	bool bStuck = true;

	while ( bStuck )
	{
		trace_t tr;
		UTIL_TraceHull( pNPC->GetAbsOrigin(), pNPC->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_NPCSOLID, pNPC, COLLISION_GROUP_NONE, &tr );

		if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			// Nos hemos atorado en un objeto con fisicas.
			if ( FClassnameIs(tr.m_pEnt, "prop_physics") )
			{
				// Lo ajustamos como "No solido" para que el bucle lo ignore.
				tr.m_pEnt->AddSolidFlags(FSOLID_NOT_SOLID);
				// Removemos el objeto.
				UTIL_RemoveImmediate(tr.m_pEnt);
				continue;
			}

			// Nos hemos atorado con una pared o algo del mundo.
			if ( tr.m_pEnt->IsWorld() )
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
CAI_BaseNPC *CDirectorZombieSpawn::MakeNPC(bool Horde, bool disclosePlayer)
{
	if ( Disabled || g_pGameRules->IsMultiplayer() )
		return NULL;

	// Seleccionamos una clase de zombi para crear.
	const char *pZombieClass	= SelectRandomZombie();

	// Creamos al zombi.
	CAI_BaseNPC *pZombie		= (CAI_BaseNPC *)CreateEntityByName(pZombieClass);

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pZombie )
	{
		Warning("[DIRECTOR ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un zombie. \r\n");
		return NULL;
	}

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
	//pZombie->SetSquadName(MAKE_STRING("DIRECTOR_ZOMBIES"));
	// Tiene que caer al suelo.
	pZombie->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	// Su cuerpo tiene que desaparecer al morir.
	pZombie->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	// Creamos al zombi, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pZombie);
	pZombie->SetOwnerEntity(this);
	DispatchActivate(pZombie);

	// Skin al azar.
	pZombie->m_nSkin = random->RandomInt(1, 4);

	// Es un zombi para la horda ¡woot!
	if ( Horde )
	{
		int MoreHealth = 3;

		// Aumentamos la salud dependiendo del nivel de dificultad.
		if ( GameRules()->IsSkillLevel(SKILL_MEDIUM) ) 
			MoreHealth = 5;

		if ( GameRules()->IsSkillLevel(SKILL_HARD) )
			MoreHealth = 8;

		// Más salud.
		pZombie->SetMaxHealth(pZombie->GetMaxHealth() + MoreHealth);
		pZombie->SetHealth(pZombie->GetMaxHealth());

		// Más rápido.
		pZombie->SetAddAccel(40);

		// No colisionan con otros NPC's. (Zombis)
		pZombie->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);
	}

	// Debe conocer la ubicación del jugador (Su enemigo)
	if ( disclosePlayer )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		//pZombie->SetEnemy(pPlayer);
		pZombie->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
	}

	Childs++;
	ChildsAlive++;
	LastSpawn = gpGlobals->curtime;

	OnSpawnNPC.FireOutput(pZombie, this);
	return pZombie;
}

//=========================================================
// Crea un Zombi sin colisiones.
//=========================================================
CAI_BaseNPC *CDirectorZombieSpawn::MakeNoCollisionNPC(bool Horde, bool disclosePlayer)
{
	if( Disabled )
		return NULL;

	// Seleccionamos una clase de zombi para crear.
	const char *pZombieClass	= SelectRandomZombie();

	// Creamos al zombi.
	CAI_BaseNPC *pZombie		= (CAI_BaseNPC *)CreateEntityByName(pZombieClass);

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pZombie )
	{
		Warning("[DIRECTOR ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un zombie. \r\n");
		return NULL;
	}

	// Lugar de creación.
	pZombie->SetAbsOrigin(GetAbsOrigin());

	// Nombre del zombie.
	// [¡NO CAMBIAR!] Es utilizado por otras entidades para referirse a los zombis creados por el director.
	pZombie->SetName(MAKE_STRING("director_zombie"));

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pZombie->SetAbsAngles(angles);

	// Nombre del esquadron.
	//pZombie->SetSquadName(MAKE_STRING("DIRECTOR_ZOMBIES"));
	// Tiene que caer al suelo.
	pZombie->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	// Su cuerpo tiene que desaparecer al morir.
	pZombie->AddSpawnFlags(SF_NPC_FADE_CORPSE);
	// No colisionan con otros NPC's. (Zombis)
	pZombie->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);

	// Creamos al zombi, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pZombie);
	pZombie->SetOwnerEntity(this);
	DispatchActivate(pZombie);

	// Es un zombi para la horda ¡woot!
	if ( Horde )
	{
		int MoreHealth = 3;

		// Aumentamos la salud dependiendo del nivel de dificultad.
		if ( GameRules()->IsSkillLevel(SKILL_MEDIUM) ) 
			MoreHealth = 5;

		if ( GameRules()->IsSkillLevel(SKILL_HARD) )
			MoreHealth = 8;

		// Más salud.
		pZombie->SetMaxHealth(pZombie->GetMaxHealth() + MoreHealth);
		pZombie->SetHealth(pZombie->GetMaxHealth());

		// Más rápido.
		pZombie->SetAddAccel(50);
	}

	// Debe conocer la ubicación del jugador (Su enemigo)
	if ( disclosePlayer )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		//pZombie->SetEnemy(pPlayer);
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
CAI_BaseNPC *CDirectorZombieSpawn::MakeGrunt()
{
	if( Disabled )
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
void CDirectorZombieSpawn::DeathNotice(CBaseEntity *pVictim)
{
	ChildsAlive--;
	ChildsKilled++;

	CInDirector *pDirector = GetDirector();

	if ( pDirector )
	{
		// ¡Han matado a un Zombie!
		if ( pVictim->GetEntityName() == MAKE_STRING("director_zombie") )
			pDirector->ZombieKilled();
			
		// ¡Han matado a un Grunt!
		if ( pVictim->GetEntityName() == MAKE_STRING("director_grunt") )
			pDirector->GruntKilled();
	}

	OnChildDead.FireOutput(pVictim, this);
}

//=========================================================
// Selecciona una clase de zombi.
//=========================================================
const char *CDirectorZombieSpawn::SelectRandomZombie()
{
	int pRandom = random->RandomInt(1, 5);

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

int CDirectorZombieSpawn::DrawDebugTextOverlays()
{
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char message[512];
		Q_snprintf(message, sizeof(message), 
			"Zombis creados: %i", 
		Childs);

		if ( pPlayer )
		{
			// Calculamos la distancia del creador al jugador.
			Vector distToMaker	= GetAbsOrigin() - pPlayer->GetAbsOrigin();
			float dist			= VectorNormalize(distToMaker);

			Q_snprintf(message, sizeof(message), 
				"Distancia: %i", 
			dist);
		}

		EntityText(text_offset++, message, 0);
	}

	return text_offset;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS)
//=========================================================
//=========================================================

void CDirectorZombieSpawn::InputSpawn(inputdata_t &inputdata)
{
	MakeNPC();
}

void CDirectorZombieSpawn::InputSpawnCount(inputdata_t &inputdata)
{
	int i;
	CAI_BaseNPC *pZombie = NULL;

	ConVarRef indirector_force_spawn_outview("indirector_force_spawn_outview");
	int oldValue = indirector_force_spawn_outview.GetInt();

	indirector_force_spawn_outview.SetValue(0);

	for ( i = 0; i <= inputdata.value.Int(); i = i + 1)
	{
		pZombie = MakeNoCollisionNPC();

		if ( !pZombie )
		{
			DevMsg("[DIRECTOR ZOMBIE MAKER] No se ha podido crear el zombi %i \r\n", i);
			continue;
		}
	}

	indirector_force_spawn_outview.SetValue(oldValue);
}

void CDirectorZombieSpawn::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

void CDirectorZombieSpawn::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

void CDirectorZombieSpawn::InputToggle(inputdata_t &inputdata)
{
	if( Disabled )
		Enable();
	else
		Disable();
}






//=========================================================
//=========================================================
// SURVIVAL ZOMBIE MAKER
//=========================================================
//=========================================================

ConVar sv_spawn_zombies("sv_spawn_zombies", "1", FCVAR_NOTIFY | FCVAR_REPLICATED);

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( survival_zombie_spawn, CSurvivalZombieSpawn );

BEGIN_DATADESC( CSurvivalZombieSpawn )

	DEFINE_KEYFIELD( Disabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( SpawnRadius,	FIELD_FLOAT,	"Radius" ),

	/* INPUTS */
	DEFINE_INPUTFUNC( FIELD_VOID,		"Spawn",					InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SpawnCount",				InputSpawnCount ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Enable",					InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Disable",					InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Toggle",					InputToggle ),

	/* OUTPUTS */
	DEFINE_OUTPUT( OnZombieDead,		"OnZombieDead" ),
	DEFINE_OUTPUT( OnSpawnZombie,		"OnSpawnZombie" ),

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CSurvivalZombieSpawn::CSurvivalZombieSpawn()
{
	Childs			= 0;
	ChildsAlive		= 0;
	ChildsKilled	= 0;

	LastSpawn		= 0;
	pGruntMusic		= NULL;

	SetNextThink(gpGlobals->curtime);
}

void CSurvivalZombieSpawn::Think()
{
	if ( ChildsAlive < 5 )
		MakeNPC();

	SetNextThink(gpGlobals->curtime + 10);
}

//=========================================================
// Aparecer
//=========================================================
void CSurvivalZombieSpawn::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CSurvivalZombieSpawn::Precache()
{
	BaseClass::Precache();

	if ( HasSpawnFlags(SF_SPAWN_CLASSIC) )
		UTIL_PrecacheOther("npc_zombie");

	if ( HasSpawnFlags(SF_SPAWN_ZOMBINE) )
		UTIL_PrecacheOther("npc_zombine");

	if ( HasSpawnFlags(SF_SPAWN_FAST) )
		UTIL_PrecacheOther("npc_fastzombie");

	if ( HasSpawnFlags(SF_SPAWN_POISON) )
		UTIL_PrecacheOther("npc_poisonzombie");

	if ( HasSpawnFlags(SF_SPAWN_GRUNT) )
		UTIL_PrecacheOther("npc_grunt");
}

//=========================================================
// Activa InDirector
//=========================================================
void CSurvivalZombieSpawn::Enable()
{
	Disabled = false;
}

//=========================================================
// Desactiva InDirector
//=========================================================
void CSurvivalZombieSpawn::Disable()
{
	Disabled = true;
}

//=========================================================
// Verifica las condiciones y devuelve si es
// conveniente/posible crear un NPC en las coordenadas.
//=========================================================
bool CSurvivalZombieSpawn::CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult)
{
	// Desactivado
	if ( Disabled || !sv_spawn_zombies.GetBool() )
	{
		UTIL_RemoveImmediate(pNPC);
		return false;
	}

	Vector origin;
	
	// Verificamos si es posible crear el NPC en el radio especificado.
	if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, true) )
	{
		if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, false) )
		{
			DevWarning("[SURVIVAL ZOMBIE MAKER] No se encontro un lugar valido para crear un zombie. \r\n");

			UTIL_RemoveImmediate(pNPC);
			return false;
		}
	}

	// Crear en la misma altura que el spawn (Y así evitamos que se cree por debajo del suelo)
	origin.z = GetAbsOrigin().z;

	*pResult = origin;
	return true;
}

//=========================================================
// Crea un Zombi.
//=========================================================
CAI_BaseNPC *CSurvivalZombieSpawn::MakeNPC()
{
	// Desactivado
	if ( Disabled || !sv_spawn_zombies.GetBool() )
		return NULL;

	// Seleccionamos una clase de zombi para crear.
	const char *pZombieClass	= SelectRandomZombie();

	// El elegido es un Grunt.
	if ( pZombieClass == "npc_grunt" )
		return MakeGrunt();

	// Creamos al zombi.
	CAI_BaseNPC *pZombie = (CAI_BaseNPC *)CreateEntityByName(pZombieClass);

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pZombie )
	{
		Warning("[SURVIVAL ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un zombie. \r\n");
		return NULL;
	}

	Vector origin;

	if ( !CanMakeNPC(pZombie, &origin) )
		return NULL;

	// Lugar de creación.
	pZombie->SetAbsOrigin(origin);

	// Nombre del zombie.
	// [¡NO CAMBIAR!] Es utilizado por otras entidades para referirse a los zombis creados por esta entidad.
	pZombie->SetName(MAKE_STRING("survival_zombie"));

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pZombie->SetAbsAngles(angles);

	// Tiene que caer al suelo.
	pZombie->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	// Su cuerpo tiene que desaparecer al morir.
	pZombie->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	// No colisionan con otros NPC's. (Zombis)
	if ( random->RandomInt(1, 3) == 2 )
		pZombie->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);

	// Creamos al zombi, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pZombie);
	pZombie->SetOwnerEntity(this);
	DispatchActivate(pZombie);

	// Skin al azar.
	pZombie->m_nSkin = random->RandomInt(1, 4);

	Childs++;
	ChildsAlive++;
	LastSpawn = gpGlobals->curtime;

	OnSpawnZombie.FireOutput(pZombie, this);
	return pZombie;
}

//=========================================================
// Crea un NPC Grunt.
//=========================================================
CAI_BaseNPC *CSurvivalZombieSpawn::MakeGrunt()
{
	// Desactivado
	if ( Disabled || !sv_spawn_zombies.GetBool() )
		return NULL;

	CAI_BaseNPC *pGrunt = (CAI_BaseNPC *)CreateEntityByName("npc_grunt");

	// Ocurrio algún problema.
	if ( !pGrunt )
	{
		Warning("[SURVIVAL ZOMBIE MAKER] Ha ocurrido un problema al intentar crear un grunt. \r\n");
		return NULL;
	}

	Vector origin;

	if ( !CanMakeNPC(pGrunt, &origin) )
		return NULL;

	// Lugar de creación.
	pGrunt->SetAbsOrigin(origin);

	// Nombre del Grunt.
	// [¡NO CAMBIAR!] Es utilizado por otras entidades para referirse a los zombis creados por esta entidad.
	pGrunt->SetName(MAKE_STRING("survival_grunt"));

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

	// Ahora creamos la música.
	pGruntMusic = (CEnvSound *)CreateEntityByName("env_sound");
	pGruntMusic->SetSourceEntityName(MAKE_STRING("survival_grunt"));
	pGruntMusic->SetSoundName(MAKE_STRING("NPC_Grunt.BackgroundMusic"));
	pGruntMusic->SetRadius(8000.0);
	
	pGruntMusic->SetAbsOrigin(origin);
	DispatchSpawn(pGruntMusic);
	pGruntMusic->SetOwnerEntity(this);
	DispatchActivate(pGruntMusic);

	pGruntMusic->SetPitch(100);
	pGruntMusic->SetVolume(1);
	pGruntMusic->PlayManual(1, 100);

	return pGrunt;
}

//=========================================================
// Ha muerto un NPC que he creado.
//=========================================================
void CSurvivalZombieSpawn::DeathNotice(CBaseEntity *pVictim)
{
	ChildsAlive--;
	ChildsKilled++;

	if ( pVictim->GetClassname() == "npc_grunt" && pGruntMusic )
	{
		pGruntMusic->FadeOut(2);

		//UTIL_RemoveImmediate(pGruntMusic);
		pGruntMusic = NULL;
	}

	OnZombieDead.FireOutput(pVictim, this);
}

//=========================================================
// Selecciona una clase de zombi.
//=========================================================
const char *CSurvivalZombieSpawn::SelectRandomZombie()
{
	int pRandom = random->RandomInt(1, 6);

	if ( pRandom == 1 && HasSpawnFlags(SF_SPAWN_CLASSIC) )
		return "npc_zombie";

	if ( pRandom == 2 && HasSpawnFlags(SF_SPAWN_ZOMBINE) )
		return "npc_zombine";

	if ( pRandom == 3 && HasSpawnFlags(SF_SPAWN_FAST) )
		return "npc_fastzombie";

	if ( pRandom == 4 && HasSpawnFlags(SF_SPAWN_POISON) )
		return "npc_poisonzombie";

	if ( pRandom == 5 && random->RandomInt(1, 30) > 28 && HasSpawnFlags(SF_SPAWN_GRUNT) && CountGrunts() <= 0 )
		return "npc_grunt";

	return SelectRandomZombie();
}

int CSurvivalZombieSpawn::CountGrunts()
{
	int GruntsAlive		= 0;
	CNPC_Grunt *pGrunt	= NULL;

	do
	{
		pGrunt = (CNPC_Grunt *)gEntList.FindEntityByClassname(pGrunt, "npc_grunt");

		if ( pGrunt )
			GruntsAlive++;

	} while(pGrunt);

	return GruntsAlive;
}

int CSurvivalZombieSpawn::DrawDebugTextOverlays()
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

void CSurvivalZombieSpawn::InputSpawn(inputdata_t &inputdata)
{
	MakeNPC();
}

void CSurvivalZombieSpawn::InputSpawnCount(inputdata_t &inputdata)
{
	int i;
	CAI_BaseNPC *pZombie = NULL;

	for ( i = 0; i <= inputdata.value.Int(); i = i + 1)
	{
		pZombie = MakeNPC();

		if ( !pZombie )
		{
			DevMsg("[SURVIVAL ZOMBIE MAKER] No se ha podido crear el zombi %i \r\n", i);
			continue;
		}
	}
}

void CSurvivalZombieSpawn::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

void CSurvivalZombieSpawn::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

void CSurvivalZombieSpawn::InputToggle(inputdata_t &inputdata)
{
	if( Disabled )
		Enable();
	else
		Disable();
}