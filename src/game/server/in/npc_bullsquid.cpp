//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Bullsquid
//
// Reintegración por Iván Bravo para el modelo creado por Black Mesa Team.
//
//====================================================================================//

#include "cbase.h"
#include "game.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_moveprobe.h"
#include "ai_route.h"
#include "ai_navigator.h"
#include "npc_bullsquid.h"
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

#define		SQUID_SPRINT_DIST	256

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar sk_bullsquid_health("sk_bullsquid_health", "0", 0, "Salud del Bullsquid");
ConVar sk_bullsquid_dmg_bite("sk_bullsquid_dmg_bite", "0", 0, "Daño causado por una mordida");
ConVar sk_bullsquid_dmg_whip("sk_bullsquid_dmg_whip", "0", 0, "Daño causado por ataque de cola");

//=========================================================
// Modelo
//=========================================================

#define MODEL_BULLSQUID "models/xenians/bullsquid.mdl"
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2
#define BLOOD			BLOOD_COLOR_YELLOW

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_SEECRAB = LAST_SHARED_SCHEDULE,
	SCHED_EAT
};

//=========================================================
// Tareas
//=========================================================
enum 
{
	TASK_EAT = LAST_SHARED_TASK,
};

//=========================================================
// Condiciones
//=========================================================
enum
{
	COND_SMELL_FOOD = LAST_SHARED_CONDITION,
};


//=========================================================
// Interacciones
//=========================================================
int	g_interactionBullsquidThrow		= 0;

//=========================================================
// Eventos de animaciones
//=========================================================

int AE_SQUID_MELEE_ATTACK1;
int AE_SQUID_MELEE_ATTACK2;
int AE_SQUID_RANGE_ATTACK1;

//=========================================================
// Animaciones
//=========================================================

int ACT_EAT;

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS(npc_bullsquid, CNPC_Bullsquid);

BEGIN_DATADESC(CNPC_Bullsquid)
	DEFINE_FIELD(m_fCanThreatDisplay,	FIELD_BOOLEAN),
	DEFINE_FIELD(m_flLastHurtTime,		FIELD_TIME),
	DEFINE_FIELD(m_flNextSpitTime,		FIELD_TIME),
	DEFINE_FIELD(m_flHungryTime,		FIELD_TIME),
	DEFINE_FIELD(m_nextSquidSoundTime,	FIELD_TIME),
END_DATADESC()

//=========================================================
// Spawn()
// Crear un nuevo Bullsquid.
//=========================================================
void CNPC_Bullsquid::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	SetModel(MODEL_BULLSQUID);
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
	m_iHealth			= sk_bullsquid_health.GetFloat();
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;
	
	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);
	
	m_fCanThreatDisplay	= true;
	m_flNextSpitTime	= gpGlobals->curtime;
	m_flDistTooFar		= FLT_MAX;

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Precache()
// Guardar en caché objetos necesarios.
//=========================================================
void CNPC_Bullsquid::Precache()
{
	// Modelo
	PrecacheModel(MODEL_BULLSQUID);

	// Disparo de ácido.
	UTIL_PrecacheOther("grenade_spit");
	PrecacheParticleSystem("blood_impact_yellow_01");

	// Sonidos
	PrecacheScriptSound("NPC_Bullsquid.Idle");
	PrecacheScriptSound("NPC_Bullsquid.Pain");
	PrecacheScriptSound("NPC_Bullsquid.Alert");
	PrecacheScriptSound("NPC_Bullsquid.Death");
	PrecacheScriptSound("NPC_Bullsquid.Attack1");
	PrecacheScriptSound("NPC_Bullsquid.Growl");
	PrecacheScriptSound("NPC_Bullsquid.TailWhip");
	PrecacheScriptSound("NPC_Bullsquid.Eat");

	PrecacheScriptSound("NPC_Antlion.PoisonShoot");
	PrecacheScriptSound("NPC_Antlion.PoisonBall");

	BaseClass::Precache();
}

//=========================================================
// Classify()
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_Bullsquid::Classify()
{
	return CLASS_BULLSQUID; 
}

//=========================================================
// IdleSound()
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Bullsquid::IdleSound()
{
	EmitSound("NPC_Bullsquid.Idle");
}

//=========================================================
// PainSound()
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Bullsquid::PainSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Bullsquid.Pain");
}

//=========================================================
// AlertSound()
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Bullsquid::AlertSound()
{
	// TODO: ¿Porque carajo se reproduce cada segundo? Flood...
	//EmitSound("NPC_Bullsquid.Alert");
}

//=========================================================
// DeathSound()
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Bullsquid::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Bullsquid.Death");
}

//=========================================================
// AttackSound()
// Reproducir sonido al azar de ataque.
//=========================================================
void CNPC_Bullsquid::AttackSound()
{
	EmitSound("NPC_Bullsquid.Attack1");
}

//=========================================================
// GrowlSound()
// Reproducir sonido al azar de gruñido.
//=========================================================
void CNPC_Bullsquid::GrowlSound()
{
	if (gpGlobals->curtime >= m_nextSquidSoundTime)
	{
		EmitSound("NPC_Bullsquid.Growl");
		m_nextSquidSoundTime	= gpGlobals->curtime + random->RandomInt(1.5,3.0);
	}
}

//=========================================================
// MaxYawSpeed()
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_Bullsquid::MaxYawSpeed()
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
void CNPC_Bullsquid::HandleAnimEvent(animevent_t *pEvent)
{
	// Ataque a distancia 1
	// Escupitajo de ácido.
	if (pEvent->event == AE_SQUID_RANGE_ATTACK1)
	{
		// Hay un enemigo.
		if (GetEnemy())
		{
			Vector vSpitPos;
			GetAttachment("Mouth", vSpitPos); // Debe salir de mi boca ¿no?

			Vector			vTarget = GetEnemy()->GetAbsOrigin();
			Vector			vToss;
			CBaseEntity*	pBlocker;

			// Calculos y más calculos...
			vTarget[2] += random->RandomFloat(0.0f, 32.0f);
			ThrowLimit(vSpitPos, vTarget, 300, 3, Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker);

			Vector vecToTarget = (vTarget - vSpitPos);
			VectorNormalize(vecToTarget);

			float flVelocity	= VectorNormalize(vToss);
			float flCosTheta	= DotProduct(vecToTarget, vToss);
			float flTime		= (vSpitPos-vTarget).Length2D() / (flVelocity * flCosTheta);

			// Decirle a los demás: ¡aquí escupire ácido! ¡¡quitense!!
			CSoundEnt::InsertSound(SOUND_DANGER, vTarget, (15*12), flTime, this);
			// El próximo ataque será en...
			SetNextAttack(gpGlobals->curtime + flTime + random->RandomFloat(0.5f, 2.0f));

			// Escupir 3 bolas de ácido.
			for (int i = 0; i < 3; i++)
			{
				CGrenadeSpit *pGrenade = (CGrenadeSpit*)CreateEntityByName("grenade_spit");
				pGrenade->SetAbsOrigin(vSpitPos);
				pGrenade->SetAbsAngles(vec3_angle);
				DispatchSpawn(pGrenade);
				pGrenade->SetThrower(this);
				pGrenade->SetOwnerEntity(this);

				if (i == 0)
				{
					pGrenade->SetSpitSize(SPIT_LARGE);
					pGrenade->SetAbsVelocity(vToss * flVelocity);
				}
				else
				{
					pGrenade->SetSpitSize(random->RandomInt(SPIT_SMALL, SPIT_MEDIUM));
					pGrenade->SetAbsVelocity((vToss + RandomVector(-0.035f, 0.035f)) * flVelocity);
				}

				pGrenade->SetLocalAngularVelocity(
					QAngle(
						random->RandomFloat( -100, -500 ),
						random->RandomFloat( -100, -500 ),
						random->RandomFloat( -100, -500 )
					)
				);
			}

			// Efectos especiales para mis escupitajos.
			for (int i = 0; i < 5; i++)
				DispatchParticleEffect("blood_impact_yellow_01", vSpitPos + RandomVector(-12.0f, 12.0f), RandomAngle(0, 360));
			
			// Grr!!
			AttackSound();
		}

		return;
	}

	// Ataque cuerpo a cuerpo 1
	// Ataque de cola.
	if (pEvent->event == AE_SQUID_MELEE_ATTACK1)
	{
		// Atacar
		CBaseEntity *pHurt = CheckTraceHullAttack(70, Vector(-16,-16,-16), Vector(16,16,16), sk_bullsquid_dmg_whip.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB);

		// ¿Le hice daño?
		if (pHurt) 
		{
			Vector right, up;
			AngleVectors(GetAbsAngles(), NULL, &right, &up);

			// Aventarlo por los aires.
			if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
				 pHurt->ViewPunch(QAngle(20, 0, -20));
			
			pHurt->ApplyAbsVelocityImpulse(150 * (up+2*right));
		}

		return;
	}

	// Ataque cuerpo a cuerpo 2
	// Mordida
	if (pEvent->event == AE_SQUID_MELEE_ATTACK2)
	{
		// Atacar
		CBaseEntity *pHurt = CheckTraceHullAttack(70, Vector(-16,-16,-16), Vector(16,16,16), sk_bullsquid_dmg_bite.GetFloat(), DMG_SLASH);
		
		// ¿Le hice daño?
		if (pHurt)
		{
			Vector forward, up;
			AngleVectors(GetAbsAngles(), &forward, NULL, &up);

			pHurt->ApplyAbsVelocityImpulse(100 * (up-forward));
			pHurt->SetGroundEntity(NULL);
		}

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// RangeAttack1Conditions()
// Verifica si es conveniente hacer un ataque a distancia.
//=========================================================
int CNPC_Bullsquid::RangeAttack1Conditions(float flDot, float flDist)
{
	// No creo que sea un buen momento... en un ratito más.
	if(GetNextAttack() > gpGlobals->curtime)
		return COND_NOT_FACING_ATTACK;

	// El enemigo esta muy lejos.
	if(IsMoving() && flDist >= 600)
		return COND_TOO_FAR_TO_ATTACK;

	// ¿Como voy a atacar cuando esta encima de mi cara?
	if(flDot < DOT_10DEGREE)
		return COND_NOT_FACING_ATTACK;

	// Dale con todo.
	return COND_CAN_RANGE_ATTACK1;
}

//=========================================================
// MeleeAttack1Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Ataque de cola.
//=========================================================
int CNPC_Bullsquid::MeleeAttack1Conditions(float flDot, float flDist)
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
int CNPC_Bullsquid::MeleeAttack2Conditions(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7 && !HasCondition(COND_CAN_MELEE_ATTACK1))
		 return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// FValidateHintType()
//
//=========================================================
bool CNPC_Bullsquid::FValidateHintType(CAI_Hint *pHint)
{
	if (pHint->HintType() == HINT_HL1_WORLD_HUMAN_BLOOD || pHint->HintType() == HINT_NOT_USED_WORLD_HUMAN_BLOOD)
		 return true;

	return false;
}

//=========================================================
// RemoveIgnoredConditions()
//
//=========================================================
void CNPC_Bullsquid::RemoveIgnoredConditions( void )
{
	if (m_flHungryTime > gpGlobals->curtime)
		 ClearCondition(COND_SMELL_FOOD);
}

//=========================================================
// IRelationType()
//
//=========================================================
Disposition_t CNPC_Bullsquid::IRelationType(CBaseEntity *pTarget)
{
	if (gpGlobals->curtime - m_flLastHurtTime < 5 && FClassnameIs(pTarget, "npc_headcrab"))
		return D_NU;
	
	return BaseClass::IRelationType(pTarget);
}

//=========================================================
// OnTakeDamage_Alive()
// 
//=========================================================
int CNPC_Bullsquid::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	if (!FClassnameIs(inputInfo.GetAttacker(), "npc_headcrab"))
		m_flLastHurtTime = gpGlobals->curtime;

	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// GetSoundInterests()
// Devuelve los bits con los valores de sonidos interesantes
// para el NPC.
// Nota: Investigar si esto es necesario/funcional.
//=========================================================
int CNPC_Bullsquid::GetSoundInterests()
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
		    SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_PLAYER;
}

//=========================================================
// OnListened()
// Ejecutar acciones al oir/oler un sonido.
//=========================================================
void CNPC_Bullsquid::OnListened()
{
	AISoundIter_t iter;	
	CSound *pCurrentSound;

	static int conditionsToClear[] = 
	{
		COND_SMELL_FOOD,
	};

	ClearConditions(conditionsToClear, ARRAYSIZE(conditionsToClear));

	// Obtener los sonidos más cercanos.
	pCurrentSound = GetSenses()->GetFirstHeardSound(&iter);
	
	while (pCurrentSound)
	{
		// ¿No es sonido? Es un olor en un sonido ¡yay Valve!
		if (!pCurrentSound->FIsSound())
		{
			// ¿Es un olor a carne o cadáver? ¡Yumi!
			if (pCurrentSound->m_iType & (SOUND_MEAT | SOUND_CARCASS))
				SetCondition(COND_SMELL_FOOD);
		}

		pCurrentSound = GetSenses()->GetNextHeardSound(&iter);
	}

	BaseClass::OnListened();
}

//=========================================================
// RunAI()
// 
//=========================================================
void CNPC_Bullsquid::RunAI( void )
{
	BaseClass::RunAI();

	if (GetEnemy() != NULL && GetActivity() == ACT_RUN)
	{
		if ((GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D() < SQUID_SPRINT_DIST)
			m_flPlaybackRate = 1.25;
	}
}

//=========================================================
// SelectSchedule()
// Seleccionar una tarea programada dependiendo del estado
//=========================================================
int CNPC_Bullsquid::SelectSchedule()
{
	switch	(m_NPCState)
	{
		case NPC_STATE_ALERT:
		{
			break;
		}

		case NPC_STATE_COMBAT:
		{
			if (HasCondition(COND_SMELL_FOOD))
				return SCHED_EAT;

			if (HasCondition(COND_ENEMY_DEAD))
				return BaseClass::SelectSchedule();

			if (HasCondition(COND_NEW_ENEMY))
			{
				// this means squid sees a headcrab!
				if (m_fCanThreatDisplay && IRelationType(GetEnemy()) == D_HT && FClassnameIs(GetEnemy(), "npc_headcrab"))
				{		
					//m_fCanThreatDisplay = false;
					return SCHED_SEECRAB;
				}
				else
					return SCHED_WAKE_ANGRY;
			}

			if (HasCondition(COND_CAN_RANGE_ATTACK1))
				return SCHED_RANGE_ATTACK1;

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
// FInViewCone()
// returns true is the passed vector is in the caller's forward view cone.
// The dot product is performed in 2d, making the view cone infinitely tall. 
//=========================================================
bool CNPC_Bullsquid::FInViewCone(Vector pOrigin)
{
	Vector los	= (pOrigin - GetAbsOrigin());
	los.z		= 0;

	VectorNormalize(los);
	Vector facingDir = EyeDirection2D();

	float flDot = DotProduct(los, facingDir);

	if (flDot > m_flFieldOfView)
		return true;

	return false;
}

//=========================================================
// StartTask()
// Realiza los calculos necesarios al iniciar una tarea.
//=========================================================
void CNPC_Bullsquid::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
		// Mordida
		case TASK_MELEE_ATTACK2:
		{
			if (GetEnemy())
			{
				GrowlSound();

				m_flLastAttackTime = gpGlobals->curtime;
				BaseClass::StartTask(pTask);
			}

			break;
		}

		// Comer ¡yumi!
		case TASK_EAT:
		{
			m_flHungryTime = gpGlobals->curtime + pTask->flTaskData;

			TaskComplete();
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
void CNPC_Bullsquid::RunTask(const Task_t *pTask)
{
	BaseClass::RunTask(pTask);
}

//=========================================================
// SelectIdealState()
// 
//=========================================================
NPC_STATE CNPC_Bullsquid::SelectIdealState( void )
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

AI_BEGIN_CUSTOM_NPC(npc_bullsquid, CNPC_Bullsquid)

	DECLARE_TASK(TASK_EAT)

	DECLARE_CONDITION(COND_SMELL_FOOD)

	DECLARE_ANIMEVENT(AE_SQUID_MELEE_ATTACK1);
	DECLARE_ANIMEVENT(AE_SQUID_MELEE_ATTACK2);
	DECLARE_ANIMEVENT(AE_SQUID_RANGE_ATTACK1);

	DECLARE_ACTIVITY(ACT_EAT)

	DECLARE_INTERACTION(g_interactionBullsquidThrow)

	//=========================================================
	// > ¡He visto un Headcrab!
	//=========================================================

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

	//=========================================================
	// > ¡A comer!
	//=========================================================

	DEFINE_SCHEDULE
	(
		SCHED_EAT,
	
		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_EAT							10"
		"		TASK_STORE_LASTPOSITION				0"
		"		TASK_GET_PATH_TO_BESTSCENT			0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_EAT"
		"		TASK_EAT							50"
		"		TASK_GET_PATH_TO_LASTPOSITION		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CLEAR_LASTPOSITION				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
	)
	
AI_END_CUSTOM_NPC()