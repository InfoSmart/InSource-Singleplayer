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
#include "npc_base.h" // Cambiame
#include "npcevent.h"
#include "soundent.h"
#include "activitylist.h"
#include "weapon_brickbat.h"
#include "npc_headcrab.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "grenade_spit.h"
#include "grenade_brickbat.h"
#include "entitylist.h"
#include "shake.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar in_basenpc_health("sk_basenpc_health", "0", 0, "Salud del NPC");

//=========================================================
// Configuración del NPC
//=========================================================

#define MODEL_BASE		"models/.mdl"
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2
#define BLOOD			BLOOD_COLOR_YELLOW

//=========================================================
// Tareas programadas
//=========================================================
enum
{
};

//=========================================================
// Tareas
//=========================================================
enum 
{
};

//=========================================================
// Condiciones
//=========================================================
enum
{
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

LINK_ENTITY_TO_CLASS(npc_base, CNPC_Base);

BEGIN_DATADESC(CNPC_Base)

	DEFINE_FIELD(m_flLastHurtTime,		FIELD_TIME),
	DEFINE_FIELD(m_nextAlertSoundTime,	FIELD_TIME),

END_DATADESC()

//=========================================================
// Spawn()
// Crear un nuevo 
//=========================================================
void CNPC_Base::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	SetModel(MODEL_BASE);
	SetBloodColor(BLOOD);

	// Tamaño
	SetHullType(HULL_WIDE_SHORT);
	SetHullSizeNormal();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	SetNavType(NAV_GROUND);	
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(255, 255, 255, 255);

	// Salud, estado del NPC y vista.
	m_iHealth			= in_basenpc_health.GetFloat();
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Precache()
// Guardar en caché objetos necesarios.
//=========================================================
void CNPC_Base::Precache()
{
	// Modelo
	PrecacheModel(MODEL_BASE);

	// Sonidos
	PrecacheScriptSound("NPC_Base.Idle");
	PrecacheScriptSound("NPC_Base.Pain");
	PrecacheScriptSound("NPC_Base.Alert");
	PrecacheScriptSound("NPC_Base.Death");
	PrecacheScriptSound("NPC_Base.Attack1");
	PrecacheScriptSound("NPC_Base.Growl");

	BaseClass::Precache();
}

//=========================================================
// Classify()
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_Base::Classify()
{
	return CLASS_BASE; 
}

//=========================================================
// IdleSound()
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Base::IdleSound()
{
	EmitSound("NPC_Base.Idle");
}

//=========================================================
// PainSound()
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Base::PainSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Base.Pain");
}

//=========================================================
// AlertSound()
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Base::AlertSound()
{
	if (gpGlobals->curtime >= m_nextAlertSoundTime)
	{
		EmitSound("NPC_Base.Alert");
		m_nextAlertSoundTime = gpGlobals->curtime + random->RandomInt(1.5, 3.0);
	}
}

//=========================================================
// DeathSound()
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Base::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Base.Death");
}

//=========================================================
// AttackSound()
// Reproducir sonido al azar de ataque.
//=========================================================
void CNPC_Base::AttackSound()
{
	EmitSound("NPC_Base.Attack1");
}

//=========================================================
// MaxYawSpeed()
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_Base::MaxYawSpeed()
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
void CNPC_Base::HandleAnimEvent(animevent_t *pEvent)
{
	/*if (pEvent->event == AE_SQUID_RANGE_ATTACK1)
	{
		return;
	}*/

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// MeleeAttack1()
// Ataque cuerpo a cuerpo #1
// En este caso: 
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

	AttackSound();
}

//=========================================================
// MeleeAttack2()
// Ataque cuerpo a cuerpo #2
// En este caso: 
//=========================================================
void CNPC_Grunt::MeleeAttack2()
{
}

//=========================================================
// MeleeAttack1Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Ataque de cola.
//=========================================================
int CNPC_Base::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7)
		return COND_CAN_MELEE_ATTACK1;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// MeleeAttack2Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Mordida
//=========================================================
int CNPC_Base::MeleeAttack2Conditions(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7 && !HasCondition(COND_CAN_MELEE_ATTACK1))
		 return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// OnTakeDamage_Alive()
// 
//=========================================================
int CNPC_Base::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// SelectSchedule()
// Seleccionar una tarea programada dependiendo del estado
//=========================================================
int CNPC_Base::SelectSchedule()
{
	switch	(m_NPCState)
	{
		case NPC_STATE_ALERT:
		{
			break;
		}

		case NPC_STATE_COMBAT:
		{
			if (HasCondition(COND_ENEMY_DEAD))
				return BaseClass::SelectSchedule();

			if (HasCondition(COND_NEW_ENEMY))
				return SCHED_WAKE_ANGRY;

			if (HasCondition(COND_CAN_MELEE_ATTACK1))
				return SCHED_MELEE_ATTACK1;

			if (HasCondition(COND_CAN_MELEE_ATTACK2))
				return SCHED_MELEE_ATTACK2;

			return SCHED_CHASE_ENEMY;
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
// StartTask()
// Realiza los calculos necesarios al iniciar una tarea.
//=========================================================
void CNPC_Base::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
		/*case TASK_MELEE_ATTACK2:
		{
			break;
		}*/

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
void CNPC_Base::RunTask(const Task_t *pTask)
{
	BaseClass::RunTask(pTask);
}

//=========================================================
// SelectIdealState()
// 
//=========================================================
NPC_STATE CNPC_Base::SelectIdealState( void )
{
	switch (m_NPCState)
	{
		case NPC_STATE_COMBAT:
		{
			if (GetEnemy() != NULL)
			{
				SetEnemy(NULL);
				return NPC_STATE_ALERT;
			}

			break;
		}
	}

	return BaseClass::SelectIdealState();
}

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

AI_BEGIN_CUSTOM_NPC(npc_base, CNPC_Base)

	//DECLARE_TASK(TASK_EAT)

	//DECLARE_CONDITION(COND_SMELL_FOOD)

	//DECLARE_ANIMEVENT(AE_SQUID_MELEE_ATTACK1);

	//DECLARE_ACTIVITY(ACT_EAT)

	//DECLARE_INTERACTION(g_interactionBullsquidThrow)

	/*
	DEFINE_SCHEDULE
	(
		SCHED_SEECRAB,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_WAKE				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MELEE_ATTACK1"
		"		TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)
	*/

AI_END_CUSTOM_NPC()