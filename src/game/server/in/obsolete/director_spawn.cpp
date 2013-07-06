//=====================================================================================//
//
// director_spawn
//
// Entidad encargada de crear NPC's alrededor de si mismo.
// Usado por el Director para la creación de NPC's.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"

#include "ai_basenpc.h"
#include "ai_behavior_passenger.h"

#include "director.h"
#include "director_spawn.h"

#include "npc_grunt.h"

#include "env_sound.h"
#include "TemplateEntities.h"
#include "mapentities.h"

#include "in_gamerules.h"
#include "in_player.h"
#include "in_utils.h"

#include "tier0/memdbgon.h"

/*
	@TODO Con esto susupone que tendríamos la posibilidad de que InDirector pueda crear zombis en las areas
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

//=========================================================
//=========================================================
// DIRECTOR SPAWN
//=========================================================
//=========================================================

//=========================================================
// Configuración
//=========================================================

// Nombres para los NPC's.
// [NO CAMBIAR] Es utilizado por otras entidades para referirse a los NPC's creados por el director.
#define CHILD_NAME	"director_child"
#define BOSS_NAME	"director_boss"

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_spawn, CDirectorSpawn );

BEGIN_DATADESC( CDirectorSpawn )

	DEFINE_FIELD( LastSpawn,	FIELD_FLOAT ),

	DEFINE_FIELD( SpawnRadius,	FIELD_FLOAT ),
	DEFINE_FIELD( Childs,		FIELD_INTEGER ),
	DEFINE_FIELD( ChildsAlive,	FIELD_INTEGER ),
	DEFINE_FIELD( ChildsKilled,	FIELD_INTEGER ),

	DEFINE_KEYFIELD( Disabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	/* NPC'S */
	DEFINE_KEYFIELD( iNpcs[0],	FIELD_STRING,	"cnpc1" ),
	DEFINE_KEYFIELD( iNpcs[1],	FIELD_STRING,	"cnpc2" ),
	DEFINE_KEYFIELD( iNpcs[2],	FIELD_STRING,	"cnpc3" ),
	DEFINE_KEYFIELD( iNpcs[3],	FIELD_STRING,	"cnpc4" ),
	DEFINE_KEYFIELD( iNpcs[4],	FIELD_STRING,	"cnpc5" ),
	DEFINE_KEYFIELD( iNpcs[5],	FIELD_STRING,	"cnpc6" ),
	DEFINE_KEYFIELD( iNpcs[6],	FIELD_STRING,	"cnpc7" ),
	DEFINE_KEYFIELD( iNpcs[7],	FIELD_STRING,	"cnpc8" ),

	DEFINE_KEYFIELD( iBoss[0],	FIELD_STRING,	"cboss1" ),
	DEFINE_KEYFIELD( iBoss[1],	FIELD_STRING,	"cboss2" ),
	DEFINE_KEYFIELD( iBoss[2],	FIELD_STRING,	"cboss3" ),

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
CDirectorSpawn::CDirectorSpawn()
{
	Childs			= 0;
	ChildsAlive		= 0;
	ChildsKilled	= 0;
	LastSpawn		= 0;
}

//=========================================================
// Creación
//=========================================================
void CDirectorSpawn::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirectorSpawn::Precache()
{
	BaseClass::Precache();

	// NPC's
	int pNpcs = ARRAYSIZE(iNpcs) - 1;
	for ( int i = 0; i < pNpcs; ++i )
	{
		if ( iNpcs[i] == NULL_STRING )
			continue;

		// Buscamos si hay una plantilla con este nombre.
		string_t NpcData = Templates_FindByTargetName(STRING(iNpcs[i]));
			
		// No, se trata de una clase.
		if ( NpcData == NULL_STRING )
			UTIL_PrecacheOther(STRING(iNpcs[i]));
		else
		{
			// Guardamos en caché la plantilla.
			CBaseEntity *pEntity = NULL;
			MapEntity_ParseEntity(pEntity, STRING(NpcData), NULL);

			if ( pEntity != NULL )
			{
				pEntity->Precache();
				UTIL_RemoveImmediate(pEntity);
			}
		}
	}

	// JEFES
	int pBoss = ARRAYSIZE(iBoss) - 1;
	for ( int i = 0; i < pBoss; ++i )
	{
		if ( iBoss[i] == NULL_STRING )
			continue;

		// Buscamos si hay una plantilla con este nombre.
		string_t NpcData = Templates_FindByTargetName(STRING(iBoss[i]));
			
		// No, se trata de una clase.
		if ( NpcData == NULL_STRING )
			UTIL_PrecacheOther(STRING(iBoss[i]));
		else
		{
			// Guardamos en caché la plantilla.
			CBaseEntity *pEntity = NULL;
			MapEntity_ParseEntity(pEntity, STRING(NpcData), NULL);

			if ( pEntity != NULL )
			{
				pEntity->Precache();
				UTIL_RemoveImmediate(pEntity);
			}
		}
	}
}

//=========================================================
// Activa InDirector
//=========================================================
void CDirectorSpawn::Enable()
{
	Disabled = false;
}

//=========================================================
// Desactiva InDirector
//=========================================================
void CDirectorSpawn::Disable()
{
	Disabled = true;
}

//=========================================================
// Verifica las condiciones y devuelve si es
// conveniente/posible crear un NPC en las coordenadas.
//=========================================================
bool CDirectorSpawn::CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult)
{
	// Desactivado
	// Esta entidad no funciona en Multiplayer.
	if ( Disabled )
	{
		UTIL_RemoveImmediate(pNPC);
		return false;
	}

	Vector origin;
	ConVarRef director_force_spawn_outview("director_force_spawn_outview");
	
	// Verificamos si es posible crear el NPC en el radio especificado.
	if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, director_force_spawn_outview.GetBool()) )
	{
		if ( !CAI_BaseNPC::FindSpotForNPCInRadius(&origin, GetAbsOrigin(), pNPC, SpawnRadius, false) )
		{
			DevWarning("[DIRECTOR SPAWN] No se encontro un lugar valido para crear un NPC. \r\n");

			UTIL_RemoveImmediate(pNPC);
			return false;
		}
	}

	// Crear en la misma altura que el spawn (Y así evitamos que se cree por debajo del suelo)
	origin.z = GetAbsOrigin().z;

	// Si esta activado la opción de forzar la creación fuera de la visibilidad del usuario.
	// Hay que asegurarnos de que el usuario no esta viendo el lugar de creación...
	if ( director_force_spawn_outview.GetBool() )
	{
		CDirector *pDirector = GetDirector();

		if ( !pDirector )
			return false;

		// En Climax con que no lo vea directamente es suficiente...
		//if ( pDirector->GetStatus() == CLIMAX )
		//{
			if ( UTIL_IsPlayersVisible(origin) )
			{
				DevWarning("[DIRECTOR SPAWN] El lugar de creacion estaba en el campo de vision. \r\n");
				UTIL_RemoveImmediate(pNPC);

				return false;
			}
		//}

		// En cualquier otro modo no debe ser visible ni estar en el cono de visibilidad del jugador.
		/*else
		{
			if ( UTIL_IsPlayersVisibleCone(origin) )
			{
				Warning("[DIRECTOR SPAWN] El lugar de creacion estaba en el campo de vision. \r\n");
				UTIL_RemoveImmediate(pNPC);

				return false;
			}
		}*/
	}

	*pResult = origin;
	return true;
}

//=========================================================
// Verificaciones después de crear al NPC.
//=========================================================
bool CDirectorSpawn::PostSpawn(CAI_BaseNPC *pNPC)
{
	bool bStuck = true;

	while ( bStuck )
	{
		trace_t tr;
		UTIL_TraceHull(pNPC->GetAbsOrigin(), pNPC->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_NPCSOLID, pNPC, COLLISION_GROUP_NONE, &tr);

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
				return false;
			}
		}

		bStuck = false;
	}

	return true;
}

//=========================================================
// Añade salud del personaje.
//=========================================================
void CDirectorSpawn::AddHealth(CAI_BaseNPC *pNPC)
{
	// Aumentamos la salud dependiendo del nivel de dificultad.
	int MoreHealth = 3;

	// Normal: 5 más de salud.
	if ( GameRules()->IsSkillLevel(SKILL_MEDIUM) ) 
		MoreHealth = 5;

	// Dificil: 8 más de salud.
	if ( GameRules()->IsSkillLevel(SKILL_HARD) )
		MoreHealth = 8;

	// Establecemos la nueva salud.
	pNPC->SetMaxHealth(pNPC->GetMaxHealth() + MoreHealth);
	pNPC->SetHealth(pNPC->GetMaxHealth());
}

//=========================================================
// Devuelve la creación de un NPC desde su clase/plantilla
//=========================================================
CAI_BaseNPC *CDirectorSpawn::VerifyClass(const char *pClass)
{
	// Buscamos si hay una plantilla con este nombre.
	string_t NpcData		= Templates_FindByTargetName(pClass);
	CAI_BaseNPC *pNPC		= NULL;

	// No, se trata de una clase.
	if ( NpcData == NULL_STRING )
	{
		// Creamos al NPC.
		pNPC = (CAI_BaseNPC *)CreateEntityByName(pClass);
	}
	else
	{
		// Creamos al NPC a partir de la plantilla.
		CBaseEntity *pEntity = NULL;
		MapEntity_ParseEntity(pEntity, STRING(NpcData), NULL);

		if ( pEntity != NULL )
			pNPC = (CAI_BaseNPC *)pEntity;
	}

	return pNPC;
}

//=========================================================
// Crea un Zombi.
//=========================================================
CAI_BaseNPC *CDirectorSpawn::MakeNPC(bool Horde, bool disclosePlayer, bool checkRadius)
{
	// Desactivado
	// Esta entidad no funciona en Multiplayer.
	if ( Disabled )
		return NULL;

	// Seleccionamos una clase de NPC para crear.
	const char *pClass	= SelectRandom();
	CAI_BaseNPC *pNPC	= VerifyClass(pClass);

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pNPC )
	{
		Warning("[DIRECTOR SPAWN] Ha ocurrido un problema al intentar crear un NPC. \r\n");
		return NULL;
	}

	Vector origin;

	// Verificamos si podemos crear un zombi en el radio.
	if ( checkRadius )
	{
		if ( !CanMakeNPC(pNPC, &origin) )
			return NULL;
	}

	// Lugar de creación.
	pNPC->SetAbsOrigin(origin);

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pNPC->SetAbsAngles(angles);

	// Tiene que caer al suelo.
	pNPC->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	// Su cuerpo tiene que desaparecer al morir.
	pNPC->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	// Creamos al NPC, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pNPC);
	pNPC->SetOwnerEntity(this);
	DispatchActivate(pNPC);

	// Al parecer se atoro en una pared.
	if ( !PostSpawn(pNPC) )
		return NULL;

	// Nombre del NPC.
	pNPC->SetName(MAKE_STRING(CHILD_NAME));

#ifdef APOCALYPSE
	// Skin al azar.
	if ( pNPC->GetClassname() == "npc_zombie" )	
		pNPC->m_nSkin = random->RandomInt(1, 4);
#endif

	// Es un NPC para la horda ¡woot!
	if ( Horde )
	{
		AddHealth(pNPC);

#ifdef APOCALYPSE
		// Más rápido.
		pNPC->SetAddAccel(40);

		// No colisiona con otros NPC's. (Zombis)
		if ( random->RandomInt(1, 4) == 2 )
			pNPC->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);
#endif
	}

	// Debe conocer la ubicación del jugador (Su enemigo)
	if ( disclosePlayer )
	{
		CIN_Player *pPlayer = UTIL_GetRandomInPlayer();

		if ( pPlayer )
			pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
	}

	Childs++;
	ChildsAlive++;
	LastSpawn = gpGlobals->curtime;

	OnSpawnNPC.FireOutput(pNPC, this);
	return pNPC;
}

//=========================================================
// Crea un Zombi sin colisiones.
//=========================================================
CAI_BaseNPC *CDirectorSpawn::MakeNoCollisionNPC(bool Horde, bool disclosePlayer)
{
	// Desactivado
	if ( Disabled )
		return NULL;

	// Creamos el NPC normalmente.
	CAI_BaseNPC *pNPC = MakeNPC(Horde, disclosePlayer, false);

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	if ( !pNPC )
	{
		Warning("[DIRECTOR NPC] Ha ocurrido un problema al intentar crear un zombie. \r\n");
		return NULL;
	}

	// Lugar de creación.
	pNPC->SetAbsOrigin(GetAbsOrigin());
	// No colisiona con otros NPC's.
	pNPC->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);

	return pNPC;
}

//=========================================================
// Crea un Jefe.
//=========================================================
CAI_BaseNPC *CDirectorSpawn::MakeBoss()
{
	// Desactivado
	if ( Disabled )
		return NULL;

	// Seleccionamos una clase de NPC para crear.
	const char *pClass	= SelectRandomBoss();
	CAI_BaseNPC *pNPC	= VerifyClass(pClass);

	// Ocurrio algún problema.
	if ( !pNPC )
	{
		Warning("[DIRECTOR SPAWN] Ha ocurrido un problema al intentar crear un Jefe. \r\n");
		return NULL;
	}

	Vector origin;

	// Verificamos si podemos crear el Grunt en el radio.
	if ( !CanMakeNPC(pNPC, &origin) )
		return NULL;

	// Lugar de creación.
	pNPC->SetAbsOrigin(origin);

	// Nombre del Jefe.
	pNPC->SetName(MAKE_STRING(BOSS_NAME));

	QAngle angles	= GetAbsAngles();
	angles.x		= 0.0;
	angles.z		= 0.0;

	pNPC->SetAbsAngles(angles);

	// Tiene que caer al suelo.
	pNPC->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	// Creamos al Jefe, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pNPC);
	pNPC->SetOwnerEntity(this);
	DispatchActivate(pNPC);

	// Al parecer se atoro en una pared.
	if ( !PostSpawn(pNPC) )
		return NULL;

	// Debe conocer la ubicación del jugador (Su enemigo)
	CIN_Player *pPlayer = UTIL_GetRandomInPlayer();

	if ( pPlayer )
	{
		// Ataca al jugador YA
		pNPC->SetEnemy(pPlayer);
		pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
	}

	return pNPC;
}

//=========================================================
// Devuelve si este creador puede crear un jefe.
//=========================================================
bool CDirectorSpawn::CanMakeBoss()
{
	for ( int i = 0; i <= ARRAYSIZE(iBoss) - 1; ++i )
	{
		if ( iBoss[i] == NULL_STRING )
			continue;

		return true;
	}

	return false;
}

//=========================================================
// Ha muerto un NPC que he creado.
//=========================================================
void CDirectorSpawn::DeathNotice(CBaseEntity *pVictim)
{
	ChildsAlive--;
	ChildsKilled++;

	CDirector *pDirector = GetDirector();

	if ( pDirector )
	{
		// ¡Han matado a un NPC!
		if ( pVictim->GetEntityName() == MAKE_STRING(CHILD_NAME) )
			pDirector->ChildKilled(pVictim);
			
		// ¡Han matado a un Jefe!
		if ( pVictim->GetEntityName() == MAKE_STRING(BOSS_NAME) )
			pDirector->MBossKilled(pVictim);
	}

	OnChildDead.FireOutput(pVictim, this);
}

//=========================================================
// Selecciona una clase de NPC.
//=========================================================
const char *CDirectorSpawn::SelectRandom()
{
	int pRandom = random->RandomInt(0, ARRAYSIZE(iNpcs) - 1);

	if ( iNpcs[pRandom] == NULL_STRING )
		return SelectRandom();

	return STRING(iNpcs[pRandom]);
}

//=========================================================
// Selecciona una clase de Jefe.
//=========================================================
const char *CDirectorSpawn::SelectRandomBoss()
{
	int pRandom = random->RandomInt(0, ARRAYSIZE(iBoss) - 1);

	if ( iBoss[pRandom] == NULL_STRING )
		return SelectRandomBoss();

	return STRING(iBoss[pRandom]);
}


int CDirectorSpawn::DrawDebugTextOverlays()
{
	int text_offset			= BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char message[512];
		Q_snprintf(message, sizeof(message), 
			"NPC's creados: %i", 
		Childs);

		if ( !InGameRules()->IsMultiplayer() )
		{
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

			if ( pPlayer )
			{
				// Calculamos la distancia del creador al jugador.
				Vector distToMaker	= GetAbsOrigin() - pPlayer->GetAbsOrigin();
				float dist			= VectorNormalize(distToMaker);

				Q_snprintf(message, sizeof(message), 
					"Distancia: %i", 
				dist);
			}
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

void CDirectorSpawn::InputSpawn(inputdata_t &inputdata)
{
	MakeNPC();
}

void CDirectorSpawn::InputSpawnCount(inputdata_t &inputdata)
{
	int i;
	CAI_BaseNPC *pNPC = NULL;

	ConVarRef director_force_spawn_outview("director_force_spawn_outview");
	int oldValue = director_force_spawn_outview.GetInt();

	director_force_spawn_outview.SetValue(0);

	for ( i = 0; i <= inputdata.value.Int(); ++i )
	{
		pNPC = MakeNoCollisionNPC();

		if ( !pNPC )
		{
			DevWarning("[DIRECTOR SPAWN] No se ha podido crear el NPC (%i) \r\n", i);
			continue;
		}
	}

	director_force_spawn_outview.SetValue(oldValue);
}

void CDirectorSpawn::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

void CDirectorSpawn::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

void CDirectorSpawn::InputToggle(inputdata_t &inputdata)
{
	if ( Disabled )
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