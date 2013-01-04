//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BASE
//
// Usa este archivo para hacer una base al crear un nuevo NPC.
//
//=====================================================================================//

#include "cbase.h"
#include "game.h"

#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_moveprobe.h"
#include "ai_route.h"
#include "ai_navigator.h"
#include "ai_hull.h"
#include "ai_moveprobe.h"
#include "ai_default.h"
#include "ai_memory.h"

#include "npc_grunt.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "entitylist.h"
#include "engine/ienginesound.h"

#include "weapon_brickbat.h"
#include "ammodef.h"
#include "grenade_spit.h"
#include "grenade_brickbat.h"

#include "player.h"
#include "gamerules.h"
#include "shake.h"

#include "gib.h"
#include "physobj.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "vstdlib/random.h"
#include "movevars_shared.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar sk_grunt_health		("sk_grunt_health", "0", 0, "Salud del NPC");
ConVar sk_grunt_debug_health("sk_grunt_debug_health", "0", 0, "Ver la salud del NPC");
ConVar sk_grunt_dmg_high	("sk_grunt_dmg_high", "0", 0, "Daño causado por un golpe alto");
ConVar sk_grunt_dmg_low		("sk_grunt_dmg_low", "0", 0, "Daño causado por un golpe bajo");

extern ISoundEmitterSystemBase *soundemitterbase;

//=========================================================
// Configuración del NPC
//=========================================================

// Predeterminado
#define MODEL_BASE		"models/xenians/agrunt.mdl"
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_TURN_HEAD
#define BLOOD			BLOOD_COLOR_YELLOW
#define SEE_DIST		9000.0

// Opciones extra - Flags
#define SF_GRUNT_NO_BACKGROUND_MUSIC	0x00010000

// Lanzar objetos
#define THROW_PHYSICS_FARTHEST_OBJECT	40.0*12.0
#define THROW_PHYSICS_SEARCH_DEPTH		80
#define THROW_PHYSICS_MAX_MASS			3000
#define THROW_PHYSICS_SWATDIST			200

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_THROW = LAST_SHARED_SCHEDULE,
	SCHED_CHANGE_POSITION
};

//=========================================================
// Tareas
//=========================================================
enum 
{
	TASK_GET_PATH_TO_PHYSOBJ = LAST_SHARED_TASK,
	TASK_THROW_PHYSOBJ,
	TASK_DELAY_THROW
};

//=========================================================
// Condiciones
//=========================================================
enum
{
	COND_CAN_THROW = LAST_SHARED_CONDITION,
};

//=========================================================
// Eventos de animaciones
//=========================================================

int AE_AGRUNT_MELEE_ATTACK_HIGH;
int AE_AGRUNT_MELEE_ATTACK_LOW;

//=========================================================
// Animaciones
//=========================================================

int ACT_MEELE_ATTACK1;

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS(npc_grunt, CNPC_Grunt);

BEGIN_DATADESC(CNPC_Grunt)

	DEFINE_THINKFUNC(MusicThink),

	DEFINE_FIELD(m_flLastHurtTime,			FIELD_TIME),
	DEFINE_FIELD(m_nextAlertSoundTime,		FIELD_TIME),
	DEFINE_FIELD(m_flNextThrow,				FIELD_TIME),

	DEFINE_FIELD(m_volumeFadeOutBackgound,	FIELD_FLOAT),

	DEFINE_FIELD(m_hPhysicsEnt,				FIELD_EHANDLE),
	DEFINE_FIELD(m_hPhysicsCanThrow,		FIELD_BOOLEAN),

END_DATADESC()

//=========================================================
// Spawn()
// Crear un nuevo Grunt
//=========================================================
void CNPC_Grunt::Spawn()
{
	Precache();
	NPCInit();

	// Modelo y color de sangre.
	SetModel(MODEL_BASE);
	SetBloodColor(BLOOD);

	// Tamaño
	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	SetNavType(NAV_GROUND);	
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(255, 255, 255, 255);
	SetDistLook(SEE_DIST);

	// Salud, estado del NPC y vista.
	m_iHealth				= sk_grunt_health.GetFloat();
	m_NPCState				= NPC_STATE_IDLE;
	m_flFieldOfView			= -0.5;

	// Más información
	m_flNextThrow				= gpGlobals->curtime;
	m_nextAlertSoundTime		= gpGlobals->curtime + 3;
	m_hPhysicsEnt				= NULL;
	m_hPhysicsCanThrow			= false;
	m_backgroundMusicStarted	= false;

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL);

	RegisterThinkContext("MusicBackgroundThink");
	SetContextThink(&CNPC_Grunt::MusicThink, gpGlobals->curtime, "MusicBackgroundThink");

	BaseClass::Spawn();
}

//=========================================================
// Precache()
// Guardar los objetos necesarios en caché.
//=========================================================
void CNPC_Grunt::Precache()
{
	// Modelo
	PrecacheModel(MODEL_BASE);

	// Sonidos
	PrecacheScriptSound("NPC_Grunt.Idle");
	PrecacheScriptSound("NPC_Grunt.Pain");
	PrecacheScriptSound("NPC_Grunt.Alert");
	PrecacheScriptSound("NPC_Grunt.Death");
	PrecacheScriptSound("NPC_Grunt.HighAttack");
	PrecacheScriptSound("NPC_Grunt.LowAttack");
	PrecacheScriptSound("NPC_Grunt.BackgroundMusic");
	PrecacheScriptSound("NPC_Grunt.Step");

	BaseClass::Precache();
}

//=========================================================
// Classify()
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_Grunt::Classify()
{
	return CLASS_GRUNT; 
}

//=========================================================
// IdleSound()
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Grunt::IdleSound()
{
	EmitSound("NPC_Grunt.Idle");
}

//=========================================================
// PainSound()
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Grunt::PainSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Grunt.Pain");
}

//=========================================================
// AlertSound()
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Grunt::AlertSound()
{
	if (gpGlobals->curtime >= m_nextAlertSoundTime)
	{
		EmitSound("NPC_Grunt.Alert");
		m_nextAlertSoundTime = gpGlobals->curtime + random->RandomInt(.5, 2.0);
	}
}

//=========================================================
// DeathSound()
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Grunt::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Grunt.Death");
}

//=========================================================
// AttackSound()
// Reproducir sonido al azar de ataque alto/fuerte.
//=========================================================
void CNPC_Grunt::HighAttackSound()
{
	EmitSound("NPC_Grunt.HighAttack");
}

//=========================================================
// AttackSound()
// Reproducir sonido al azar de ataque alto/fuerte.
//=========================================================
void CNPC_Grunt::LowAttackSound()
{
	EmitSound("NPC_Grunt.LowAttack");
}

//=========================================================
// StartBackgroundMusic()
// Reproducir música de fondo.
//=========================================================
void CNPC_Grunt::StartBackgroundMusic()
{
	CSoundParameters params;

	if (!soundemitterbase->GetParametersForSound("NPC_Grunt.BackgroundMusic", params, GENDER_NONE))
		return;

	m_volumeFadeOutBackgound = 1.0f;
	UTIL_EmitAmbientSound(GetSoundSourceIndex(), GetAbsOrigin(), params.soundname, params.volume, params.soundlevel, 0, params.pitch);
}

//=========================================================
// VolumeBackgroundMusic()
// Actualizar el volumen de la música de fondo.
//=========================================================
void CNPC_Grunt::VolumeBackgroundMusic(float volume)
{
	CSoundParameters params;

	if (!soundemitterbase->GetParametersForSound("NPC_Grunt.BackgroundMusic", params, GENDER_NONE))
		return;

	if(volume > 0)
		UTIL_EmitAmbientSound(GetSoundSourceIndex(), GetAbsOrigin(), params.soundname, volume, params.soundlevel, SND_CHANGE_VOL, params.pitch);
	else
		UTIL_EmitAmbientSound(GetSoundSourceIndex(), GetAbsOrigin(), params.soundname, volume, params.soundlevel, SND_STOP, params.pitch);
}

//=========================================================
// MaxYawSpeed()
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_Grunt::MaxYawSpeed()
{
	switch (GetActivity())
	{
		case ACT_WALK:
			return 25;	
		break;

		case ACT_RUN:
			return 15;
		break;

		default:
			return 90;
		break;
	}
}

//=========================================================
// HandleAnimEvent()
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_Grunt::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_AGRUNT_MELEE_ATTACK_HIGH)
	{
		MeleeAttack1();
		return;
	}

	if (pEvent->event == AE_AGRUNT_MELEE_ATTACK_LOW)
	{
		MeleeAttack2();
		return;
	}

	const char *pName = EventList_NameForIndex(pEvent->event);
	Msg("Evento: %s \n", pName);

	if(pEvent->event == NPC_EVENT_LEFTFOOT || pEvent->event == NPC_EVENT_RIGHTFOOT)
	{
		EmitSound("NPC_Grunt.Step", pEvent->eventtime);
		UTIL_ScreenShake(GetAbsOrigin(), 5.0, 10.0, 3.0, 350, SHAKE_START, false);

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// MeleeAttack1()
// Ataque cuerpo a cuerpo #1
// En este caso: Golpe alto
//=========================================================
void CNPC_Grunt::MeleeAttack1()
{
	// Atacar
	CBaseEntity *pHurt = CheckTraceHullAttack(70, Vector(-16,-16,-16), Vector(16,16,16), sk_grunt_dmg_high.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB);

	// ¿Le hice daño?
	if (pHurt) 
	{
		Vector forward, up;
		AngleVectors(GetAbsAngles(), &forward, NULL, &up);

		// Aventarlo por los aires.
		if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
			 pHurt->ViewPunch(QAngle(70, 0, -70));
			
		pHurt->ApplyAbsVelocityImpulse(400 * (up + 1*forward));
	}

	HighAttackSound();
}

//=========================================================
// MeleeAttack2()
// Ataque cuerpo a cuerpo #2
// En este caso: Golpe bajo
//=========================================================
void CNPC_Grunt::MeleeAttack2()
{
	// Atacar
	CBaseEntity *pHurt = CheckTraceHullAttack(70, Vector(-16,-16,-16), Vector(16,16,16), sk_grunt_dmg_low.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB);

	// ¿Le hice daño?
	if (pHurt) 
	{
		Vector right, up;
		AngleVectors(GetAbsAngles(), NULL, &right, &up);

		// Aventarlo por los aires.
		if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
			 pHurt->ViewPunch(QAngle(40, 0, -40));
			
		pHurt->ApplyAbsVelocityImpulse(200 * (up+2*right));
	}

	LowAttackSound();
}

//=========================================================
// MeleeAttack1Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe alto
//=========================================================
int CNPC_Grunt::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL)
		return COND_NONE;

	if (flDist <= 85 && flDot >= 0.7)
	{
		m_flNextThrow += 2.0;
		return COND_CAN_MELEE_ATTACK1;
	}

	// Build a cube-shaped hull, the same hull that MeleeAttack is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors(&forward, NULL, NULL);
	trace_t	tr;
	AI_TraceHull(WorldSpaceCenter(), WorldSpaceCenter() + forward * 50, vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction == 1.0 || !tr.m_pEnt)
		return COND_TOO_FAR_TO_ATTACK;

	if (tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt))))
		return COND_CAN_MELEE_ATTACK1;

	if (!tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt)
		return COND_CAN_MELEE_ATTACK1;

 	if (!tr.m_pEnt->IsWorld() && tr.m_pEnt->VPhysicsGetObject()->IsMoveable() && tr.m_pEnt->VPhysicsGetObject()->GetMass() <= THROW_PHYSICS_MAX_MASS)
		return COND_CAN_MELEE_ATTACK1;

	if (m_hPhysicsCanThrow)
	{
		Msg("CANTHROW!! \r\n");

		m_hPhysicsCanThrow	= false;
		m_flNextThrow		+= 2.0;

		return COND_CAN_MELEE_ATTACK1;
	}
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// MeleeAttack2Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe bajo
//=========================================================
int CNPC_Grunt::MeleeAttack2Conditions(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7)
	{
		m_flNextThrow += 2.0;
		return COND_CAN_MELEE_ATTACK2;
	}

	if (m_hPhysicsCanThrow)
	{
		Msg("CANTHROW\r\n");

		m_hPhysicsCanThrow	= false;
		m_flNextThrow		+= 2.0;

		return COND_CAN_MELEE_ATTACK2;
	}
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// OnTakeDamage_Alive()
// 
//=========================================================
int CNPC_Grunt::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// Event_Killed()
// Muerte del NPC.
//=========================================================
void CNPC_Grunt::Event_Killed(const CTakeDamageInfo &info)
{
	if(!HasSpawnFlags(SF_GRUNT_NO_BACKGROUND_MUSIC))
		VolumeBackgroundMusic(-1);

	BaseClass::Event_Killed(info);
}

//=========================================================
// MusicThink()
// Bucle de tareas para la música de fondo.
// Disminuye el sonido de la música conforme se mata al NPC.
//=========================================================
void CNPC_Grunt::MusicThink()
{
	if(sk_grunt_debug_health.GetBool())
		DevMsg("SALUD DEL ALIEN GRUNT: %i\r\n", GetHealth());

	if(!HasSpawnFlags(SF_GRUNT_NO_BACKGROUND_MUSIC) && !m_backgroundMusicStarted)
	{
		StartBackgroundMusic();
		m_backgroundMusicStarted = true;
	}

	if(!m_backgroundMusicStarted)
		return;

	if(GetHealth() <= 250)
	{
		m_volumeFadeOutBackgound = (0.05f * GetHealth());
		VolumeBackgroundMusic(m_volumeFadeOutBackgound);

		if(m_volumeFadeOutBackgound == 0 || m_volumeFadeOutBackgound == 0.0f)
		{
			SetNextThink(0);
			return;
		}
	}

	SetNextThink(gpGlobals->curtime + .5, "MusicBackgroundThink");
}

//=========================================================
// GatherConditions()
// 
//=========================================================
void CNPC_Grunt::GatherConditions()
{
	BaseClass::GatherConditions();

	if(gpGlobals->curtime >= m_flNextThrow && m_hPhysicsEnt == NULL)
		FindNearestPhysicsObject();

	if(m_hPhysicsEnt != NULL && gpGlobals->curtime >= m_flNextThrow)
		SetCondition(COND_CAN_THROW);
	else
		ClearCondition(COND_CAN_THROW);
}

//=========================================================
// SelectCombatSchedule()
// Selecciona una tarea programada dependiendo del combate
//=========================================================
int CNPC_Grunt::SelectCombatSchedule()
{
	if (HasCondition(COND_ENEMY_DEAD))
		return BaseClass::SelectSchedule();

	if (HasCondition(COND_NEW_ENEMY))
		return SCHED_WAKE_ANGRY;

	if(HasCondition(COND_HEAVY_DAMAGE))
		return SCHED_MOVE_AWAY_FROM_ENEMY;

	if (HasCondition(COND_CAN_MELEE_ATTACK1))
		return SCHED_MELEE_ATTACK1;

	if (HasCondition(COND_CAN_MELEE_ATTACK2))
		return SCHED_MELEE_ATTACK2;

	if(HasCondition(COND_CAN_THROW))
		return SCHED_THROW;

	if (HasCondition(COND_ENEMY_OCCLUDED) || HasCondition(COND_ENEMY_TOO_FAR) || HasCondition(COND_TOO_FAR_TO_ATTACK) || HasCondition(COND_NOT_FACING_ATTACK))
		return SCHED_CHASE_ENEMY;

	return SCHED_CHANGE_POSITION;
}

//=========================================================
// SelectSchedule()
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CNPC_Grunt::SelectSchedule()
{
	switch	(m_NPCState)
	{
		case NPC_STATE_ALERT:
		{
			break;
		}

		case NPC_STATE_COMBAT:
		{
			return SelectCombatSchedule();
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
// SelectIdealState()
// 
//=========================================================
NPC_STATE CNPC_Grunt::SelectIdealState()
{
	switch (m_NPCState)
	{
		case NPC_STATE_COMBAT:
		{
			if (GetEnemy() == NULL)
				return NPC_STATE_ALERT;

			break;
		}

		case NPC_STATE_DEAD:
		{
			return NPC_STATE_DEAD;
			break;
		}
	}

	return BaseClass::SelectIdealState();
}

//=========================================================
// StartTask()
// Realiza los calculos necesarios al iniciar una tarea.
//=========================================================
void CNPC_Grunt::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
		case TASK_DELAY_THROW:
		{
			m_flNextThrow = gpGlobals->curtime + pTask->flTaskData;
			TaskComplete();

			break;
		}

		case TASK_GET_PATH_TO_PHYSOBJ:
		{
			if(m_hPhysicsEnt == NULL)
				TaskFail("No hay ningun objeto para lanzar");

			Vector vecGoalPos;
			Vector vecDir;

			vecDir		= GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z	= 0;

			AI_NavGoal_t goal(m_hPhysicsEnt->WorldSpaceCenter());
			goal.pTarget = m_hPhysicsEnt;
			GetNavigator()->SetGoal(goal);

			TaskComplete();
			break;
		}

		case TASK_THROW_PHYSOBJ:
		{
			if(m_hPhysicsEnt == NULL)
				TaskFail("No hay ningun objeto para lanzar");

			else if(DistToPhysicsEnt() > THROW_PHYSICS_SWATDIST)
				TaskFail("El objeto ya no esta a mi alcanze");

			else
			{
				m_hPhysicsCanThrow	= true;
				TaskComplete();
			}

			break;
		}

		default:
		{
			BaseClass::StartTask(pTask);
			break;
		}
	}
}

//=========================================================
// RunTask
//
//=========================================================
void CNPC_Grunt::RunTask(const Task_t *pTask)
{
	switch(pTask->iTask)
	{
		default:
			BaseClass::RunTask(pTask);
		break;
	}	
}

//=========================================================
//=========================================================

// FUNCIONES PERSONALIZADAS

//=========================================================
//=========================================================

//----------------------------------------------------
// FindNearestPhysicsObject()
// Encontrar el objeto más cercano ideal a lanzar
// Debe estar tanto cerca del zombi como del enemigo.
//----------------------------------------------------
bool CNPC_Grunt::FindNearestPhysicsObject()
{
	CBaseEntity		*pList[THROW_PHYSICS_SEARCH_DEPTH];
	CBaseEntity		*pNearest = NULL;
	IPhysicsObject	*pPhysObj;

	Vector			vecDirToEnemy;
	Vector			vecDirToObject;

	if(!GetEnemy())
	{
		m_hPhysicsEnt = NULL;
		return false;
	}

	// Calcular la distancia al enemigo.
	vecDirToEnemy = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	float dist = VectorNormalize(vecDirToEnemy);
	vecDirToEnemy.z = 0;

	float flNearestDist = min(dist, THROW_PHYSICS_FARTHEST_OBJECT * 0.5);
	Vector vecDelta(flNearestDist, flNearestDist, GetHullHeight() * 2.0);

	class CGruntSwatEntitiesEnum : public CFlaggedEntitiesEnum
	{
		public:
			CGruntSwatEntitiesEnum(CBaseEntity **pList, int listMax, int iMaxMass) : CFlaggedEntitiesEnum(pList, listMax, 0), m_iMaxMass(iMaxMass)
			{
			}

			virtual IterationRetval_t EnumElement(IHandleEntity *pHandleEntity)
			{
				CBaseEntity *pEntity = gEntList.GetBaseEntity(pHandleEntity->GetRefEHandle());

				if (pEntity && 
					 pEntity->VPhysicsGetObject() && 
					 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
					 pEntity->VPhysicsGetObject()->IsAsleep() && 
					 pEntity->VPhysicsGetObject()->IsMoveable())
				{
					return CFlaggedEntitiesEnum::EnumElement(pHandleEntity );
				}

				return ITERATION_CONTINUE;
			}

			int m_iMaxMass;
	};

	CGruntSwatEntitiesEnum swatEnum(pList, THROW_PHYSICS_SEARCH_DEPTH, THROW_PHYSICS_MAX_MASS);
	int count = UTIL_EntitiesInBox(GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, &swatEnum);

	Vector vecGruntKnees;
	CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.25f), &vecGruntKnees);

	float flDist;

	for(int i = 0 ; i < count ; i++)
	{
		pPhysObj = pList[i]->VPhysicsGetObject();

		Assert(!(!pPhysObj || pPhysObj->GetMass() > THROW_PHYSICS_MAX_MASS || !pPhysObj->IsAsleep()));

		Vector center	= pList[i]->WorldSpaceCenter();
		flDist			= UTIL_DistApprox2D(GetAbsOrigin(), center);

		if(flDist >= flNearestDist)
			continue;

		// Este objeto esta muy cerca... pero ¿esta entre el npc y el enemigo?
		vecDirToObject = pList[i]->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		//if(DotProduct(vecDirToEnemy, vecDirToObject) < 0.8)
		//	continue;

		if(flDist >= UTIL_DistApprox2D(center, GetEnemy()->GetAbsOrigin()))
			continue;

		// No lanzar objetos que esten más arriba de mis rodillas.
		//if ((center.z + pList[i]->BoundingRadius()) < vecGruntKnees.z)
		//	continue;

		// No lanzar objetos que esten sobre mi cabeza.
		if(center.z > EyePosition().z)
			continue;

		vcollide_t *pCollide = modelinfo->GetVCollide(pList[i]->GetModelIndex());
		
		Vector objMins, objMaxs;
		physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles());

		if (objMaxs.z < vecGruntKnees.z)
			continue;

		if (!FVisible(pList[i]))
			continue;

		if(!GetEnemy()->FVisible(pList[i]))
			continue;

		// No lanzar cadaveres de ningún tipo...
		if (FClassnameIs(pList[i], "physics_prop_ragdoll"))
			continue;
			
		if (FClassnameIs(pList[i], "prop_ragdoll"))
			continue;

		// El objeto es pefecto para lanzar, cumple lo necesario.
		pNearest		= pList[i];
		flNearestDist	= flDist;
	}

	m_hPhysicsEnt = pNearest;

	if(m_hPhysicsEnt == NULL)
		return false;
	
	return true;
}

float CNPC_Grunt::DistToPhysicsEnt()
{
	if (m_hPhysicsEnt != NULL)
		return UTIL_DistApprox2D(GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter());

	return THROW_PHYSICS_SWATDIST + 1;
}

bool CNPC_Grunt::IsPhysicsObject(CBaseEntity *pEntity)
{
	if (pEntity && 
		pEntity->VPhysicsGetObject()->IsAsleep() && 
		pEntity->VPhysicsGetObject()->IsMoveable())
		return true;

	return false;
}

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

AI_BEGIN_CUSTOM_NPC(npc_grunt, CNPC_Grunt)

	DECLARE_TASK(TASK_GET_PATH_TO_PHYSOBJ)
	DECLARE_TASK(TASK_THROW_PHYSOBJ)
	DECLARE_TASK(TASK_DELAY_THROW)

	DECLARE_CONDITION(COND_CAN_THROW)

	DECLARE_ANIMEVENT(AE_AGRUNT_MELEE_ATTACK_HIGH);
	DECLARE_ANIMEVENT(AE_AGRUNT_MELEE_ATTACK_LOW);

	DECLARE_ACTIVITY(ACT_MEELE_ATTACK1)

	//DECLARE_INTERACTION(g_interactionBullsquidThrow)

	DEFINE_SCHEDULE
	(
		SCHED_THROW,
	
		"	Tasks"
		"		TASK_DELAY_THROW				3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_GET_PATH_TO_PHYSOBJ		0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_THROW_PHYSOBJ				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CHANGE_POSITION,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_WANDER									720432"
		"		TASK_RUN_PATH								0"
		"		TASK_STOP_MOVING							0"
		"		TASK_WAIT_FACE_ENEMY_RANDOM					5"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_MOVE_AWAY"
		"		COND_NEW_ENEMY"

	)

AI_END_CUSTOM_NPC()