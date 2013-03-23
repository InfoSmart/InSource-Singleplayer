//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC Grunt
//
// Soldado alienigena sacado de Black Mesa: Source
// En Apocalypse es un soldado de la empresa "malvada". ¡Tiene mucha vida!
// Inspiración: "Tank" de Left 4 Dead
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
#include "ai_behavior.h"

#include "npc_grunt.h"
#include "hl2_player.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "entitylist.h"
#include "engine/ienginesound.h"

#include "weapon_brickbat.h"
#include "ammodef.h"
#include "grenade_spit.h"
#include "grenade_brickbat.h"

//#include "player.h"
//#include "in_player.h"
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

//extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

//=========================================================
// Definición de variables para la configuración.
//=========================================================

ConVar sk_grunt_health		("sk_grunt_health",			"0", 0, "Salud del Grunt");
ConVar sk_grunt_dmg_high	("sk_grunt_dmg_high",		"0", 0, "Daño causado por un golpe alto");
ConVar sk_grunt_dmg_low		("sk_grunt_dmg_low",		"0", 0, "Daño causado por un golpe bajo");

//=========================================================
// Configuración del NPC
//=========================================================

// Modelo
#define MODEL_BASE		"models/xenians/agrunt.mdl"

// ¿Qué capacidades tendrá?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_MOVE_JUMP | bits_CAP_TURN_HEAD

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_MECH

#define RENDER_COLOR	255, 255, 255, 255

// Distancia de visibilidad.
#define SEE_DIST		9000.0f

// Campo de visión
#define FOV				-0.4f

// Propiedades
// No disolverse (Con la bola de energía) - No morir con la super arma de gravedad.
#define EFLAGS			EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL

// Opciones extra - Flags (Para el Hammer Editor)
#define SF_GRUNT_NO_BACKGROUND_MUSIC	0x00010000

// Lanzar objetos
#define THROW_PHYSICS_FARTHEST_OBJECT	40.0*12.0
#define THROW_PHYSICS_SEARCH_DEPTH		100
#define THROW_PHYSICS_MAX_MASS			8000
#define THROW_PHYSICS_SWATDIST			80
#define THROW_DELAY						5

#define MELEE_ATTACK1_DIST				130
#define MELEE_ATTACK1_IMPULSE			400
#define MELEE_ATTACK1_MIN_PUNCH			60
#define MELEE_ATTACK1_MAX_PUNCH			100

#define MELEE_ATTACK2_DIST				80
#define MELEE_ATTACK2_IMPULSE			200
#define MELEE_ATTACK2_MIN_PUNCH			40
#define MELEE_ATTACK2_MAX_PUNCH			60

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_THROW = LAST_SHARED_SCHEDULE,
	SCHED_MOVE_THROWITEM,
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
	COND_MELEE_OBSTRUCTION
};

//=========================================================
// Eventos de animaciones
//=========================================================

int AE_AGRUNT_MELEE_ATTACK_HIGH;
int AE_AGRUNT_MELEE_ATTACK_LOW;
int AE_AGRUNT_RANGE_ATTACK1;
int AE_SWAT_OBJECT;

//=========================================================
// Animaciones
//=========================================================

int CNPC_Grunt::ACT_SWATLEFTMID;

int ImpactEffectTexture = -1;

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( npc_grunt, CNPC_Grunt );

BEGIN_DATADESC( CNPC_Grunt )

	DEFINE_FIELD( LastHurtTime,				FIELD_TIME ),
	DEFINE_FIELD( NextAlertSound,			FIELD_TIME ),
	DEFINE_FIELD( NextPainSound,			FIELD_TIME ),
	DEFINE_FIELD( NextThrow,				FIELD_TIME ),

	DEFINE_FIELD( PhysicsEnt,			FIELD_EHANDLE ),
	DEFINE_FIELD( PhysicsCanThrow,		FIELD_BOOLEAN ),

END_DATADESC()

//=========================================================
// Spawn()
// Crear un nuevo Grunt
//=========================================================
void CNPC_Grunt::Spawn()
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
	//SetNavType(NAV_GROUND);	
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(RENDER_COLOR);
	SetDistLook(SEE_DIST);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth				= sk_grunt_health.GetFloat();
	m_NPCState				= NPC_STATE_IDLE;
	m_flFieldOfView			= FOV;

	// Más información
	NextThrow				= gpGlobals->curtime;
	NextAlertSound			= gpGlobals->curtime + 3;
	NextPainSound			= gpGlobals->curtime;
	NextRangeAttack1		= gpGlobals->curtime + 5;
	PhysicsEnt				= NULL;
	PhysicsCanThrow			= false;

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFLAGS);

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
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
	if ( gpGlobals->curtime >= NextPainSound )
	{
		EmitSound("NPC_Grunt.Pain");
		NextPainSound = gpGlobals->curtime + random->RandomFloat(1.0, 3.0);
	}
}

//=========================================================
// AlertSound()
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Grunt::AlertSound()
{
	if ( gpGlobals->curtime >= NextAlertSound )
	{
		EmitSound("NPC_Grunt.Alert");
		NextAlertSound = gpGlobals->curtime + random->RandomFloat(.5, 2.0);
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
void CNPC_Grunt::AttackSound(bool highAttack)
{
	if ( highAttack )
		EmitSound("NPC_Grunt.HighAttack");
	else
		EmitSound("NPC_Grunt.LowAttack");
}

//=========================================================
// MaxYawSpeed()
// Devuelve la velocidad máxima del yaw dependiendo de la
// actividad actual.
//=========================================================
float CNPC_Grunt::MaxYawSpeed()
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
// HandleAnimEvent()
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_Grunt::HandleAnimEvent(animevent_t *pEvent)
{
	const char *pName = EventList_NameForIndex(pEvent->event);
	DevMsg("[GRUNT] Se ha producido el evento %s \n", pName);

	// Ataque de fuerza mayor.
	if ( pEvent->event == AE_AGRUNT_MELEE_ATTACK_HIGH )
	{
		MeleeAttack(true);
		return;
	}

	// Ataque de fuerza menor.
	if ( pEvent->event == AE_AGRUNT_MELEE_ATTACK_LOW )
	{
		MeleeAttack();
		return;
	}

	// Disparo
	/*
	if( pEvent->event == AE_AGRUNT_RANGE_ATTACK1 )
	{
		RangeAttack1();
		return;
	}
	*/

	// Aventando objeto.
	if ( pEvent->event == AE_SWAT_OBJECT )
	{
		CBaseEntity *pEnemy = GetEnemy();

		if( pEnemy )
		{
			Vector v;
			CBaseEntity *pPhysicsEntity = PhysicsEnt;

			if( !pPhysicsEntity )
				return;

			IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

			if( !pPhysObj )
				return;

			PhysicsImpactSound(pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 1800);

			Vector PhysicsCenter	= pPhysicsEntity->WorldSpaceCenter();
			v						= pEnemy->WorldSpaceCenter() - PhysicsCenter;

			VectorNormalize(v);

			v	= v * 1800;
			v.z += 400;

			AngularImpulse AngVelocity(random->RandomFloat(-180, 180), 20, random->RandomFloat(-360, 360));
			pPhysObj->AddVelocity(&v, &AngVelocity);

			PhysicsEnt	= NULL;
			NextThrow	= gpGlobals->curtime + THROW_DELAY;
		}

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

bool CNPC_Grunt::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 220.0f;
	const float MAX_JUMP_DISTANCE	= 512.0f;
	const float MAX_JUMP_DROP		= 384.0f;

	return BaseClass::IsJumpLegal(startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE);
}

//=========================================================
// Ataque cuerpo a cuerpo.
//=========================================================
void CNPC_Grunt::MeleeAttack(bool highAttack)
{
	// No hay un enemigo a quien atacar.
	if ( !GetEnemy() )
		return;

	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();
	CBaseEntity *pVictim	= NULL;
	float pDamage			= ( highAttack ) ? sk_grunt_dmg_high.GetFloat() : sk_grunt_dmg_low.GetFloat();
	int pTypeDamage			= DMG_SLASH | DMG_ALWAYSGIB;

	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs();
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	int Melee_Attack_Dist		= ( highAttack ) ? MELEE_ATTACK1_DIST : MELEE_ATTACK2_DIST;
	int Melee_Attack_Impulse	= ( highAttack ) ? MELEE_ATTACK1_IMPULSE : MELEE_ATTACK2_IMPULSE;
	int Melee_Attack_Min_Punch	= ( highAttack ) ? MELEE_ATTACK1_MIN_PUNCH : MELEE_ATTACK2_MIN_PUNCH;
	int Melee_Attack_Max_Punch	= ( highAttack ) ? MELEE_ATTACK1_MAX_PUNCH : MELEE_ATTACK2_MAX_PUNCH;

	// Siempre debemos atacar a un Bullseye
	if ( GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		pVictim = GetEnemy();
		CTakeDamageInfo info(this, this, vec3_origin, GetAbsOrigin(), pDamage, pTypeDamage);
		pVictim->TakeDamage(info);
	}
	// Intentamos realizar un ataque verdadero contra el jugador o varios NPC.
	else
		pVictim = CheckTraceHullAttack(Melee_Attack_Dist, vecMins, vecMaxs, pDamage, pTypeDamage, 1.0, false);

	if ( pVictim )
	{
		// Grrr! te he dado.
		AttackSound(highAttack);
		
		Vector forward, up;
		AngleVectors(GetAbsAngles(), &forward, NULL, &up);

		Vector pImpulse = Melee_Attack_Impulse * (up + 2 * forward);

		// Nuestra victima es el jugador.
		if ( pVictim->IsPlayer() )
		{
			int pPunch = random->RandomInt(Melee_Attack_Min_Punch, Melee_Attack_Max_Punch);

			// Desorientar
			pVictim->ViewPunch(QAngle(pPunch, 0, -pPunch));

			// Lanzarlo por los aires.
			pVictim->ApplyAbsVelocityImpulse(pImpulse);

			// El jugador tiene un arma.
			if ( pPlayer->GetActiveWeapon() && !GameRules()->IsSkillLevel(SKILL_EASY) )
			{
				// !!!REFERENCE
				// En Left4Dead cuando un Tank avienta por los aires a un jugador el mismo
				// "desactiva" su arma hasta que cae, después se crea la animación de levantar y activar
				// el arma. (Mientras esta desactivada no se puede disparar)

				// Ocultar el arma del jugador.
				// FIXME: Incluso con el arma oculta es posible disparar.
				// FIXME 2: Si el arma ya esta "oculta" el juego lanza una excepción (Se va al carajo...)
				pPlayer->GetActiveWeapon()->Holster();

				// El lanzamiento fue muy poderoso, hacer que el jugador suelte el arma.
				// FIXME: Si el jugador al momento de soltar el arma tenia 100 balas de un máximo de 200
				// al recojer el arma su munición se restaura a 200. (Balas gratis)
				if ( pPunch > 90 && random->RandomInt(0, 1) == 1 )
					pPlayer->GetActiveWeapon()->Drop(pImpulse * 1.5);
			}
		}
		// Nuestra victima es un NPC (o algo así...)
		else
		{
			// Lanzamos por los aires
			pVictim->ApplyAbsVelocityImpulse(pImpulse * 3);
			pVictim->VelocityPunch(pImpulse * 2);

			// ¡GRRR! Quitense de mi camino. (Matamos al NPC)
			CTakeDamageInfo damage;

			damage.SetAttacker(this);
			damage.SetInflictor(this);
			damage.SetDamage(pVictim->GetHealth());
			damage.SetDamageType(pTypeDamage);

			pVictim->TakeDamage(damage);
		}
	}
	else
	{
		// TODO
		//FailAttackSound();
	}
}

/*
void CNPC_Grunt::RangeAttack1()
{
	if( gpGlobals->curtime <= NextRangeAttack1 )
		return;

	trace_t tr;
	Vector vecShootPos;
	GetAttachment(LookupAttachment("muzzle"), vecShootPos);

	Vector vecShootDir;
	vecShootDir	= GetEnemy()->WorldSpaceCenter() - vecShootPos;

	FireBullets(5, vecShootPos, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, 1, 2, entindex(), 0);

	/*
	Vector m_blastHit;
	Vector m_blastNormal;

	

	
	float flDist	= VectorNormalize(vecShootDir);

	AI_TraceLine(vecShootPos, vecShootPos + vecShootDir * flDist, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	m_blastHit		= tr.endpos;
	m_blastHit		+= tr.plane.normal * 16;
	m_blastNormal	= tr.plane.normal;

	EntityMessageBegin(this, true);
		WRITE_BYTE(1);
		WRITE_VEC3COORD(tr.endpos);
	MessageEnd();

	CPASAttenuationFilter filter2(this, "NPC_Strider.Shoot");
	EmitSound(filter2, entindex(), "NPC_Strider.Shoot");

	CreateConcussiveBlast(m_blastHit, m_blastNormal, this, 1.5);
	--

	NextRangeAttack1 = gpGlobals->curtime + random->RandomFloat(5.0, 10.0);
}
*/

//=========================================================
// MeleeAttack1Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe alto
//=========================================================
int CNPC_Grunt::MeleeAttack1Conditions(float flDot, float flDist)
{
	// El Grunt es muy poderoso, no podemos matar a los personajes vitales.
	if ( GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL )
		return COND_NONE;
	
	// Distancia y angulo correcto, ¡ataque!
	if ( flDist <= 30 && flDot >= 0.7 )
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

	if ( tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt))) )
		return COND_CAN_MELEE_ATTACK1;

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
		return COND_CAN_MELEE_ATTACK1;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// MeleeAttack2Conditions()
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe bajo
//=========================================================
int CNPC_Grunt::MeleeAttack2Conditions(float flDot, float flDist)
{
	if( flDist <= 50 && flDot >= 0.7 )
		return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}

/*
int CNPC_Grunt::RangeAttack1Conditions(float flDot, float flDist)
{
	if( gpGlobals->curtime <= NextRangeAttack1 )
		return COND_NONE;

	if( flDist <= 800 && flDot >= 0.7 )
		return COND_CAN_RANGE_ATTACK1;

	return COND_TOO_FAR_TO_ATTACK;
}
*/

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
	BaseClass::Event_Killed(info);
}

//=========================================================
// GatherConditions()
// 
//=========================================================
void CNPC_Grunt::GatherConditions()
{
	BaseClass::GatherConditions();

	if(gpGlobals->curtime >= NextThrow && PhysicsEnt == NULL)
	{
		FindNearestPhysicsObject();
		NextThrow = gpGlobals->curtime + 2.0;
	}

	if( PhysicsEnt != NULL && gpGlobals->curtime >= NextThrow && HasCondition(COND_SEE_ENEMY) )
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
	if( HasCondition(COND_CAN_THROW) )
		return SCHED_THROW;

	if ( HasCondition(COND_ENEMY_OCCLUDED) || HasCondition(COND_ENEMY_TOO_FAR) || HasCondition(COND_TOO_FAR_TO_ATTACK) || HasCondition(COND_NOT_FACING_ATTACK) )
		return SCHED_CHASE_ENEMY;

	return BaseClass::SelectSchedule();
}

//=========================================================
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CNPC_Grunt::SelectSchedule()
{
	if( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch( m_NPCState )
	{
		case NPC_STATE_COMBAT:
		{
			return SelectCombatSchedule();
			break;
		}

		case NPC_STATE_IDLE:
		{
			int Idle = BaseClass::SelectSchedule();

			if( Idle == SCHED_IDLE_STAND )
				return SCHED_CHANGE_POSITION;

			return Idle;
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
//=========================================================
int CNPC_Grunt::TranslateSchedule(int scheduleType)
{
	switch( scheduleType )
	{
		case SCHED_THROW:
			if( DistToPhysicsEnt() > THROW_PHYSICS_SWATDIST )
				return SCHED_MOVE_THROWITEM;
			else
				return SCHED_THROW;
		break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
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
			NextThrow = gpGlobals->curtime + pTask->flTaskData;
			TaskComplete();

			break;
		}

		case TASK_GET_PATH_TO_PHYSOBJ:
		{
			if( PhysicsEnt == NULL )
				TaskFail("No hay ningun objeto para lanzar");

			Vector vecGoalPos;
			Vector vecDir;

			vecDir		= GetLocalOrigin() - PhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z	= 0;

			AI_NavGoal_t goal( PhysicsEnt->WorldSpaceCenter() );
			goal.pTarget = PhysicsEnt;
			GetNavigator()->SetGoal(goal);

			TaskComplete();
			break;
		}

		case TASK_THROW_PHYSOBJ:
		{
			if( PhysicsEnt == NULL )
				TaskFail("No hay ningun objeto para lanzar");

			else if( DistToPhysicsEnt() > THROW_PHYSICS_SWATDIST )
				TaskFail("El objeto ya no esta a mi alcanze");

			else
				SetIdealActivity((Activity)ACT_SWATLEFTMID);

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
		case TASK_THROW_PHYSOBJ:
			if( IsActivityFinished() )
				TaskComplete();
		break;

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

	if( !GetEnemy() )
	{
		PhysicsEnt = NULL;
		return false;
	}

	// Calcular la distancia al enemigo.
	vecDirToEnemy	= GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	float dist		= VectorNormalize(vecDirToEnemy);
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

				if ( pEntity && 
					 pEntity->VPhysicsGetObject() && 
					 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
					 pEntity->VPhysicsGetObject()->IsAsleep() && 
					 pEntity->VPhysicsGetObject()->IsMoveable() )
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

	for( int i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[i]->VPhysicsGetObject();

		Assert(!(!pPhysObj || pPhysObj->GetMass() > THROW_PHYSICS_MAX_MASS || !pPhysObj->IsAsleep()));

		Vector center	= pList[i]->WorldSpaceCenter();
		flDist			= UTIL_DistApprox2D(GetAbsOrigin(), center);

		if( flDist >= flNearestDist )
			continue;

		// Este objeto esta muy cerca... pero ¿esta entre el npc y el enemigo?
		vecDirToObject = pList[i]->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		if(DotProduct(vecDirToEnemy, vecDirToObject) < 0.8)
			continue;

		if( flDist >= UTIL_DistApprox2D(center, GetEnemy()->GetAbsOrigin()) )
			continue;

		// No lanzar objetos que esten más arriba de mis rodillas.
		if ((center.z + pList[i]->BoundingRadius()) < vecGruntKnees.z)
			continue;

		// No lanzar objetos que esten sobre mi cabeza.
		if(center.z > EyePosition().z)
			continue;

		vcollide_t *pCollide = modelinfo->GetVCollide(pList[i]->GetModelIndex());
		
		Vector objMins, objMaxs;
		physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles());

		if ( objMaxs.z < vecGruntKnees.z )
			continue;

		if ( !FVisible(pList[i]) )
			continue;

		if( !GetEnemy()->FVisible(pList[i]) )
			continue;

		// No lanzar cadaveres de ningún tipo...
		if ( FClassnameIs(pList[i], "physics_prop_ragdoll") )
			continue;
			
		if ( FClassnameIs(pList[i], "prop_ragdoll") )
			continue;

		// El objeto es pefecto para lanzar, cumple lo necesario.
		pNearest		= pList[i];
		flNearestDist	= flDist;
	}

	PhysicsEnt = pNearest;

	if( PhysicsEnt == NULL )
		return false;
	
	return true;
}

float CNPC_Grunt::DistToPhysicsEnt()
{
	if ( PhysicsEnt != NULL )
		return UTIL_DistApprox2D(GetAbsOrigin(), PhysicsEnt->WorldSpaceCenter());

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

AI_BEGIN_CUSTOM_NPC( npc_grunt, CNPC_Grunt )

	DECLARE_TASK( TASK_GET_PATH_TO_PHYSOBJ )
	DECLARE_TASK( TASK_THROW_PHYSOBJ )
	DECLARE_TASK( TASK_DELAY_THROW )

	DECLARE_CONDITION( COND_CAN_THROW )
	DECLARE_CONDITION( COND_MELEE_OBSTRUCTION )

	DECLARE_ANIMEVENT( AE_AGRUNT_MELEE_ATTACK_HIGH )
	DECLARE_ANIMEVENT( AE_AGRUNT_MELEE_ATTACK_LOW )
	DECLARE_ANIMEVENT( AE_AGRUNT_RANGE_ATTACK1 )
	DECLARE_ANIMEVENT( AE_SWAT_OBJECT )

	DECLARE_ACTIVITY( ACT_SWATLEFTMID )

	DEFINE_SCHEDULE
	(
		SCHED_MOVE_THROWITEM,

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
	)

	DEFINE_SCHEDULE
	(
		SCHED_THROW,
	
		"	Tasks"
		"		TASK_DELAY_THROW				3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_THROW_PHYSOBJ				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
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
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"

	)

AI_END_CUSTOM_NPC()