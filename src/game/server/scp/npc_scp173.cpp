//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC SCP-173
//
// 
//
//=====================================================================================//

#include "cbase.h"
#include "game.h"

#include "npc_scp173.h"
#include "in_player.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "in_gamerules.h"
#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de comandos para la consola.
//=========================================================


//=========================================================
// Configuración del NPC
//=========================================================

// Modelo
#define MODEL_BASE		"models/player.mdl"

// ¿Qué capacidades tendrá?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_MECH

// Color de renderizado
#define RENDER_COLOR	255, 255, 255, 255

// Distancia de visibilidad.
#define SEE_DIST		9000.0f

// Campo de visión
#define FOV				VIEW_FIELD_FULL

// Propiedades
// No disolverse (Con la bola de energía) - No morir con la super arma de gravedad.
#define EFLAGS			EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_CHANGE_POSITION = LAST_SHARED_SCHEDULE
};

//=========================================================
// Tareas
//=========================================================
enum 
{
	TASK_GET_PATH_TO_PHYSOBJ = LAST_SHARED_TASK
};

//=========================================================
// Condiciones
//=========================================================
enum
{
	COND_CAN_THROW = LAST_SHARED_CONDITION
};

//=========================================================
// Eventos de animaciones
//=========================================================

//=========================================================
// Animaciones
//=========================================================

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( npc_scp173, CNPC_SCP173 );

BEGIN_DATADESC( CNPC_SCP173 )

END_DATADESC()

//=========================================================
// Crear un nuevo Grunt
//=========================================================
void CNPC_SCP173::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	SetModel(MODEL_BASE);
	SetBloodColor(BLOOD);

	// Tamaño
	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(RENDER_COLOR);
	SetDistLook(SEE_DIST);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth				= 5;
	m_NPCState				= NPC_STATE_IDLE;
	m_flFieldOfView			= FOV;

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFLAGS);

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CNPC_SCP173::Precache()
{
	// Modelo
	PrecacheModel(MODEL_BASE);

	// Sonidos

	BaseClass::Precache();
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CNPC_SCP173::Think()
{
	BaseClass::Think();
	
	// ¡Eh! Te estan viendo.
	if ( UTIL_InPlayersViewCone(this) && IsMoving() )
	{
		GetNavigator()->StopMoving();
		GetNavigator()->ClearGoal();
	}
}

//=========================================================
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_SCP173::Classify()
{
	return CLASS_SCP_173; 
}

Disposition_t CNPC_SCP173::IRelationType(CBaseEntity *pTarget)
{
	return BaseClass::IRelationType(pTarget);
}

//=========================================================
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_SCP173::IdleSound()
{
	
}

//=========================================================
// Reproducir sonido al azar de ataque alto/fuerte.
//=========================================================
void CNPC_SCP173::AttackSound()
{
	EmitSound("NPC_Grunt.LowAttack");
}

//=========================================================
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_SCP173::MaxYawSpeed()
{
	switch ( GetActivity() )
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
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_SCP173::HandleAnimEvent(animevent_t *pEvent)
{
	const char *pName = EventList_NameForIndex(pEvent->event);
	DevMsg("[SCP 173] Se ha producido el evento %s \n", pName);

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// Ataque cuerpo a cuerpo.
//=========================================================
void CNPC_SCP173::MeleeAttack(bool highAttack)
{
	// No hay un enemigo a quien atacar.
	if ( !GetEnemy() )
		return;

	CBaseEntity *pVictim	= NULL;
	int pTypeDamage			= DMG_SLASH | DMG_ALWAYSGIB;

	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs() + 5;
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	// Siempre debemos atacar a un Bullseye
	if ( GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		pVictim = GetEnemy();
		CTakeDamageInfo info(this, this, vec3_origin, GetAbsOrigin(), 100, pTypeDamage);
		pVictim->TakeDamage(info);
	}

	
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Romperte el cuello
//=========================================================
int CNPC_SCP173::MeleeAttack1Conditions(float flDot, float flDist)
{	
	// Distancia y angulo correcto, ¡ataque!
	if ( flDist <= 40 && flDot >= 0.7 )
		return COND_CAN_MELEE_ATTACK1;

	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs();
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	Vector forward;
	GetVectors(&forward, NULL, NULL);

	trace_t	tr;
	AI_TraceHull(WorldSpaceCenter(), WorldSpaceCenter() + forward * 50, vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr);

	if ( tr.fraction == 1.0 || !tr.m_pEnt )
		return COND_TOO_FAR_TO_ATTACK;

//	if ( tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt))) )
		//return COND_CAN_MELEE_ATTACK1;

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
		return COND_CAN_MELEE_ATTACK1;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// 
//=========================================================
int CNPC_SCP173::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// Muerte del NPC.
//=========================================================
void CNPC_SCP173::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);
}

//=========================================================
// 
//=========================================================
void CNPC_SCP173::GatherConditions()
{
	BaseClass::GatherConditions();
}

//=========================================================
// Selecciona una tarea programada dependiendo del combate
//=========================================================
int CNPC_SCP173::SelectCombatSchedule()
{
	// Hemos perdido al enemigo, buscarlo o perseguirlo.
	if ( HasCondition(COND_ENEMY_OCCLUDED) || HasCondition(COND_ENEMY_TOO_FAR) || HasCondition(COND_TOO_FAR_TO_ATTACK) || HasCondition(COND_NOT_FACING_ATTACK) )
		return SCHED_CHASE_ENEMY;

	return BaseClass::SelectSchedule();
}

//=========================================================
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CNPC_SCP173::SelectSchedule()
{
	switch ( m_NPCState )
	{
		case NPC_STATE_COMBAT:
		{
			return SelectCombatSchedule();
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
//=========================================================
int CNPC_SCP173::TranslateSchedule(int scheduleType)
{
	return BaseClass::TranslateSchedule(scheduleType);
}

//=========================================================
//=========================================================
Activity CNPC_SCP173::NPC_TranslateActivity(Activity baseAct)
{
	switch ( baseAct )
	{
		case ACT_WALK:
		case ACT_RUN:
		{
			if ( UTIL_InPlayersViewCone(this) )
				return ACT_IDLE;
		}
	}

	return BaseClass::NPC_TranslateActivity(baseAct);
}

//=========================================================
// 
//=========================================================
NPC_STATE CNPC_SCP173::SelectIdealState()
{
	switch ( m_NPCState )
	{
		case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL )
				return NPC_STATE_ALERT;

			break;
		}
	}

	return BaseClass::SelectIdealState();
}

//=========================================================
// Realiza los calculos necesarios al iniciar una tarea.
//=========================================================
void CNPC_SCP173::StartTask(const Task_t *pTask)
{
	switch ( pTask->iTask )
	{
		
	}

	BaseClass::StartTask(pTask);
}

//=========================================================
//
//=========================================================
void CNPC_SCP173::RunTask(const Task_t *pTask)
{
	switch ( pTask->iTask )
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

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

/*
AI_BEGIN_CUSTOM_NPC( npc_scp173 CNPC_SCP173 )

	//DECLARE_TASK( TASK_CANNON_FIRE )

	//DECLARE_CONDITION( COND_CAN_THROW )

	//DECLARE_ANIMEVENT( AE_AGRUNT_MELEE_ATTACK_HIGH )

	//DECLARE_ACTIVITY( ACT_SWATLEFTMID )

AI_END_CUSTOM_NPC()
*/