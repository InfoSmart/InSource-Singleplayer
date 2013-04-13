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

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar sk_zombie_health				("sk_zombie_health",			"3",	0, "Salud del zombi clasico.");
ConVar sk_zombie_speed				("sk_zombie_speed",				"0",	0, "Aumento de velocidad.");
ConVar sk_zombie_accel				("sk_zombie_accel",				"0",	0, "Aumento de aceleración.");
ConVar sk_zombie_canrun				("sk_zombie_canrun",			"1",	0, "¿El modelo del zombi puede correr?");

//=========================================================
// Configuración del NPC
//=========================================================

#define BLOOD			BLOOD_COLOR_RED
#define FOV				0.2

#define MODEL_HEADCRAB	"models/headcrabclassic.mdl"
#define MODEL_LEGS		"models/zombie/classic_legs.mdl"
#define MODEL_TORSO		"models/zombie/classic_torso.mdl"

#define CLASS_NAME_HEADCRAB	"npc_headcrab"

static const char *RandomModels[] =
{
	// Valve - HL2
	"classic.mdl",

	// Zombie Master
	/*"zm_classic.mdl",
	"zm_classic_01.mdl",
	"zm_classic_02.mdl",
	"zm_classic_03.mdl",
	"zm_classic_04.mdl",
	"zm_classic_05.mdl",
	"zm_classic_06.mdl",
	"zm_classic_07.mdl",
	"zm_classic_08.mdl",
	"zm_classic_09.mdl",
	"zm_classic_10.mdl",*/

	// Black Mesa
	"zombie_guard.mdl",
	"zombie_sci.mdl",
};

//=========================================================
// Información
//=========================================================

#define ZOMBIE_SQUASH_MASS	300.0f

//=========================================================
// Sonidos y su volumen.
//=========================================================

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

//=========================================================
// Condiciones
//=========================================================

enum
{
	COND_BLOCKED_BY_DOOR = LAST_BASE_ZOMBIE_CONDITION,
	COND_DOOR_OPENED,
	COND_ZOMBIE_CHARGE_TARGET_MOVED,
};

//=========================================================
// Eventos
//=========================================================

enum
{
	SCHED_ZOMBIE_BASH_DOOR = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_WANDER_ANGRILY,
	SCHED_ZOMBIE_CHARGE_ENEMY,
	SCHED_ZOMBIE_FAIL,
};

//=========================================================
// Tareas
//=========================================================

enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_YAW_TO_DOOR,
	TASK_ZOMBIE_ATTACK_DOOR,
	TASK_ZOMBIE_CHARGE_ENEMY,
};

//=========================================================
// Actividades
//=========================================================

int ACT_ZOMBIE_TANTRUM;
int ACT_ZOMBIE_WALLPOUND;

//=========================================================
// Clase del Zombie clásico
//=========================================================

class CZombie : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_DATADESC();
	DECLARE_CLASS(CZombie, CAI_BlendingHost<CNPC_BaseZombie>);

public:
	CZombie() : DurationDoorBash(2, 6), NextTimeToStartDoorBash(3.0)
	{
	}

	void Spawn();
	void Precache();

	bool BlackMesaModel();
	void SetZombieModel();
	void MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize);
	bool ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold);

	bool CanBecomeLiveTorso() { return !IsHeadless; }
	void GatherConditions();

	int SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	int TranslateSchedule(int scheduleType);

	void CheckFlinches() { }

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
	CHandle< CBaseDoor > BlockingDoor;
	float				 DoorBashYaw;

	CRandSimTimer 		 DurationDoorBash;
	CSimTimer 	  		 NextTimeToStartDoorBash;

	Vector				 PositionCharged;
	bool				bModel;
};

LINK_ENTITY_TO_CLASS( npc_zombie,		CZombie );
LINK_ENTITY_TO_CLASS( npc_zombie_torso, CZombie );

//=========================================================
// Definición de datos.
//=========================================================

BEGIN_DATADESC( CZombie )

	DEFINE_FIELD( BlockingDoor,		FIELD_EHANDLE ),
	DEFINE_FIELD( DoorBashYaw,		FIELD_FLOAT ),
	DEFINE_FIELD( PositionCharged,	FIELD_POSITION_VECTOR ),

	DEFINE_EMBEDDED( DurationDoorBash ),
	DEFINE_EMBEDDED( NextTimeToStartDoorBash ),

END_DATADESC()

//=========================================================
// Sonidos de los gemidos del zombi
//=========================================================

const char *CZombie::pMoanSounds[] =
{
	 "NPC_BaseZombie.Moan1",
	 "NPC_BaseZombie.Moan2",
	 "NPC_BaseZombie.Moan3",
	 "NPC_BaseZombie.Moan4",
};

//=========================================================
// Crear un nuevo zombi.
//=========================================================
void CZombie::Spawn()
{
	Precache();
	ConVarRef zombie_attach_headcrab("zombie_attach_headcrab");

	IsTorso			= ( FClassnameIs(this, "npc_zombie") ) ? false : true; // ¿Es el puro torso?
	IsHeadless		= zombie_attach_headcrab.GetBool();

	// Color de la sangre.
	SetBloodColor(BLOOD);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth			= sk_zombie_health.GetFloat();
	m_flFieldOfView		= FOV;
	//m_flGroundSpeed		= 500.0f;

	CapabilitiesClear();
	BaseClass::Spawn();

	// La próxima vez que "gemire"
	NextMoanSound = gpGlobals->curtime + random->RandomFloat(1.0, 4.0);

	bModel = false;

	// Aumentamos su velocidad ¡esto es un juego de zombis!
	SetAddSpeed(sk_zombie_speed.GetFloat());
	SetAddAccel(sk_zombie_accel.GetFloat());
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CZombie::Precache()
{
	BaseClass::Precache();

	// Modelos del torso y piernas.
	PrecacheModel(MODEL_TORSO);
	PrecacheModel(MODEL_LEGS);

	// Distintos modelos para el zombi.
	int Models	= ARRAYSIZE(RandomModels);
	int i;

	for ( i = 0; i < Models; ++i )
		PrecacheModel(CFmtStr("models/zombie/%s", RandomModels[i]));

	// Sonidos.
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

	// Quejidos
	PrecacheScriptSound("NPC_BaseZombie.Moan1");
	PrecacheScriptSound("NPC_BaseZombie.Moan2");
	PrecacheScriptSound("NPC_BaseZombie.Moan3");
	PrecacheScriptSound("NPC_BaseZombie.Moan4");
}

//=========================================================
// Pre-Pensamiento: Bucle de ejecución de tareas.
//=========================================================
void CZombie::PrescheduleThink()
{
	// Al parecer me toca gemir.
  	if ( gpGlobals->curtime > NextMoanSound )
  	{
		// ¿Puedo "gemir"?
  		if ( CanPlayMoanSound() )
  		{
			// Gemir y decidir cuando volvere a gemir.
			IdleSound();
  			NextMoanSound = gpGlobals->curtime + random->RandomFloat(2.0, 5.0);
  		}
  		else
  			NextMoanSound = gpGlobals->curtime + random->RandomFloat(1.0, 2.0);
  	}

	BaseClass::PrescheduleThink();
}

//=========================================================
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CZombie::SelectSchedule()
{
	// Daño por físicas (o también conocido como "daño por mundo")
	if ( HasCondition(COND_PHYSICS_DAMAGE) && !m_ActBusyBehavior.IsActive() )
		return SCHED_FLINCH_PHYSICS;

	return BaseClass::SelectSchedule();
}

//=========================================================
// Reproducir los sonidos de mis pasos.
//=========================================================
void CZombie::FootstepSound(bool fRightFoot)
{
	// Pie derecho
	if ( fRightFoot )
		EmitSound("Zombie.FootstepRight");

	// Pie izquierdo
	else
		EmitSound("Zombie.FootstepLeft");
}

//=========================================================
// Reproducir los sonidos del pie arrastrandose.
//=========================================================
void CZombie::FootscuffSound(bool fRightFoot)
{
	// Pie derecho
	if( fRightFoot )
		EmitSound("Zombie.ScuffRight");

	// Pie izquierdo
	else
		EmitSound("Zombie.ScuffLeft");
}

//=========================================================
// Reproducir sonido al azar de ataque.
//=========================================================
void CZombie::AttackHitSound()
{
	EmitSound("Zombie.AttackHit");
}

//=========================================================
// Reproducir sonido al azar de ataque fallido.
//=========================================================
void CZombie::AttackMissSound()
{
	EmitSound("Zombie.AttackMiss");
}

//=========================================================
// Reproducir sonido de dolor.
//=========================================================
void CZombie::PainSound(const CTakeDamageInfo &info)
{
	// Cuando el zombi esta en llamas sufre daños constantemente.
	// Para evitar un bucle de sonidos, no reproducir cuando este en llamas.
	if ( IsOnFire() )
		return;

	EmitSound("Zombie.Pain");
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CZombie::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("Zombie.Die");
}

//=========================================================
// Reproducir sonido de alerta.
//=========================================================
void CZombie::AlertSound()
{
	// Huelo algo... ¿estas ahí? quiero tu carne...
	EmitSound("Zombie.Alert");

	// Retrasar el sonido de "gemido".
	NextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//=========================================================
// Obtener los sonidos de "gemido" para esta clase de zombi.
//=========================================================
const char *CZombie::GetMoanSound(int nSound)
{
	return pMoanSounds[nSound % ARRAYSIZE(pMoanSounds)];
}

//=========================================================
// Reproducir sonido al azar de descanso.
//=========================================================
void CZombie::IdleSound()
{
	// Evitamos sonidos constantes.
	if( GetState() == NPC_STATE_IDLE && random->RandomFloat(0, 1) == 0 )
		return;

	// ¿Estamos "durmiendo"? ¡No hacer ruido!
	if( IsSlumped() )
		return;

	EmitSound("Zombie.Idle");
	MakeAISpookySound(360.0f);
}

//=========================================================
// Reproducir sonido al azar de ataque.
//=========================================================
void CZombie::AttackSound()
{
	EmitSound("Zombie.Attack");
}

//=========================================================
// Nombre de la clase de NPC del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//=========================================================
const char *CZombie::GetHeadcrabClassname()
{
	return CLASS_NAME_HEADCRAB;
}

//=========================================================
// Ubicación del modelo del headcrab de este zombi.
// Es el mismo que se "desprenderá" del zombi al morir.
//=========================================================
const char *CZombie::GetHeadcrabModel()
{
	return MODEL_HEADCRAB;
}

//=========================================================
// Ubicación del modelo de las piernas de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//=========================================================
const char *CZombie::GetLegsModel()
{
	return MODEL_LEGS;
}

//=========================================================
// Ubicación del modelo del torso de este zombi.
// Usado en caso de que el zombi se parta a la mitad.
//=========================================================
const char *CZombie::GetTorsoModel()
{
	return MODEL_TORSO;
}

//=========================================================
// Establece el modelo de este zombi.
//=========================================================
void CZombie::SetZombieModel()
{
	Hull_t LastHull = GetHullType();

	// ¡Me han partido a la mitad!
	if ( IsTorso )
	{
		SetModel(MODEL_TORSO);
		SetHullType(HULL_TINY);
	}
	else
	{
		const char *ZombieModel;

		if ( !bModel )
		{
			// Obtenemos un modelo al azar.
			int rModel	= random->RandomInt(0, (ARRAYSIZE(RandomModels) - 1));
			ZombieModel = CFmtStr("models/zombie/%s", RandomModels[rModel]);

			bModel = true;
		}
		else
			ZombieModel = CFmtStr("models/zombie/%s", GetModelName());
		
		SetModel(ZombieModel);
		SetHullType(HULL_HUMAN);
	}

	SetBodygroup(ZOMBIE_BODYGROUP_HEADCRAB, !IsHeadless);

	SetHullSizeNormal(true);
	SetDefaultEyeOffset();
	SetActivity(ACT_IDLE);

	// El tamaño del zombi ha cambiado.
	if ( LastHull != GetHullType() )
	{
		if ( VPhysicsGetObject() )
			SetupVPhysicsHull();
	}
}

bool CZombie::BlackMesaModel()
{
	if ( GetModelName() == MAKE_STRING("models/zombies/zombie_guard.mdl") || GetModelName() == MAKE_STRING("models/zombies/zombie_sci.mdl") )
		return true;

	return false;
}

//=========================================================
// Reproducir sonido de gemido.
//=========================================================
void CZombie::MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize)
{
	if ( IsOnFire() )
		BaseClass::MoanSound(pEnvelope, iEnvelopeSize);
}

//=========================================================
// ¿Partir el zombi a la mitad?
//=========================================================
bool CZombie::ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold)
{
	// No partir a la mitad cuando esta "dormido".
	// Aparte de ser poco realista, si el jugador mata al zombi con un explosivo (granada, barril)
	// la fuerza de la explosión causara que las piernas/torso salgan disparadas a velocidades ridículas debido a su peso.
	if ( IsSlumped() )
		return false;

	// Dejar la desición a la clase madre. (basezombie)
	return BaseClass::ShouldBecomeTorso(info, flDamageThreshold);
}

//=========================================================
// GatherConditions()
// Reunir condiciones
//=========================================================
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
	if ( BlockingDoor == NULL ||
		 (BlockingDoor->m_toggle_state == TS_AT_TOP ||
		   BlockingDoor->m_toggle_state == TS_GOING_UP) )
	{
		// No hay puerta.
		ClearCondition(COND_BLOCKED_BY_DOOR);

		// Al parecer porque se abrio.
		if ( BlockingDoor != NULL )
		{
			SetCondition(COND_DOOR_OPENED);
			BlockingDoor = NULL;
		}
	}
	else
		SetCondition(COND_BLOCKED_BY_DOOR);

	if ( ConditionInterruptsCurSchedule(COND_ZOMBIE_CHARGE_TARGET_MOVED) )
	{
		if ( GetNavigator()->IsGoalActive() )
		{
			const float CHARGE_RESET_TOLERANCE = 60.0;

			if ( !GetEnemy() || (PositionCharged - GetEnemyLKP()).Length() > CHARGE_RESET_TOLERANCE )
				SetCondition(COND_ZOMBIE_CHARGE_TARGET_MOVED);
		}
	}
}

//=========================================================
// SelectFailSchedule()

//=========================================================
int CZombie::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	if ( HasCondition(COND_BLOCKED_BY_DOOR) && BlockingDoor != NULL )
	{
		ClearCondition(COND_BLOCKED_BY_DOOR);

		if (NextTimeToStartDoorBash.Expired() && failedSchedule != SCHED_ZOMBIE_BASH_DOOR)
			return SCHED_ZOMBIE_BASH_DOOR;

		BlockingDoor = NULL;
	}

	if ( failedSchedule != SCHED_ZOMBIE_CHARGE_ENEMY && IsPathTaskFailure(taskFailCode) && random->RandomInt(1, 100) < 50 )
		return SCHED_ZOMBIE_CHARGE_ENEMY;

	if ( failedSchedule != SCHED_ZOMBIE_WANDER_ANGRILY && (failedSchedule == SCHED_TAKE_COVER_FROM_ENEMY || failedSchedule == SCHED_CHASE_ENEMY_FAILED) )
		return SCHED_ZOMBIE_WANDER_ANGRILY;

	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}

//=========================================================
// TranslateSchedule()

//=========================================================
int CZombie::TranslateSchedule(int scheduleType)
{
	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable(GetEnemy()) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if ( !IsTorso && scheduleType == SCHED_FAIL )
		return SCHED_ZOMBIE_FAIL;

	if ( scheduleType == SCHED_IDLE_STAND || scheduleType == SCHED_ALERT_STAND )
		return SCHED_ZOMBIE_WANDER_ANGRILY;

	return BaseClass::TranslateSchedule(scheduleType);
}

//=========================================================
// Convierte actividades que este zombi no posee a
// actividades válidas.
//=========================================================
Activity CZombie::NPC_TranslateActivity(Activity newActivity)
{
	newActivity = BaseClass::NPC_TranslateActivity(newActivity);

	// Los zombis clasicos no pueden corren (¡carajo!)
	if ( !sk_zombie_canrun.GetBool() )
	{
		if ( newActivity == ACT_RUN )
			return ACT_WALK;
	}

	if ( IsTorso && (newActivity == ACT_ZOMBIE_TANTRUM) )
		return ACT_IDLE;

	return newActivity;
}

//=========================================================
// OnStateChange()
// Cuando el estado cambia.
//=========================================================
void CZombie::OnStateChange(NPC_STATE OldState, NPC_STATE NewState)
{
	BaseClass::OnStateChange(OldState, NewState);
}

//=========================================================
// StartTask()
// Empezar tarea
//=========================================================
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
			AssertMsg(BlockingDoor != NULL, "Expected condition handling to break schedule before landing here");

			if ( BlockingDoor != NULL )
				GetMotor()->SetIdealYaw(DoorBashYaw);

			TaskComplete();
			break;
		}

		// Atacar puerta.
		case TASK_ZOMBIE_ATTACK_DOOR:
		{
		 	DurationDoorBash.Reset();
			SetIdealActivity(SelectDoorBash());

			break;
		}

		// ¿Encontrar enemigo?
		case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
				TaskFail(FAIL_NO_ENEMY);

			else if ( GetNavigator()->SetVectorGoalFromTarget(GetEnemy()->GetLocalOrigin()) )
			{
				PositionCharged = GetEnemy()->GetLocalOrigin();
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

//=========================================================
// RunTask()
// Ejecutar tarea.
//=========================================================
void CZombie::RunTask(const Task_t *pTask)
{
	switch( pTask->iTask )
	{
		// Atacar puerta
		case TASK_ZOMBIE_ATTACK_DOOR:
		{
			if ( IsActivityFinished() )
			{
				if ( DurationDoorBash.Expired() )
				{
					TaskComplete();
					NextTimeToStartDoorBash.Reset();
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
			if ( IsActivityFinished() )
				TaskComplete();

			break;
		}

		default:
			BaseClass::RunTask(pTask);
		break;
	}
}

//=========================================================
// OnObstructingDoor()
// Cuando una puerta obstruye el objetivo.
//=========================================================
bool CZombie::OnObstructingDoor(AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult)
{
	if ( BaseClass::OnObstructingDoor(pMoveGoal, pDoor, distClear, pResult) )
	{
		if  ( IsMoveBlocked(*pResult) && pMoveGoal->directTrace.vHitNormal != vec3_origin )
		{
			BlockingDoor = pDoor;
			DoorBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );
		}

		return true;
	}

	return false;
}

//=========================================================
// SelectDoorBash()
// Seleccionar tarea adecuada cuando una puerta esta
// obstruyendo el objetivo.
//=========================================================
Activity CZombie::SelectDoorBash()
{
	// Animación de ataque.
	if ( random->RandomInt( 1, 3 ) == 1 )
		return ACT_MELEE_ATTACK1;

	// Animación de golpear puerta.
	return (Activity)ACT_ZOMBIE_WALLPOUND;
}

//=========================================================
// Ignite()
// Tareas al quemarse.
// Nota: Esta función solo sirve para decidir si el
// zombi debe "gritar espantosamente" al quemarse.
// Nota 2: Alemania (y otros países) no permiten
// "gritar espantosamente", encerio...
//=========================================================
void CZombie::Ignite(float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	// Zombi vivo y no se esta quemando.
 	if( !IsOnFire() && IsAlive() )
	{
		// ¡Prenderle fuego!
		BaseClass::Ignite(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);

		// El filtro de "violencia baja" esta desactivado.
		// (Gracias países estrictos...)
		if ( !UTIL_IsLowViolence() )
		{
			RemoveSpawnFlags(SF_NPC_GAG);
			MoanSound(envZombieMoanIgnited, ARRAYSIZE(envZombieMoanIgnited));

			if ( sMoanSound )
			{
				ENVELOPE_CONTROLLER.SoundChangePitch(sMoanSound, 120, 1.0);
				ENVELOPE_CONTROLLER.SoundChangeVolume(sMoanSound, 1, 1.0);
			}
		}
	}
}

//=========================================================
// Zombi quemandose, extinguir fuego.
//=========================================================
void CZombie::Extinguish()
{
	// ¡Calma! Deja de gritar.
	if( sMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundChangeVolume(sMoanSound, 0, 2.0);
		ENVELOPE_CONTROLLER.SoundChangePitch(sMoanSound, 100, 2.0);

		NextMoanSound = gpGlobals->curtime + random->RandomFloat(2.0, 4.0);
	}

	BaseClass::Extinguish();
}

//=========================================================
// OnTakeDamage_Alive()
// Al recibir daño y estar aún vivo.
//=========================================================
int CZombie::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// IsHeavyDamage()
// Daños graves
//=========================================================
bool CZombie::IsHeavyDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsHeavyDamage(info);
}

//=========================================================
// IsSquashed()
// ¿Producir el efecto de aplastado?
//=========================================================
bool CZombie::IsSquashed(const CTakeDamageInfo &info)
{
	if( GetHealth() > 0 )
		return false;

	if( info.GetDamageType() & DMG_CRUSH )
	{
		IPhysicsObject *pCrusher = info.GetInflictor()->VPhysicsGetObject();

		// Calcular si el objeto es pesado como para producir el efecto.
		// ZOMBIE_SQUASH_MASS = 300 kg
		if( pCrusher && pCrusher->GetMass() >= ZOMBIE_SQUASH_MASS && info.GetInflictor()->WorldSpaceCenter().z > EyePosition().z )
			return true;
	}

	return false;
}

//=========================================================
//=========================================================
void CZombie::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if ( !IsTorso && !IsCurSchedule(SCHED_FLINCH_PHYSICS) && !m_ActBusyBehavior.IsActive() )
		SetCustomInterruptCondition(COND_PHYSICS_DAMAGE);
}

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

AI_BEGIN_CUSTOM_NPC( npc_zombie, CZombie )

	DECLARE_CONDITION( COND_BLOCKED_BY_DOOR )
	DECLARE_CONDITION( COND_DOOR_OPENED )
	DECLARE_CONDITION( COND_ZOMBIE_CHARGE_TARGET_MOVED )

	DECLARE_TASK( TASK_ZOMBIE_EXPRESS_ANGER )
	DECLARE_TASK( TASK_ZOMBIE_YAW_TO_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_ATTACK_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_CHARGE_ENEMY )

	DECLARE_ACTIVITY( ACT_ZOMBIE_TANTRUM );
	DECLARE_ACTIVITY( ACT_ZOMBIE_WALLPOUND );

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