//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Combine Zombie... Zombie Combine... its like a... Zombine... get it?
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "ai_route.h"
#include "ai_squad.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "ai_task.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "npc_BaseZombie.h"
#include "movevars_shared.h"
#include "IEffects.h"
#include "props.h"
#include "physics_npc_solver.h"
#include "hl2_player.h"
#include "hl2_gamerules.h"

#include "basecombatweapon.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"

#include "ai_interactions.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar sk_zombie_soldier_health("sk_zombie_soldier_health", "2", 0, "Salud del zombi soldado.");

//=========================================================
// Configuración del NPC
//=========================================================

#define BLOOD			BLOOD_COLOR_RED						// Color de la sangre
#define FOV				0.2									// Campo de visión

#define MODEL			"models/zombie/zombie_soldier.mdl"	// Modelo del NPC
#define MODEL_HEADCRAB	"models/headcrabclassic.mdl"		// Modelo su Headcrab
#define MODEL_LEGS		"models/zombie/zombie_soldier_legs.mdl"
#define MODEL_TORSO		"models/zombie/zombie_soldier_torso.mdl"

#define CLASS_NAME_HEADCRAB	"npc_headcrab"

extern bool IsAlyxInDarknessMode();
float g_flZombineGrenadeTimes = 0;

//=========================================================
// Información
//=========================================================

#define MIN_SPRINT_TIME 3.5f
#define MAX_SPRINT_TIME 5.5f

#define MIN_SPRINT_DISTANCE 64.0f
#define MAX_SPRINT_DISTANCE 1024.0f

#define SPRINT_CHANCE_VALUE 10
#define SPRINT_CHANCE_VALUE_DARKNESS 50

#define GRENADE_PULL_MAX_DISTANCE 256.0f

#define ZOMBINE_MAX_GRENADES 1

//=========================================================
// Escuadrones
//=========================================================

enum
{	
	SQUAD_SLOT_ZOMBINE_SPRINT1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_ZOMBINE_SPRINT2,
};

//=========================================================
// Actividades
//=========================================================

int ACT_ZOMBINE_GRENADE_PULL;
int ACT_ZOMBINE_GRENADE_WALK;
int ACT_ZOMBINE_GRENADE_RUN;
int ACT_ZOMBINE_GRENADE_IDLE;
int ACT_ZOMBINE_ATTACK_FAST;
int ACT_ZOMBINE_GRENADE_FLINCH_BACK;
int ACT_ZOMBINE_GRENADE_FLINCH_FRONT;
int ACT_ZOMBINE_GRENADE_FLINCH_WEST;
int ACT_ZOMBINE_GRENADE_FLINCH_EAST;

int AE_ZOMBINE_PULLPIN;

//=========================================================
// Clase del Zombi soldado
//=========================================================

class CNPC_Zombine : public CAI_BlendingHost<CNPC_BaseZombie>, public CDefaultPlayerPickupVPhysics
{
	DECLARE_DATADESC();
	DECLARE_CLASS(CNPC_Zombine, CAI_BlendingHost<CNPC_BaseZombie>);

public:

	void Spawn();
	void Precache();

	void SetZombieModel();

	virtual void	PrescheduleThink();
	virtual int		SelectSchedule();
	virtual void	BuildScheduleTestBits();

	virtual void HandleAnimEvent(animevent_t *pEvent);

	virtual const char *GetLegsModel();
	virtual const char *GetTorsoModel();
	virtual const char *GetHeadcrabClassname();
	virtual const char *GetHeadcrabModel();

	virtual void PainSound(const CTakeDamageInfo &info);
	virtual void DeathSound(const CTakeDamageInfo &info);
	virtual void AlertSound();
	virtual void IdleSound();
	virtual void AttackSound();
	virtual void AttackHitSound();
	virtual void AttackMissSound();
	virtual void FootstepSound(bool fRightFoot);
	virtual void FootscuffSound(bool fRightFoot);
	virtual void MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize);

	virtual void Event_Killed(const CTakeDamageInfo &info);
	virtual void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
	virtual void RunTask(const Task_t *pTask);

	virtual int  MeleeAttack1Conditions (float flDot, float flDist );

	virtual bool ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold);

	virtual void OnScheduleChange();
	virtual bool CanRunAScriptedNPCInteraction(bool bForced);

	void GatherGrenadeConditions();

	virtual Activity NPC_TranslateActivity(Activity baseAct);

	const char *GetMoanSound(int nSound);

	bool AllowedToSprint();
	void Sprint(bool bMadSprint = false);
	void StopSprint();

	void DropGrenade(Vector vDir);

	bool IsSprinting() { return m_flSprintTime > gpGlobals->curtime; }
	bool HasGrenade() { return m_hGrenade != NULL; }

	int TranslateSchedule(int scheduleType);

	void InputStartSprint (inputdata_t &inputdata );
	void InputPullGrenade (inputdata_t &inputdata );

	virtual CBaseEntity *OnFailedPhysGunPickup (Vector vPhysgunPos);

	//Called when we want to let go of a grenade and let the physcannon pick it up.
	void ReleaseGrenade(Vector vPhysgunPos);

	virtual bool HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt);

	enum
	{
		COND_ZOMBINE_GRENADE = LAST_BASE_ZOMBIE_CONDITION,
	};

	enum
	{
		SCHED_ZOMBINE_PULL_GRENADE = LAST_BASE_ZOMBIE_SCHEDULE,
	};

public:
	DEFINE_CUSTOM_AI;

private:

	float	m_flSprintTime;
	float	m_flSprintRestTime;

	float	m_flSuperFastAttackTime;
	float   m_flGrenadePullTime;
	
	int		m_iGrenadeCount;

	EHANDLE	m_hGrenade;

protected:
	static const char *pMoanSounds[];

};

LINK_ENTITY_TO_CLASS( npc_zombine, CNPC_Zombine );

//=========================================================
// Definición de datos.
//=========================================================

BEGIN_DATADESC( CNPC_Zombine )
	DEFINE_FIELD( m_flSprintTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flSprintRestTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flSuperFastAttackTime,	FIELD_TIME ),
	DEFINE_FIELD( m_hGrenade,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_flGrenadePullTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iGrenadeCount,			FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"StartSprint", InputStartSprint ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"PullGrenade", InputPullGrenade ),
END_DATADESC()

//=========================================================
// Sonidos de los gemidos del zombi
//=========================================================

const char *CNPC_Zombine::pMoanSounds[] =
{
	"ATV_engine_null",
};

//=========================================================
// Crear un nuevo zombi.
//=========================================================
void CNPC_Zombine::Spawn()
{
	ConVarRef zombie_attach_headcrab("zombie_attach_headcrab");
	Precache();

	IsTorso		= false; // ¿Es el puro torso?
	IsHeadless	= zombie_attach_headcrab.GetBool();
	
	// Color de la sangre.
	SetBloodColor(BLOOD);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth			= sk_zombie_soldier_health.GetFloat();
	m_flFieldOfView		= FOV;

	CapabilitiesClear();
	BaseClass::Spawn();

	// Correr
	m_flSprintTime		= 0.0f;
	m_flSprintRestTime	= 0.0f;

	// La próxima vez que "gemire"
	NextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );

	// Granadas
	g_flZombineGrenadeTimes = gpGlobals->curtime;
	m_flGrenadePullTime		= gpGlobals->curtime;

	// Número máximo de granadas.
	m_iGrenadeCount = ZOMBINE_MAX_GRENADES;
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CNPC_Zombine::Precache()
{
	BaseClass::Precache();

	// Modelo
	PrecacheModel(MODEL);

	// Sonidos
	PrecacheScriptSound( "Zombie.FootstepRight" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombine.ScuffRight" );
	PrecacheScriptSound( "Zombine.ScuffLeft" );
	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
	PrecacheScriptSound( "Zombine.Pain" );
	PrecacheScriptSound( "Zombine.Die" );
	PrecacheScriptSound( "Zombine.Alert" );
	PrecacheScriptSound( "Zombine.Idle" );
	PrecacheScriptSound( "Zombine.ReadyGrenade" );

	PrecacheScriptSound( "ATV_engine_null" );
	PrecacheScriptSound( "Zombine.Charge" );
	PrecacheScriptSound( "Zombie.Attack" );
}

//=========================================================
// Establece el modelo de este zombi.
//=========================================================
void CNPC_Zombine::SetZombieModel()
{
	// Establecemos el modelo y el tamaño.
	SetModel(MODEL);
	SetHullType(HULL_HUMAN);

	// Headcrab sobre tu cara.
	SetBodygroup(ZOMBIE_BODYGROUP_HEADCRAB, !IsHeadless);

	SetHullSizeNormal(true);
	SetDefaultEyeOffset();
	SetActivity(ACT_IDLE);
}

//=========================================================
// Pre-Pensamiento: Bucle de ejecución de tareas.
//=========================================================
void CNPC_Zombine::PrescheduleThink()
{
	// Verificamos si es conveniente sacar una granada.
	GatherGrenadeConditions();

	// Al parecer me toca gemir.
	if ( gpGlobals->curtime > NextMoanSound )
	{
		// ¿Puedo "gemir"?
		if( CanPlayMoanSound() )
		{
			// Gemir y decidir cuando volvere a gemir.
			IdleSound();
			NextMoanSound = gpGlobals->curtime + random->RandomFloat(10.0, 15.0);
		}
		else
			NextMoanSound = gpGlobals->curtime + random->RandomFloat(2.5, 5.0);
	}

	// Al parecer tengo una granada.
	if ( HasGrenade () )
	{
		// ¡¡Ateción!! Tengo una granada y no pienso soltarla...
		CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin() + GetSmoothedVelocity() * 0.5f , 256, 0.1, this, SOUNDENT_CHANNEL_ZOMBINE_GRENADE);

		// Si estoy corriendo
		// y mi enemigo es una persona vital para el jugador
		if( IsSprinting() && GetEnemy() && GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL && HasCondition(COND_SEE_ENEMY) )
		{
			// Además estoy cerca de el... dejar de correr.
			if( GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin()) < Square(144) )
				StopSprint();
		}
	}

	BaseClass::PrescheduleThink();
}

//=========================================================
// Cuando una tarea cambia.
//=========================================================
void CNPC_Zombine::OnScheduleChange()
{
	// Puedo atacar y además estoy corriendo.
	if ( HasCondition(COND_CAN_MELEE_ATTACK1) && IsSprinting() )
		m_flSuperFastAttackTime = gpGlobals->curtime + 1.0f;

	BaseClass::OnScheduleChange();
}

//=========================================================
// ¿Puedo ejecutar una interacción programada?
//=========================================================
bool CNPC_Zombine::CanRunAScriptedNPCInteraction(bool bForced)
{
	// Teniendo una granada :NO:
	if ( HasGrenade() )
		return false;

	return BaseClass::CanRunAScriptedNPCInteraction( bForced );
}

//=========================================================
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CNPC_Zombine::SelectSchedule()
{
	// ¿Estoy muerto?
	if ( GetHealth() <= 0 )
		return BaseClass::SelectSchedule();

	// Tengo una granada, soltarla.
	if ( HasCondition(COND_ZOMBINE_GRENADE) )
	{
		ClearCondition(COND_ZOMBINE_GRENADE);		
		return SCHED_ZOMBINE_PULL_GRENADE;
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
//=========================================================
void CNPC_Zombine::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();
	SetCustomInterruptCondition(COND_ZOMBINE_GRENADE);
}

//=========================================================
// Convierte actividades que este zombi no posee a
// actividades válidas.
//=========================================================
Activity CNPC_Zombine::NPC_TranslateActivity( Activity baseAct )
{
	// Ataque cuerpo a cuerpo.
	if ( baseAct == ACT_MELEE_ATTACK1 )
	{
		if ( m_flSuperFastAttackTime > gpGlobals->curtime || HasGrenade() )
			return (Activity)ACT_ZOMBINE_ATTACK_FAST;
	}

	// Libre, sin hacer nada.
	if ( baseAct == ACT_IDLE )
	{
		if ( HasGrenade() )
			return (Activity)ACT_ZOMBINE_GRENADE_IDLE;
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
//=========================================================
int CNPC_Zombine::MeleeAttack1Conditions(float flDot, float flDist)
{
	int iBase = BaseClass::MeleeAttack1Conditions(flDot, flDist);

	// Al parecer tengo una granada.
	if( HasGrenade() )
	{
		// Dejar de correr si esta lo suficientemente cerca para atacar.
		// Esto le da tiempo a los demás NPC's para correr de ti.
		if ( iBase == COND_CAN_MELEE_ATTACK1 )
			StopSprint();
	}

	return iBase;
}

//=========================================================
// Verifica si es conveniente sacar una granada.
//=========================================================
void CNPC_Zombine::GatherGrenadeConditions()
{
	// ¡Nos hemos quedado sin granadas!
	if ( m_iGrenadeCount <= 0 )
		return;

	// No sacar una granada en corto tiempo.
	if ( g_flZombineGrenadeTimes > gpGlobals->curtime )
		return;

	if ( m_flGrenadePullTime > gpGlobals->curtime )
		return;

	if ( m_flSuperFastAttackTime >= gpGlobals->curtime )
		return;
	
	// 
	if ( HasGrenade() )
		return;

	// No hay un enemigo cerca.
	if ( GetEnemy() == NULL )
		return;

	// No veo a mi enemigo.
	if ( !FVisible(GetEnemy()) )
		return;

	// Estoy corriendo.
	if ( IsSprinting() )
		return;

	// Estoy ¡quemandome!
	if ( IsOnFire() )
		return;
	
	if ( IsRunningDynamicInteraction() == true )
		return;

	if ( m_ActBusyBehavior.IsActive() )
		return;

	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	// El jugador es válido y además me esta viendo.
	if ( pPlayer && pPlayer->FVisible(this) )
	{
		// Calculamos la distancia hacia el jugador.
		float flLengthToPlayer	= (pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length();
		float flLengthToEnemy	= flLengthToPlayer;

		// Al parecer el jugador no es mi enemigo.
		if ( pPlayer != GetEnemy() )
			flLengthToEnemy = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();

		// Distancia aceptable.
		if ( flLengthToPlayer <= GRENADE_PULL_MAX_DISTANCE && flLengthToEnemy <= GRENADE_PULL_MAX_DISTANCE )
		{
			float flPullChance	= 1.0f - ( flLengthToEnemy / GRENADE_PULL_MAX_DISTANCE );
			m_flGrenadePullTime = gpGlobals->curtime + 0.5f;

			// ¡Es hora de sacar una granada!
			if ( flPullChance >= random->RandomFloat( 0.0f, 1.0f ) )
			{
				g_flZombineGrenadeTimes = gpGlobals->curtime + 10.0f;
				SetCondition(COND_ZOMBINE_GRENADE);
			}
		}
	}
}

//=========================================================
//=========================================================
int CNPC_Zombine::TranslateSchedule(int scheduleType) 
{
	if ( scheduleType == SCHED_IDLE_STAND || scheduleType == SCHED_ALERT_STAND )
		return SCHED_ZOMBIE_WANDER_MEDIUM;

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// Soltar una granada.
//=========================================================
void CNPC_Zombine::DropGrenade( Vector vDir )
{
	// No tengo ninguna granada.
	if ( m_hGrenade == NULL )
		 return;

	// Estas sola amiga granada.
	m_hGrenade->SetParent(NULL);
	m_hGrenade->SetOwnerEntity(NULL);

	Vector vGunPos;
	QAngle angles;

	// Obtenemos el acoplamiento donde estaba la granada.
	GetAttachment("grenade_attachment", vGunPos, angles);

	IPhysicsObject *pPhysObj = m_hGrenade->VPhysicsGetObject();

	// La granada no tenia propiedades de la física, ajustarlas.
	if ( pPhysObj == NULL )
	{
		m_hGrenade->SetMoveType(MOVETYPE_VPHYSICS);
		m_hGrenade->SetSolid(SOLID_VPHYSICS);
		m_hGrenade->SetCollisionGroup(COLLISION_GROUP_WEAPON);

		m_hGrenade->CreateVPhysics();
	}

	// ¡Vive por ti misma!
	if ( pPhysObj )
	{
		pPhysObj->Wake();
		pPhysObj->SetPosition(vGunPos, angles, true);
		pPhysObj->ApplyForceCenter(vDir * 0.2f);

		pPhysObj->RecheckCollisionFilter();
	}

	// Ya no tienes granada zombi.
	m_hGrenade = NULL;
}

//=========================================================
// Muerte del NPC.
//=========================================================
void CNPC_Zombine::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);

	// Si tenia una granada, soltarla.
	if ( HasGrenade() )
		DropGrenade(vec3_origin);
}

//=========================================================
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//=========================================================
bool CNPC_Zombine::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sourceEnt )
{
	// Un barnacle nos esta comiendo...
	if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		// Si tenia una granada, soltarla.
		if ( HasGrenade() )
			DropGrenade(vec3_origin);
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//=========================================================
// Rastrea el ataque recibido.
//=========================================================
void CNPC_Zombine::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	BaseClass::TraceAttack(info, vecDir, ptr);

	// Mi atacante es un NPC.
	if ( info.GetAttacker() && info.GetAttacker()->IsNPC() )
		return;

	// El tipo de daño es balas o un golpe.
	if ( info.GetDamageType() & (DMG_BULLET | DMG_CLUB) )
	{
		// Justo en mi brazo izquierdo.
		if ( ptr->hitgroup == HITGROUP_LEFTARM )
		{
			// Tengo una granada.
			if ( HasGrenade() )
			{
				// Soltar la granada y dejar de correr.
				DropGrenade(info.GetDamageForce());
				StopSprint();
			}
		}
	}
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_Zombine::HandleAnimEvent( animevent_t *pEvent )
{
	// Crear una granada.
	if ( pEvent->event == AE_ZOMBINE_PULLPIN )
	{
		Vector vecStart;
		QAngle angles;

		// Obtenemos el acoplamiento donde estaba la granada.
		GetAttachment("grenade_attachment", vecStart, angles);

		// Creamos la granada.
		CBaseGrenade *pGrenade = Fraggrenade_Create(vecStart, vec3_angle, vec3_origin, AngularImpulse( 0, 0, 0 ), this, 3.5f, true);

		// La creación no tuvo problemas.
		if ( pGrenade )
		{
			// Move physobject to shadow
			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();

			if ( pPhysicsObject )
			{
				pGrenade->VPhysicsDestroyObject();

				int iAttachment = LookupAttachment("grenade_attachment");

				// Sin fisicas.
				pGrenade->SetMoveType(MOVETYPE_NONE);
				pGrenade->SetSolid(SOLID_NONE);
				pGrenade->SetCollisionGroup(COLLISION_GROUP_DEBRIS);

				// Lugar de origen.
				pGrenade->SetAbsOrigin(vecStart);
				pGrenade->SetAbsAngles(angles);

				// Yo soy tu padre.
				pGrenade->SetParent(this, iAttachment);
				// 150 de daño.
				pGrenade->SetDamage(150.0f);

				m_hGrenade = pGrenade;				
				EmitSound("Zombine.ReadyGrenade");

				// Tell player allies nearby to regard me!
				CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
				CAI_BaseNPC *pNPC;

				for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
				{
					pNPC = ppAIs[i];

					if( pNPC->Classify() == CLASS_PLAYER_ALLY || pNPC->Classify() == CLASS_PLAYER_ALLY_VITAL && pNPC->FVisible(this) )
					{
						int priority;
						Disposition_t disposition;

						priority	= pNPC->IRelationPriority(this);
						disposition = pNPC->IRelationType(this);

						pNPC->AddEntityRelationship(this, disposition, priority + 1);
					}
				}
			}

			m_iGrenadeCount--;
		}

		return;
	}

	// Notificar de ataque.
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		if ( HasGrenade() )
			return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// ¿Estoy permitido correr?
//=========================================================
bool CNPC_Zombine::AllowedToSprint()
{
	// Estoy quemandome.
	if ( IsOnFire() )
		return false;
	
	// Ya estoy corriendo.
	if ( IsSprinting() )
		return false;

	int iChance = SPRINT_CHANCE_VALUE;

	CHL2_Player *pPlayer = dynamic_cast <CHL2_Player*> (AI_GetSinglePlayer());

	if ( pPlayer )
	{
		// Estamos en la oscuridad.
		if ( HL2GameRules()->IsAlyxInDarknessMode() && pPlayer->FlashlightIsOn() == false )
			iChance = SPRINT_CHANCE_VALUE_DARKNESS;

		// Más posibilidades si el usuario no nos esta viendo.
		if ( pPlayer->FInViewCone(this) == false )
			iChance *= 2;
	}

	// Muchas más posibilidades si tenemos una granada.
	if ( HasGrenade() ) 
		iChance *= 4;

	// Below 25% health they'll always sprint
	if ( ( GetHealth() > GetMaxHealth() * 0.5f ) )
	{
		if ( IsStrategySlotRangeOccupied(SQUAD_SLOT_ZOMBINE_SPRINT1, SQUAD_SLOT_ZOMBINE_SPRINT2) == true )
			return false;
		
		if ( random->RandomInt( 0, 100 ) > iChance )
			return false;
		
		if ( m_flSprintRestTime > gpGlobals->curtime )
			return false;
	}

	float flLength = ( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() ).Length();

	if ( flLength > MAX_SPRINT_DISTANCE )
		return false;

	return true;
}

//=========================================================
// Parar de correr.
//=========================================================
void CNPC_Zombine::StopSprint()
{
	GetNavigator()->SetMovementActivity(ACT_WALK);

	m_flSprintTime		= gpGlobals->curtime;
	m_flSprintRestTime	= m_flSprintTime + random->RandomFloat(2.5f, 5.0f);
}

//=========================================================
// ¡Correr!
//=========================================================
void CNPC_Zombine::Sprint(bool bMadSprint)
{
	// Ya estoy corriendo.
	if ( IsSprinting() )
		return;

	OccupyStrategySlotRange(SQUAD_SLOT_ZOMBINE_SPRINT1, SQUAD_SLOT_ZOMBINE_SPRINT2);
	GetNavigator()->SetMovementActivity(ACT_RUN);

	float flSprintTime = random->RandomFloat(MIN_SPRINT_TIME, MAX_SPRINT_TIME);

	// Corremos hacia el enemigo antes de que explote mi granada.
	if ( HasGrenade() || bMadSprint == true )
		flSprintTime = 9999;

	m_flSprintTime		= gpGlobals->curtime + flSprintTime;
	m_flSprintRestTime	= m_flSprintTime + random->RandomFloat( 2.5f, 5.0f );

	EmitSound("Zombine.Charge");
}

//=========================================================
// Ejecuta una tarea.
//=========================================================
void CNPC_Zombine::RunTask(const Task_t *pTask)
{
	switch ( pTask->iTask )
	{
		case TASK_WAIT_FOR_MOVEMENT_STEP:
		case TASK_WAIT_FOR_MOVEMENT:
		{
			BaseClass::RunTask( pTask );

			if ( IsOnFire() && IsSprinting() )
				StopSprint();

			// Only do this if I have an enemy
			if ( GetEnemy() )
			{
				if ( AllowedToSprint() )
				{
					Sprint(( GetHealth() <= GetMaxHealth() * 0.5f ));
					return;
				}

				if ( HasGrenade() )
				{
					if ( IsSprinting() )
						GetNavigator()->SetMovementActivity((Activity)ACT_ZOMBINE_GRENADE_RUN);
					else
						GetNavigator()->SetMovementActivity((Activity)ACT_ZOMBINE_GRENADE_WALK);

					return;
				}

				if ( GetNavigator()->GetMovementActivity() != ACT_WALK )
				{
					if ( !IsSprinting() )
						GetNavigator()->SetMovementActivity(ACT_WALK);
				}
			}
			else
				GetNavigator()->SetMovementActivity(ACT_WALK);
		
			break;
		}
		default:
		{
			BaseClass::RunTask(pTask);
			break;
		}
	}
}

//=========================================================
// INPUT: Empezar a correr.
//=========================================================
void CNPC_Zombine::InputStartSprint(inputdata_t &inputdata)
{
	Sprint();
}

//=========================================================
// INPUT: Tirar una granada.
//=========================================================
void CNPC_Zombine::InputPullGrenade(inputdata_t &inputdata)
{
	g_flZombineGrenadeTimes = gpGlobals->curtime + 5.0f;
	SetCondition(COND_ZOMBINE_GRENADE);
}

//=========================================================
// Obtener los sonidos de "gemido" para esta clase de zombi.
//=========================================================
const char *CNPC_Zombine::GetMoanSound(int nSound)
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//=========================================================
// Reproducir los sonidos de mis pasos.
//=========================================================
void CNPC_Zombine::FootstepSound(bool fRightFoot)
{
	if( fRightFoot )
		EmitSound("Zombie.FootstepRight");
	else
		EmitSound("Zombie.FootstepLeft");
}

//=========================================================
// ¿Partir el zombi a la mitad?
//=========================================================
bool CNPC_Zombine::ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold)
{
	return false;
}

//=========================================================
// Reproducir los sonidos del pie arrastrandose.
//=========================================================
void CNPC_Zombine::FootscuffSound(bool fRightFoot)
{
	if ( fRightFoot )
		EmitSound("Zombine.ScuffRight");
	else
		EmitSound("Zombine.ScuffLeft");
}

//=========================================================
// Reproducir sonido al azar de ataque.
//=========================================================
void CNPC_Zombine::AttackHitSound()
{
	EmitSound("Zombie.AttackHit");
}

//=========================================================
// Reproducir sonido al azar de ataque fallido.
//=========================================================
void CNPC_Zombine::AttackMissSound()
{
	EmitSound("Zombie.AttackMiss");
}

//=========================================================
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Zombine::PainSound( const CTakeDamageInfo &info )
{
	// Cuando el zombi esta en llamas sufre daños constantemente.
	// Para evitar un bucle de sonidos, no reproducir cuando este en llamas.
	if ( IsOnFire() )
		return;

	EmitSound("Zombine.Pain");
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Zombine::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound("Zombine.Die");
}

//=========================================================
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Zombine::AlertSound()
{
	// Huelo algo... ¿estas ahí? quiero tu carne...
	EmitSound("Zombine.Alert");

	// Retrasar el sonido de "gemido".
	NextMoanSound += random->RandomFloat(2.0, 4.0);
}

//=========================================================
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Zombine::IdleSound()
{
	// Evitamos sonidos constantes.
	if ( GetState() == NPC_STATE_IDLE && random->RandomFloat( 0, 1 ) == 0 )
		return;

	// ¿Estamos "durmiendo"? ¡No hacer ruido!
	if ( IsSlumped() )
		return;

	EmitSound("Zombine.Idle");
	MakeAISpookySound(360.0f);
}

//=========================================================
// Reproducir sonido al azar de ataque.
//=========================================================
void CNPC_Zombine::AttackSound()
{
	
}

//=========================================================
// Nombre de la clase de NPC del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//=========================================================
const char *CNPC_Zombine::GetHeadcrabClassname()
{
	return CLASS_NAME_HEADCRAB;
}

//=========================================================
// Ubicación del modelo del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//=========================================================
const char *CNPC_Zombine::GetHeadcrabModel()
{
	return MODEL_HEADCRAB;
}

//=========================================================
// Ubicación del modelo de las piernas de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//=========================================================
const char *CNPC_Zombine::GetLegsModel()
{
	return MODEL_LEGS;
}

//=========================================================
// Ubicación del modelo del torso de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//=========================================================
const char *CNPC_Zombine::GetTorsoModel()
{
	return MODEL_TORSO;
}

//=========================================================
// Reproducir sonido de gemido.
//=========================================================
void CNPC_Zombine::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if ( IsOnFire() )
		BaseClass::MoanSound( pEnvelope, iEnvelopeSize );
}

//=========================================================
// Soltar una granada.
//=========================================================
void CNPC_Zombine::ReleaseGrenade(Vector vPhysgunPos)
{
	// No tengo ninguna granada.
	if ( HasGrenade() == false )
		return;

	Vector vDir = vPhysgunPos - m_hGrenade->GetAbsOrigin();
	VectorNormalize(vDir);

	Activity aActivity;

	Vector vForward, vRight;
	GetVectors(&vForward, &vRight, NULL);

	float flDotForward	= DotProduct(vForward, vDir);
	float flDotRight	= DotProduct(vRight, vDir);

	bool bNegativeForward	= false;
	bool bNegativeRight		= false;

	if ( flDotForward < 0.0f )
	{
		bNegativeForward = true;
		flDotForward	 = flDotForward * -1;
	}

	if ( flDotRight < 0.0f )
	{
		bNegativeRight	= true;
		flDotRight		= flDotRight * -1;
	}

	if ( flDotRight > flDotForward )
	{
		if ( bNegativeRight )
			aActivity = (Activity)ACT_ZOMBINE_GRENADE_FLINCH_WEST;
		else 
			aActivity = (Activity)ACT_ZOMBINE_GRENADE_FLINCH_EAST;
	}
	else
	{
		if ( bNegativeForward )
			aActivity = (Activity)ACT_ZOMBINE_GRENADE_FLINCH_BACK;
		else 
			aActivity = (Activity)ACT_ZOMBINE_GRENADE_FLINCH_FRONT;
	}

	AddGesture(aActivity);
	DropGrenade(vec3_origin);

	if ( IsSprinting() )
		StopSprint();
	else
		Sprint();
}

//=========================================================
// El jugador ha fallado al agarrar la granada.
//=========================================================
CBaseEntity *CNPC_Zombine::OnFailedPhysGunPickup( Vector vPhysgunPos )
{
	CBaseEntity *pGrenade = m_hGrenade;
	ReleaseGrenade(vPhysgunPos);

	return pGrenade;
}

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

AI_BEGIN_CUSTOM_NPC( npc_zombine, CNPC_Zombine )

	//Squad slots
	DECLARE_SQUADSLOT( SQUAD_SLOT_ZOMBINE_SPRINT1 )
	DECLARE_SQUADSLOT( SQUAD_SLOT_ZOMBINE_SPRINT2 )

	DECLARE_CONDITION( COND_ZOMBINE_GRENADE )

	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_PULL )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_WALK )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_RUN )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_IDLE )
	DECLARE_ACTIVITY( ACT_ZOMBINE_ATTACK_FAST )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_FLINCH_BACK )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_FLINCH_FRONT )
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_FLINCH_WEST)
	DECLARE_ACTIVITY( ACT_ZOMBINE_GRENADE_FLINCH_EAST )

	DECLARE_ANIMEVENT( AE_ZOMBINE_PULLPIN )


	DEFINE_SCHEDULE
	(
	SCHED_ZOMBINE_PULL_GRENADE,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ZOMBINE_GRENADE_PULL"


	"	Interrupts"

	)

AI_END_CUSTOM_NPC()