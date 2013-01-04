//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Zombi clasico: Lento y con solo ataques de cuerpo a cuerpo.
//
//=============================================================================//

#include "cbase.h"
#include "doors.h"

#include "simtimer.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "gib.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------------
// Definición de variables de configuración.
//----------------------------------------------------

ConVar sk_zombie_health("sk_zombie_health", "0", 0, "Salud del zombi clasico.");

//----------------------------------------------------
// InSource - Modelos
//----------------------------------------------------

#define MODEL_HEADCRAB	"models/headcrabclassic.mdl"
#define MODEL_LEGS		"models/zombie/classic_legs.mdl"
#define MODEL_TORSO		"models/zombie/classic_torso.mdl"
#define MODEL_ZOMBIE	"models/zombie/classic.mdl"

//----------------------------------------------------
// Información
//----------------------------------------------------

#define ZOMBIE_SQUASH_MASS	300.0f

//----------------------------------------------------
// Sonidos y su volumen.
//----------------------------------------------------

envelopePoint_t envZombieMoanVolumeFast[] =
{
	{	7.0f, 7.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envZombieMoanVolume[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanVolumeLong[] =
{
	{	1.0f, 1.0f,
		0.3f, 0.5f,
	},
	{	1.0f, 1.0f,
		0.6f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanIgnited[] =
{
	{	1.0f, 1.0f,
		0.5f, 1.0f,
	},
	{	1.0f, 1.0f,
		30.0f, 30.0f,
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};


/*---------------------------------------------------
	CZombie
	Clase del zombi
---------------------------------------------------*/

class CZombie : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_DATADESC();
	DECLARE_CLASS(CZombie, CAI_BlendingHost<CNPC_BaseZombie>);

public:
	CZombie() : m_DurationDoorBash( 2, 6), m_NextTimeToStartDoorBash( 3.0 )
	{
	}

	void Spawn();
	void Precache();

	void SetZombieModel();
	void MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize);
	bool ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold);

	bool CanBecomeLiveTorso() 
	{ 
		return !m_fIsHeadless;
	}

	void GatherConditions();

	int SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	int TranslateSchedule(int scheduleType);

	#ifndef HL2_EPISODIC
		void CheckFlinches() {}
	#endif // HL2_EPISODIC

	Activity NPC_TranslateActivity(Activity newActivity);
	Activity SelectDoorBash();

	void OnStateChange(NPC_STATE OldState, NPC_STATE NewState);

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	virtual const char *GetLegsModel();
	virtual const char *GetTorsoModel();
	virtual const char *GetHeadcrabClassname();
	virtual const char *GetHeadcrabModel();

	virtual bool OnObstructingDoor(AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult);

	void Ignite(float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false);
	void Extinguish();

	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);
	bool IsHeavyDamage(const CTakeDamageInfo &info);
	bool IsSquashed(const CTakeDamageInfo &info);
	void BuildScheduleTestBits();

	void PrescheduleThink();
	int SelectSchedule();

	void PainSound(const CTakeDamageInfo &info);
	void DeathSound(const CTakeDamageInfo &info);
	void AlertSound();
	void IdleSound();
	void AttackSound();
	void AttackHitSound();
	void AttackMissSound();
	void FootstepSound(bool fRightFoot);
	void FootscuffSound(bool fRightFoot);

	const char *GetMoanSound(int nSound);
	
public:
	DEFINE_CUSTOM_AI;

protected:
	static const char *pMoanSounds[];


private:
	CHandle< CBaseDoor > m_hBlockingDoor;
	float				 m_flDoorBashYaw;
	
	CRandSimTimer 		 m_DurationDoorBash;
	CSimTimer 	  		 m_NextTimeToStartDoorBash;

	Vector				 m_vPositionCharged;
};

LINK_ENTITY_TO_CLASS(npc_zombie, CZombie);
LINK_ENTITY_TO_CLASS(npc_zombie_torso, CZombie);

//----------------------------------------------------
// Sonidos de los gemidos del zombie
//----------------------------------------------------
const char *CZombie::pMoanSounds[] =
{
	 "NPC_BaseZombie.Moan1",
	 "NPC_BaseZombie.Moan2",
	 "NPC_BaseZombie.Moan3",
	 "NPC_BaseZombie.Moan4",
};

//----------------------------------------------------
// Condiciones
//----------------------------------------------------
enum
{
	COND_BLOCKED_BY_DOOR = LAST_BASE_ZOMBIE_CONDITION,
	COND_DOOR_OPENED,
	COND_ZOMBIE_CHARGE_TARGET_MOVED,
};

//----------------------------------------------------
// Eventos
//----------------------------------------------------
enum
{
	SCHED_ZOMBIE_BASH_DOOR = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_WANDER_ANGRILY,
	SCHED_ZOMBIE_CHARGE_ENEMY,
	SCHED_ZOMBIE_FAIL,
};

//----------------------------------------------------
// Tareas
//----------------------------------------------------
enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_YAW_TO_DOOR,
	TASK_ZOMBIE_ATTACK_DOOR,
	TASK_ZOMBIE_CHARGE_ENEMY,
};

//----------------------------------------------------
// Actividades
//----------------------------------------------------

int ACT_ZOMBIE_TANTRUM;
int ACT_ZOMBIE_WALLPOUND;

//----------------------------------------------------
// Definición de datos.
//----------------------------------------------------

BEGIN_DATADESC(CZombie)

	DEFINE_FIELD(m_hBlockingDoor, FIELD_EHANDLE),
	DEFINE_FIELD(m_flDoorBashYaw, FIELD_FLOAT),
	DEFINE_EMBEDDED(m_DurationDoorBash),
	DEFINE_EMBEDDED(m_NextTimeToStartDoorBash),
	DEFINE_FIELD(m_vPositionCharged, FIELD_POSITION_VECTOR),

END_DATADESC()


//----------------------------------------------------
// Precache()
// Precachear objetos y sonidos para su uso.
//----------------------------------------------------
void CZombie::Precache()
{
	BaseClass::Precache();

	PrecacheModel("models/zombie/classic.mdl");
	PrecacheModel("models/zombie/classic_torso.mdl");
	PrecacheModel("models/zombie/classic_legs.mdl");

	PrecacheScriptSound("Zombie.FootstepRight");
	PrecacheScriptSound("Zombie.FootstepLeft");
	PrecacheScriptSound("Zombie.FootstepLeft");
	PrecacheScriptSound("Zombie.ScuffRight");
	PrecacheScriptSound("Zombie.ScuffLeft");
	PrecacheScriptSound("Zombie.AttackHit");
	PrecacheScriptSound("Zombie.AttackMiss");
	PrecacheScriptSound("Zombie.Pain");
	PrecacheScriptSound("Zombie.Die");
	PrecacheScriptSound("Zombie.Alert");
	PrecacheScriptSound("Zombie.Idle");
	PrecacheScriptSound("Zombie.Attack");

	PrecacheScriptSound("NPC_BaseZombie.Moan1");
	PrecacheScriptSound("NPC_BaseZombie.Moan2");
	PrecacheScriptSound("NPC_BaseZombie.Moan3");
	PrecacheScriptSound("NPC_BaseZombie.Moan4");
}

//----------------------------------------------------
// Spawn()
// "Spawnear" al zombi.
//----------------------------------------------------
void CZombie::Spawn()
{
	Precache();

	if(FClassnameIs(this, "npc_zombie"))
		m_fIsTorso	= false;
	
	else
		m_fIsTorso	= true;

	m_fIsHeadless	= false;

	// Color de la sangre.
	SetBloodColor(BLOOD_COLOR_GREEN);

	m_iHealth			= sk_zombie_health.GetFloat();
	m_flFieldOfView		= 0.2;
	m_flGroundSpeed		= 500.0f;

	CapabilitiesClear();
	BaseClass::Spawn();

	// La próxima vez que "gemire"
	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat(1.0, 4.0);
}

//----------------------------------------------------
// PrescheduleThink()
// Tareas futuras a ejecutar.
//----------------------------------------------------
void CZombie::PrescheduleThink()
{
	// Me toca "gemir"
  	if(gpGlobals->curtime > m_flNextMoanSound)
  	{
		// ¿Puedo "gemir"?
  		if(CanPlayMoanSound())
  		{
			// Gemir y decidir cuando volvere a gemir.
			IdleSound();
  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat(2.0, 5.0);
  		}
  		else
  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 2.0 );
  	}

	BaseClass::PrescheduleThink();
}

//----------------------------------------------------
// SelectSchedule()
// Seleccionar evento
//----------------------------------------------------
int CZombie::SelectSchedule()
{
	// Daño por físicas (o también conocido como "daño por mundo")
	if(HasCondition(COND_PHYSICS_DAMAGE) && !m_ActBusyBehavior.IsActive())
		return SCHED_FLINCH_PHYSICS;

	return BaseClass::SelectSchedule();
}

//----------------------------------------------------
// FootstepSound()
// Reproducir sonido de pasos.
//----------------------------------------------------
void CZombie::FootstepSound(bool fRightFoot)
{
	if(fRightFoot)
		EmitSound("Zombie.FootstepRight");
	
	else
		EmitSound("Zombie.FootstepLeft");
}

//----------------------------------------------------
// FootscuffSound()
// Reproducir sonido de pasos deslizantes.
//----------------------------------------------------
void CZombie::FootscuffSound(bool fRightFoot)
{
	if(fRightFoot )
		EmitSound("Zombie.ScuffRight");
	
	else
		EmitSound("Zombie.ScuffLeft");
}

//----------------------------------------------------
// AttackHitSound()
// Reproducir sonido al azar de ataque.
//----------------------------------------------------
void CZombie::AttackHitSound()
{
	EmitSound("Zombie.AttackHit");
}

//----------------------------------------------------
// AttackMissSound()
// Reproducir sonido al azar de ataque fallido.
//----------------------------------------------------
void CZombie::AttackMissSound()
{
	EmitSound( "Zombie.AttackMiss" );
}

//----------------------------------------------------
// PainSound()
// Reproducir sonido de dolor.
//----------------------------------------------------
void CZombie::PainSound(const CTakeDamageInfo &info)
{
	// Cuando el zombi esta en llamas sufre daños constantemente.
	// Para evitar un bucle de sonidos, no reproducir cuando esta en llamas.
	if (IsOnFire())
		return;

	EmitSound("Zombie.Pain");
}

//----------------------------------------------------
// DeathSound()
// Reproducir sonido de muerte.
//----------------------------------------------------
void CZombie::DeathSound(const CTakeDamageInfo &info) 
{
	EmitSound("Zombie.Die");
}

//----------------------------------------------------
// AlertSound()
// Reproducir sonido de alerta.
//----------------------------------------------------
void CZombie::AlertSound()
{
	// Huelo algo... ¿estas ahí? quiero tu carne...
	EmitSound("Zombie.Alert");

	// Retrasar el sonido de "gemido".
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//----------------------------------------------------
// GetMoanSound()
// Obtener los sonidos de "gemido" para esta clase de zombi.
//----------------------------------------------------
const char *CZombie::GetMoanSound(int nSound)
{
	return pMoanSounds[nSound % ARRAYSIZE(pMoanSounds)];
}

//----------------------------------------------------
// IdleSound()
// Reproducir sonido al azar de descanso.
//----------------------------------------------------
void CZombie::IdleSound()
{
	Msg("Velocidad %f \r\n", m_flGroundSpeed);

	// Evitamos sonidos constantes.
	if(GetState() == NPC_STATE_IDLE && random->RandomFloat(0, 1) == 0)
		return;

	// ¿Estamos "durmiendo"?
	if(IsSlumped())
		return;

	EmitSound("Zombie.Idle");
	MakeAISpookySound(360.0f);
}

//----------------------------------------------------
// AttackSound()
// Reproducir sonido al azar de ataque.
//----------------------------------------------------
void CZombie::AttackSound()
{
	EmitSound("Zombie.Attack");
}

//----------------------------------------------------
// GetHeadcrabClassname()
// Nombre de la clase de NPC del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//----------------------------------------------------
const char *CZombie::GetHeadcrabClassname()
{
	return "npc_headcrab";
}

//----------------------------------------------------
// GetHeadcrabModel()
// Ubicación del modelo del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//----------------------------------------------------
const char *CZombie::GetHeadcrabModel()
{
	return MODEL_HEADCRAB;
}

//----------------------------------------------------
// GetLegsModel()
// Ubicación del modelo de las piernas de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//----------------------------------------------------
const char *CZombie::GetLegsModel( void )
{
	return MODEL_LEGS;
}

//----------------------------------------------------
// GetTorsoModel()
// Ubicación del modelo del torso de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//----------------------------------------------------
const char *CZombie::GetTorsoModel( void )
{
	return MODEL_TORSO;
}


//----------------------------------------------------
// SetZombieModel()
// Establecer el modelo de este zombi.
//----------------------------------------------------
void CZombie::SetZombieModel( void )
{
	Hull_t lastHull = GetHullType();

	// ¡Me han partido a la mitad!
	if (m_fIsTorso)
	{
		SetModel(MODEL_TORSO);
		SetHullType(HULL_TINY);
	}
	else
	{
		SetModel(MODEL_ZOMBIE);
		SetHullType(HULL_HUMAN);
	}

	SetBodygroup(ZOMBIE_BODYGROUP_HEADCRAB, !m_fIsHeadless);

	SetHullSizeNormal(true);
	SetDefaultEyeOffset();
	SetActivity(ACT_IDLE);

	if (lastHull != GetHullType())
	{
		if (VPhysicsGetObject())
			SetupVPhysicsHull();
	}
}

//----------------------------------------------------
// MoanSound()
// Reproducir sonido de gemido.
//----------------------------------------------------
void CZombie::MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize)
{
	if(IsOnFire())
		BaseClass::MoanSound(pEnvelope, iEnvelopeSize);
}

//----------------------------------------------------
// ShouldBecomeTorso()
// ¿Partir el zombi a la mitad?
//----------------------------------------------------
bool CZombie::ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold)
{
	// No partir a la mitad cuando esta "dormido".
	// Aparte de ser poco realista, si el jugador mata al zombi con un explosivo (granada, barril)
	// la fuerza de la explosión causara que las piernas/torso salgan disparadas a velocidades ridículas debido a su peso.
	if(IsSlumped()) 
		return false;

	// Dejar la desición a la clase madre. (basezombie)
	return BaseClass::ShouldBecomeTorso(info, flDamageThreshold);
}

//----------------------------------------------------
// GatherConditions()
// Reunir condiciones
//----------------------------------------------------
void CZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	static int conditionsToClear[] = 
	{
		COND_BLOCKED_BY_DOOR,
		COND_DOOR_OPENED,
		COND_ZOMBIE_CHARGE_TARGET_MOVED,
	};

	ClearConditions(conditionsToClear, ARRAYSIZE(conditionsToClear));

	// ¿Objetivo bloqueado por una puerta?
	if (m_hBlockingDoor == NULL || 
		 (m_hBlockingDoor->m_toggle_state == TS_AT_TOP || 
		   m_hBlockingDoor->m_toggle_state == TS_GOING_UP) )
	{
		// No hay puerta.
		ClearCondition(COND_BLOCKED_BY_DOOR);

		// Al parecer porque se abrio.
		if (m_hBlockingDoor != NULL)
		{
			SetCondition(COND_DOOR_OPENED);
			m_hBlockingDoor = NULL;
		}
	}
	else
		SetCondition(COND_BLOCKED_BY_DOOR);

	if (ConditionInterruptsCurSchedule(COND_ZOMBIE_CHARGE_TARGET_MOVED))
	{
		if (GetNavigator()->IsGoalActive())
		{
			const float CHARGE_RESET_TOLERANCE = 60.0;

			if (!GetEnemy() || (m_vPositionCharged - GetEnemyLKP()).Length() > CHARGE_RESET_TOLERANCE)
				SetCondition(COND_ZOMBIE_CHARGE_TARGET_MOVED);
		}
	}
}

//----------------------------------------------------
// SelectFailSchedule()

//----------------------------------------------------
int CZombie::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	if (HasCondition(COND_BLOCKED_BY_DOOR) && m_hBlockingDoor != NULL)
	{
		ClearCondition(COND_BLOCKED_BY_DOOR);

		if (m_NextTimeToStartDoorBash.Expired() && failedSchedule != SCHED_ZOMBIE_BASH_DOOR)
			return SCHED_ZOMBIE_BASH_DOOR;

		m_hBlockingDoor = NULL;
	}

	if (failedSchedule != SCHED_ZOMBIE_CHARGE_ENEMY && IsPathTaskFailure(taskFailCode) && random->RandomInt(1, 100) < 50)
		return SCHED_ZOMBIE_CHARGE_ENEMY;

	if (failedSchedule != SCHED_ZOMBIE_WANDER_ANGRILY && (failedSchedule == SCHED_TAKE_COVER_FROM_ENEMY || failedSchedule == SCHED_CHASE_ENEMY_FAILED))
		return SCHED_ZOMBIE_WANDER_ANGRILY;

	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}

//----------------------------------------------------
// TranslateSchedule()

//----------------------------------------------------
int CZombie::TranslateSchedule(int scheduleType)
{
	if (scheduleType == SCHED_COMBAT_FACE && IsUnreachable(GetEnemy()))
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if (!m_fIsTorso && scheduleType == SCHED_FAIL)
		return SCHED_ZOMBIE_FAIL;

	return BaseClass::TranslateSchedule(scheduleType);
}

//----------------------------------------------------
// NPC_TranslateActivity()
// Convertir actividades que este zombi no posee a 
// actividades válidas.
//----------------------------------------------------
Activity CZombie::NPC_TranslateActivity(Activity newActivity)
{
	newActivity = BaseClass::NPC_TranslateActivity(newActivity);

	// Los zombis clasicos no pueden corren (¡carajo!)
	if (newActivity == ACT_RUN)
		return ACT_WALK;
		
	if (m_fIsTorso && (newActivity == ACT_ZOMBIE_TANTRUM))
		return ACT_IDLE;

	return newActivity;
}

//----------------------------------------------------
// OnStateChange()
// Cuando el estado cambia.
//----------------------------------------------------
void CZombie::OnStateChange(NPC_STATE OldState, NPC_STATE NewState)
{
	BaseClass::OnStateChange(OldState, NewState);
}

//----------------------------------------------------
// StartTask()
// Empezar tarea
//----------------------------------------------------
void CZombie::StartTask(const Task_t *pTask)
{
	switch( pTask->iTask )
	{
		case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if (random->RandomInt( 1, 4 ) == 2)
				SetIdealActivity((Activity)ACT_ZOMBIE_TANTRUM);
			else
				TaskComplete();

			break;
		}

		case TASK_ZOMBIE_YAW_TO_DOOR:
		{
			AssertMsg(m_hBlockingDoor != NULL, "Expected condition handling to break schedule before landing here");

			if (m_hBlockingDoor != NULL)
				GetMotor()->SetIdealYaw(m_flDoorBashYaw);

			TaskComplete();
			break;
		}

		// Atacar puerta.
		case TASK_ZOMBIE_ATTACK_DOOR:
		{
		 	m_DurationDoorBash.Reset();
			SetIdealActivity(SelectDoorBash());

			break;
		}

		// ¿Encontrar enemigo?
		case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			if (!GetEnemy())
				TaskFail(FAIL_NO_ENEMY);

			else if (GetNavigator()->SetVectorGoalFromTarget(GetEnemy()->GetLocalOrigin()))
			{
				m_vPositionCharged = GetEnemy()->GetLocalOrigin();
				TaskComplete();
			}
			
			else
				TaskFail( FAIL_NO_ROUTE );
			
			break;
		}

		default:
			BaseClass::StartTask(pTask);
		break;
	}
}

//----------------------------------------------------
// RunTask()
// Ejecutar tarea.
//----------------------------------------------------
void CZombie::RunTask(const Task_t *pTask)
{
	switch( pTask->iTask )
	{
		// Atacar puerta
		case TASK_ZOMBIE_ATTACK_DOOR:
		{
			if (IsActivityFinished())
			{
				if (m_DurationDoorBash.Expired())
				{
					TaskComplete();
					m_NextTimeToStartDoorBash.Reset();
				}
				else
					ResetIdealActivity(SelectDoorBash());
			}
			break;
		}

		// ¿Buscar enemigo?
		case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			break;
		}

		case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if (IsActivityFinished())
				TaskComplete();

			break;
		}

		default:
			BaseClass::RunTask(pTask);
		break;
	}
}

//----------------------------------------------------
// OnObstructingDoor()
// Cuando una puerta obstruye el objetivo.
//----------------------------------------------------
bool CZombie::OnObstructingDoor(AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult)
{
	if (BaseClass::OnObstructingDoor(pMoveGoal, pDoor, distClear, pResult))
	{
		if  (IsMoveBlocked(*pResult) && pMoveGoal->directTrace.vHitNormal != vec3_origin)
		{
			m_hBlockingDoor = pDoor;
			m_flDoorBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );	
		}

		return true;
	}

	return false;
}

//----------------------------------------------------
// SelectDoorBash()
// Seleccionar tarea adecuada cuando una puerta esta
// obstruyendo el objetivo.
//----------------------------------------------------
Activity CZombie::SelectDoorBash()
{
	// Animación de ataque.
	if (random->RandomInt( 1, 3 ) == 1)
		return ACT_MELEE_ATTACK1;

	// Animación de golpear puerta.
	return (Activity)ACT_ZOMBIE_WALLPOUND;
}

//----------------------------------------------------
// Ignite()
// Tareas al quemarse.
// Nota: Esta función solo sirve para decidir si el
// zombi debe "gritar espantosamente" al quemarse.
// Nota 2: Alemania (y otros países) no permiten
// "gritar espantosamente", encerio...
//----------------------------------------------------
void CZombie::Ignite(float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	// Zombi vivo y no se esta quemando.
 	if(!IsOnFire() && IsAlive())
	{
		// ¡Prenderle fuego!
		BaseClass::Ignite(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);

		// El filtro de "violencia baja" esta desactivado.
		// (Gracias países estrictos...)
		if (!UTIL_IsLowViolence())
		{
			RemoveSpawnFlags(SF_NPC_GAG);
			MoanSound(envZombieMoanIgnited, ARRAYSIZE(envZombieMoanIgnited));

			if (m_pMoanSound)
			{
				ENVELOPE_CONTROLLER.SoundChangePitch(m_pMoanSound, 120, 1.0);
				ENVELOPE_CONTROLLER.SoundChangeVolume(m_pMoanSound, 1, 1.0);
			}
		}
	}
}

//----------------------------------------------------
// Extinguish()
// Zombi quemandose, extinguir fuego.
//----------------------------------------------------
void CZombie::Extinguish()
{
	// ¡Calma! Deja de gritar.
	if(m_pMoanSound)
	{
		ENVELOPE_CONTROLLER.SoundChangeVolume(m_pMoanSound, 0, 2.0);
		ENVELOPE_CONTROLLER.SoundChangePitch(m_pMoanSound, 100, 2.0);

		m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat(2.0, 4.0);
	}

	BaseClass::Extinguish();
}

//----------------------------------------------------
// OnTakeDamage_Alive()
// Al recibir daño y estar aún vivo.
//----------------------------------------------------
int CZombie::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//----------------------------------------------------
// IsHeavyDamage()
// Daños graves
//----------------------------------------------------
bool CZombie::IsHeavyDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsHeavyDamage(info);
}

//----------------------------------------------------
// IsSquashed()
// ¿Producir el efecto de aplastado?
//----------------------------------------------------
bool CZombie::IsSquashed(const CTakeDamageInfo &info)
{
	if(GetHealth() > 0)
		return false;

	if(info.GetDamageType() & DMG_CRUSH)
	{
		IPhysicsObject *pCrusher = info.GetInflictor()->VPhysicsGetObject();

		// Calcular si el objeto es pesado como para producir el efecto.
		// ZOMBIE_SQUASH_MASS = 300 kg
		if(pCrusher && pCrusher->GetMass() >= ZOMBIE_SQUASH_MASS && info.GetInflictor()->WorldSpaceCenter().z > EyePosition().z)
			return true;
	}

	return false;
}

//----------------------------------------------------
// BuildScheduleTestBits()
// 
//----------------------------------------------------
void CZombie::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if(!m_fIsTorso && !IsCurSchedule(SCHED_FLINCH_PHYSICS) && !m_ActBusyBehavior.IsActive())
		SetCustomInterruptCondition( COND_PHYSICS_DAMAGE );
}

	
//----------------------------------------------------
// Condiciones e información de la IA
//----------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_zombie, CZombie)

	DECLARE_CONDITION(COND_BLOCKED_BY_DOOR)
	DECLARE_CONDITION(COND_DOOR_OPENED)
	DECLARE_CONDITION(COND_ZOMBIE_CHARGE_TARGET_MOVED)

	DECLARE_TASK(TASK_ZOMBIE_EXPRESS_ANGER)
	DECLARE_TASK(TASK_ZOMBIE_YAW_TO_DOOR)
	DECLARE_TASK(TASK_ZOMBIE_ATTACK_DOOR)
	DECLARE_TASK(TASK_ZOMBIE_CHARGE_ENEMY)
	
	DECLARE_ACTIVITY(ACT_ZOMBIE_TANTRUM);
	DECLARE_ACTIVITY(ACT_ZOMBIE_WALLPOUND);

	DEFINE_SCHEDULE
	( 
		SCHED_ZOMBIE_BASH_DOOR,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_ZOMBIE_YAW_TO_DOOR			0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_ZOMBIE_ATTACK_DOOR			0"
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_ANGRILY,

		"	Tasks"
		"		TASK_WANDER						480240" // 48 units to 240 units.
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			4"
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHARGE_ENEMY,


		"	Tasks"
		"		TASK_ZOMBIE_CHARGE_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ZOMBIE_TANTRUM" /* placeholder until frustration/rage/fence shake animation available */
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
		"		COND_ZOMBIE_CHARGE_TARGET_MOVED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_WAIT				1"
		"		TASK_WAIT_PVS			0"
		""
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK1 "
		"		COND_CAN_RANGE_ATTACK2 "
		"		COND_CAN_MELEE_ATTACK1 "
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_GIVE_WAY"
		"		COND_DOOR_OPENED"
	)

AI_END_CUSTOM_NPC()

//=============================================================================
