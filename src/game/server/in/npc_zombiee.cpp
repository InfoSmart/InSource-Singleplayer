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

//#include "npc_base.h" // Cambiame a la cabecera del este archivo.
#include "npc_BaseZombie.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "entitylist.h"
#include "engine/IEngineSound.h"

#include "weapon_brickbat.h"
#include "ammodef.h"

#include "player.h"
#include "gamerules.h"
#include "shake.h"

#include "vstdlib/random.h"
#include "movevars_shared.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables para la configuración.
//=========================================================

ConVar sk_zombiee_health("sk_zombiee_health", "0", 0, "Salud del Zombiee");

//=========================================================
// Configuración del NPC
//=========================================================

// Modelo
#define MODEL_BASE		"models/zombies/zombie4/zombie4.mdl"

// ¿Qué capacidades tendrá?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_RED

// Distancia de visibilidad.
#define SEE_DIST		300.0

// Campo de visión
#define FOV				0.2

// Propiedades
// No disolverse (Con la bola de energía) - No morir con la super arma de gravedad.
#define EFLAGS			EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL

class CNPC_Human_Zombie : public CNPC_BaseZombie
{
	DECLARE_CLASS(CNPC_Human_Zombie, CNPC_BaseZombie);

public:

	void Spawn();
	void Precache();

	void SetZombieModel();

	void IdleSound();
	void PainSound(const CTakeDamageInfo &info);
	void AlertSound();
	void DeathSound(const CTakeDamageInfo &info);
	void AttackSound();

	void AttackHitSound() {};
	void AttackMissSound() {};
	void FootstepSound(bool fRightFoot) {};
	void FootscuffSound(bool fRightFoot) {}; // fast guy doesn't scuff

	const char *GetMoanSound( int nSound ) { return "NPC_FastZombie.Moan1"; };

	virtual const char *GetHeadcrabClassname() { return "npc_headcrab_fast"; };
	virtual const char *GetHeadcrabModel() { return "models/headcrab.mdl"; };
	virtual const char *GetLegsModel() { return "models/gibs/fast_zombie_legs.mdl"; };
	virtual const char *GetTorsoModel() { return "models/gibs/fast_zombie_torso.mdl"; };

	float MaxYawSpeed();
	void HandleAnimEvent(animevent_t *pEvent);

	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);

	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);
	int SelectSchedule();

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	NPC_STATE SelectIdealState();

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

private:
	float m_flLastHurtTime;
	float m_nextAlertSoundTime;
};

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

LINK_ENTITY_TO_CLASS(npc_human_zombie, CNPC_Human_Zombie);

BEGIN_DATADESC(CNPC_Human_Zombie)

	DEFINE_FIELD(m_flLastHurtTime,		FIELD_TIME),
	DEFINE_FIELD(m_nextAlertSoundTime,	FIELD_TIME),

END_DATADESC()

//=========================================================
// Spawn()
// Crear un nuevo 
//=========================================================
void CNPC_Human_Zombie::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	//SetModel(MODEL_BASE);
	SetBloodColor(BLOOD);

	// Tamaño
	//SetHullType(HULL_WIDE_SHORT);
	//SetHullSizeNormal();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetNavType(NAV_GROUND);	
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(255, 255, 255, 255);
	SetDistLook(SEE_DIST);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth			= sk_zombiee_health.GetFloat();
	m_NPCState			= NPC_STATE_NONE;
	m_flFieldOfView		= FOV;

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFLAGS);

	BaseClass::Spawn();
}

//=========================================================
// Precache()
// Guardar en caché objetos necesarios.
//=========================================================
void CNPC_Human_Zombie::Precache()
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


void CNPC_Human_Zombie::SetZombieModel()
{
	SetModel(MODEL_BASE);
	SetHullType(HULL_HUMAN);

	SetBodygroup(ZOMBIE_BODYGROUP_HEADCRAB, true);

	SetHullSizeNormal();
	SetDefaultEyeOffset();
}

//=========================================================
// IdleSound()
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Human_Zombie::IdleSound()
{
	EmitSound("NPC_Base.Idle");
} 

//=========================================================
// PainSound()
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Human_Zombie::PainSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Base.Pain");
}

//=========================================================
// AlertSound()
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Human_Zombie::AlertSound()
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
void CNPC_Human_Zombie::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Base.Death");
}

//=========================================================
// AttackSound()
// Reproducir sonido al azar de ataque.
//=========================================================
void CNPC_Human_Zombie::AttackSound()
{
	EmitSound("NPC_Base.Attack1");
}

//=========================================================
// MaxYawSpeed()
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_Human_Zombie::MaxYawSpeed()
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
void CNPC_Human_Zombie::HandleAnimEvent(animevent_t *pEvent)
{
	// DEBUG
	// Eliminalo cuando esto este listo.
	const char *pName = EventList_NameForIndex(pEvent->event);
	DevMsg("[NPC] Se ha producido el evento %s \n", pName);

	/*if (pEvent->event == AE_AGRUNT_MELEE_ATTACK_HIGH)
	{
		MeleeAttack1();
		return;
	}*/

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// MeleeAttack1()
// Ataque cuerpo a cuerpo #1
// En este caso: 
//=========================================================
/*
void CNPC_Human_Zombie::MeleeAttack1()
{

	// Atacar
	CBaseEntity *pHurt = CheckTraceHullAttack(70, Vector(-16,-16,-16), Vector(16,16,16), sk_grunt_dmg_high.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB);

	// ¿Le hice daño?
	if (pHurt) 
	{
		Vector forward, up;
		AngleVectors(GetAbsAngles(), &forward, NULL, &up);

		// Aturdirlo
		if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
			 pHurt->ViewPunch(QAngle(70, 0, -70));
		
		// Aventarlo por los aires.
		pHurt->ApplyAbsVelocityImpulse(400 * (up + 1*forward));
	}

	AttackSound();
}
*/

//=========================================================
// MeleeAttack2()
// Ataque cuerpo a cuerpo #2
// En este caso: 
//=========================================================
/*
void CNPC_Human_Zombie::MeleeAttack2()
{
}
*/

//=========================================================
// MeleeAttack1Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: 
//=========================================================
int CNPC_Human_Zombie::MeleeAttack1Conditions(float flDot, float flDist)
{
	if(!GetEnemy())
		return COND_NONE;

	// Lo tengo a mi alcanze
	if (flDist <= 85 && flDot >= 0.7)
		return COND_CAN_MELEE_ATTACK1;

	int baseResult = BaseClass::MeleeAttack1Conditions(flDot, flDist);

	if (baseResult == COND_TOO_FAR_TO_ATTACK || baseResult == COND_NOT_FACING_ATTACK)
		return COND_NONE;
	
	return COND_NONE;
}

//=========================================================
// MeleeAttack2Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: 
//=========================================================
int CNPC_Human_Zombie::MeleeAttack2Conditions(float flDot, float flDist)
{
	// Lo tengo a mi alcanze y no haremos el primer ataque cuerpo a cuerpo.
	if (flDist <= 85 && flDot >= 0.7 && !HasCondition(COND_CAN_MELEE_ATTACK1))
		 return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// OnTakeDamage_Alive()
// 
//=========================================================
int CNPC_Human_Zombie::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// SelectSchedule()
// Seleccionar una tarea programada dependiendo del estado
//=========================================================
int CNPC_Human_Zombie::SelectSchedule()
{
	switch	(m_NPCState)
	{
		// Alerta
		// Hemos detectado o estamos sintiendo a un enemigo cercano.
		case NPC_STATE_ALERT:
		{
			break;
		}

		// Combate
		// Estamos atacando a un enemigo.
		case NPC_STATE_COMBAT:
		{
			// ¿Enemigo muerto? Pasemos a la siguiente tarea...
			if (HasCondition(COND_ENEMY_DEAD))
				return BaseClass::SelectSchedule();

			// ¡Un nuevo enemigo!
			if (HasCondition(COND_NEW_ENEMY))
				return SCHED_WAKE_ANGRY;

			// Ataque cuerpo a cuerpo 1
			if (HasCondition(COND_CAN_MELEE_ATTACK1))
				return SCHED_MELEE_ATTACK1;

			// Ataque cuerpo a cuerpo 2
			if (HasCondition(COND_CAN_MELEE_ATTACK2))
				return SCHED_MELEE_ATTACK2;

			// Vigilando en busca de enemigos.
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
void CNPC_Human_Zombie::StartTask(const Task_t *pTask)
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
void CNPC_Human_Zombie::RunTask(const Task_t *pTask)
{
	BaseClass::RunTask(pTask);
}

//=========================================================
// SelectIdealState()
// 
//=========================================================
NPC_STATE CNPC_Human_Zombie::SelectIdealState( void )
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

AI_BEGIN_CUSTOM_NPC(npc_base, CNPC_Human_Zombie)

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