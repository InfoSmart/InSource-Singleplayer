//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC Grunt
//
// Soldado alienigena sacado de Black Mesa: Source
// En Apocalypse es un soldado de la empresa "malvada". �Tiene mucha vida!
// Inspiraci�n: "Tank" de Left 4 Dead
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
#include "in_player.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "entitylist.h"
#include "engine/ienginesound.h"

//#include "weapon_brickbat.h"
#include "ammodef.h"
//#include "grenade_spit.h"
//#include "grenade_brickbat.h"

#include "in_gamerules.h"
#include "in_utils.h"
#include "shake.h"

//#include "gib.h"
#include "physobj.h"
#include "collisionutils.h"
//#include "coordsize.h"
#include "vstdlib/random.h"
#include "movevars_shared.h"
//#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

//=========================================================
// Definici�n de comandos para la consola.
//=========================================================

ConVar sk_grunt_health		("sk_grunt_health",			"0", 0, "Salud del Grunt");
ConVar sk_grunt_dmg_high	("sk_grunt_dmg_high",		"0", 0, "Da�o causado por un golpe alto");
ConVar sk_grunt_dmg_low		("sk_grunt_dmg_low",		"0", 0, "Da�o causado por un golpe bajo");

ConVar sk_grunt_throw_delay		("sk_grunt_throw_delay",	"1",	0, "Segundos antes de poder lanzar otro objeto.");
ConVar sk_grunt_throw_dist		("sk_grunt_throw_dist",		"300",	0, "Distancia de lanzamiento.");
ConVar sk_grunt_throw_minmass	("sk_grunt_throw_minmass",	"200",	0, "Minimo peso que puede lanzar.");
ConVar sk_grunt_throw_maxmass	("sk_grunt_throw_maxmass",	"1000",	0, "M�ximo peso que puede lanzar.");
ConVar sk_grunt_throw_maxthink  ("sk_grunt_throw_maxthink", "5",	0, "Cantidad de tiempo que puede manter un objeto como lanzable");

//=========================================================
// Configuraci�n del NPC
//=========================================================

// Modelo
#define MODEL_BASE		"models/xenians/agrunt.mdl"

// �Qu� capacidades tendr�?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_MECH

// Color de renderizado
#define RENDER_COLOR	255, 255, 255, 255

// Distancia de visibilidad.
#define SEE_DIST		9000.0f

// Campo de visi�n
#define FOV				VIEW_FIELD_FULL

// Propiedades
// No disolverse (Con la bola de energ�a) - No morir con la super arma de gravedad.
#define EFLAGS			EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL

// Ataque cuerpo a cuerpo #1
#define MELEE_ATTACK1_DIST				130
#define MELEE_ATTACK1_IMPULSE			400
#define MELEE_ATTACK1_MIN_PUNCH			60
#define MELEE_ATTACK1_MAX_PUNCH			100

// Ataque cuerpo a cuerpo #2
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
	SCHED_CHANGE_POSITION,
	SCHED_CANNON_ATTACK
};

//=========================================================
// Tareas
//=========================================================
enum 
{
	TASK_GET_PATH_TO_PHYSOBJ = LAST_SHARED_TASK,
	TASK_THROW_PHYSOBJ,
	TASK_DELAY_THROW,
	TASK_CANNON_AIM,
	TASK_CANNON_FIRE
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

int AE_GRUNT_STEP_LEFT;
int AE_GRUNT_STEP_RIGHT;

int AE_SWAT_OBJECT;

//=========================================================
// Animaciones
//=========================================================

int CNPC_Grunt::ACT_SWATLEFTMID;
int CNPC_Grunt::ACT_AGRUNT_RAGE;

int ImpactEffectTexture = -1;

//=========================================================
// Guardado y definici�n de datos
//=========================================================

LINK_ENTITY_TO_CLASS( npc_grunt, CNPC_Grunt );

BEGIN_DATADESC( CNPC_Grunt )

	DEFINE_FIELD( m_fLastHurtTime,			FIELD_TIME ),
	DEFINE_FIELD( m_fNextAlertSound,		FIELD_TIME ),
	DEFINE_FIELD( m_fNextPainSound,			FIELD_TIME ),
	DEFINE_FIELD( m_fNextSuccessDance,		FIELD_TIME ),
	DEFINE_FIELD( m_fExpireThrow,			FIELD_TIME ),

	DEFINE_FIELD( m_fNextRangeAttack1,		FIELD_TIME ),

	DEFINE_FIELD( m_fNextThrow,				FIELD_TIME ),
	DEFINE_FIELD( m_ePhysicsEnt,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bPhysicsCanThrow,		FIELD_BOOLEAN ),

END_DATADESC()

//=========================================================
// Crear un nuevo Grunt
//=========================================================
void CNPC_Grunt::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	SetModel(MODEL_BASE);
	SetBloodColor(BLOOD);

	// Tama�o
	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Navegaci�n, estado f�sico y opciones extra.
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	
	SetRenderColor(RENDER_COLOR);
	SetDistLook(SEE_DIST);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth				= sk_grunt_health.GetFloat();
	m_NPCState				= NPC_STATE_IDLE;
	m_flFieldOfView			= FOV;

	// M�s informaci�n
	m_fNextThrow				= gpGlobals->curtime;		// Proxima vez que lanzaremos un objeto cercano.
	m_fNextAlertSound			= gpGlobals->curtime;		// Proxima vez que haremos el sonido de alerta.
	m_fNextPainSound			= gpGlobals->curtime;		// Proxima vez que haremos el sonido de dolor.
	m_fNextRangeAttack1			= gpGlobals->curtime + 30;	// Proxima vez que haremos un ataque con nuestra pistola biologica.
	m_fNextSuccessDance			= gpGlobals->curtime;		// Pr�xima vez que haremos el baile del �xito.
	m_fExpireThrow				= 0;						// Tiempo en el que expira el lanzamiento de un objeto.

	m_ePhysicsEnt				= NULL;
	m_bPhysicsCanThrow			= false;

#ifdef APOCALYPSE
	AttackTimer.Invalidate();
	AttackTimer.Start(BOSS_MINTIME_TO_ATTACK);
#endif

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFLAGS);

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en cach�.
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
	PrecacheScriptSound("NPC_Grunt.Fail");
	PrecacheScriptSound("NPC_Grunt.Yell");
	PrecacheScriptSound("NPC_Grunt.Step");

	PrecacheScriptSound("NPC_Strider.Shoot");
	PrecacheScriptSound("NPC_Strider.Charge");

	BaseClass::Precache();
}

//=========================================================
// Bucle de ejecuci�n de tareas.
//=========================================================
void CNPC_Grunt::Think()
{
	BaseClass::Think();

#ifdef APOCALYPSE

	// El modelo actual no env�a al c�digo cuando el Grunt camina, por lo que usaremos "IsMoving" para detectarlo.
	// Si se esta moviendo �hacer temblar el suelo!
	if ( IsMoving() )
		UTIL_ScreenShake(WorldSpaceCenter(), 5.0f, 2.0f, 1.0f, 600.0f, SHAKE_START);

	// Siempre conocer la ubicaci�n del jugador (nuestro enemigo favorito)
	if ( !GetEnemy() && !HasCondition(COND_CAN_THROW) )
	{
		CIN_Player *pPlayer = UTIL_GetRandomInPlayer();

		if ( !pPlayer || !pPlayer->IsAlive() )
			return;

		SetEnemy(pPlayer);
		UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
	}

	// Ha pasado mucho tiempo desde que golpeo a un jugador, si nadie lo esta viendo sencillamente eliminarlo.
	// Con esto podremos evitar que la m�sica (y el estado en el Director) se queden si el Jefe se ha quedado "bugeado" (Atorado en una pared o debajo del suelo)
	if ( AttackTimer.HasStarted() && AttackTimer.IsElapsed() && !UTIL_IsPlayersVisibleCone(this) )
		UTIL_RemoveImmediate(this);

#endif
}

//=========================================================
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_Grunt::Classify()
{
	return CLASS_GRUNT; 
}

//=========================================================
//=========================================================
Disposition_t CNPC_Grunt::IRelationType(CBaseEntity *pTarget)
{
	return BaseClass::IRelationType(pTarget);
}

//=========================================================
// Reproducir sonido al azar de descanso.
//=========================================================
void CNPC_Grunt::IdleSound()
{
	EmitSound("NPC_Grunt.Idle");
}

//=========================================================
// Reproducir sonido de dolor.
//=========================================================
void CNPC_Grunt::PainSound(const CTakeDamageInfo &info)
{
	if ( gpGlobals->curtime >= m_fNextPainSound )
	{
		EmitSound("NPC_Grunt.Pain");
		m_fNextPainSound = gpGlobals->curtime + random->RandomFloat(1.0, 3.0);
	}
}

//=========================================================
// Reproducir sonido de alerta.
//=========================================================
void CNPC_Grunt::AlertSound()
{
	if ( gpGlobals->curtime >= m_fNextAlertSound )
	{
		EmitSound("NPC_Grunt.Alert");
		m_fNextAlertSound = gpGlobals->curtime + random->RandomFloat(.5, 2.0);
	}
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Grunt::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("NPC_Grunt.Death");
}

//=========================================================
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
// Reproducir sonido de ataque fallido.
//=========================================================
void CNPC_Grunt::FailSound()
{
	EmitSound("NPC_Grunt.Fail");
}

//=========================================================
// Reproducir sonido de ataque �xitoso.
//=========================================================
void CNPC_Grunt::YellSound()
{
	EmitSound("NPC_Grunt.Yell");
}

//=========================================================
// Devuelve la velocidad m�xima del yaw dependiendo de la
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
// Ejecuta una acci�n al momento que el modelo hace
// la animaci�n correspondiente.
//=========================================================
void CNPC_Grunt::HandleAnimEvent(animevent_t *pEvent)
{
	//const char *pName = EventList_NameForIndex(pEvent->event);
	//DevMsg("[GRUNT] Se ha producido el evento %s \n", pName);

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

	// Lanzando objeto.
	if ( pEvent->event == AE_SWAT_OBJECT )
	{
		CBaseEntity *pEnemy = GetEnemy();

		// No tenemos ning�n enemigo a quien lanzar.
		if ( !pEnemy )
			return;
		
		Vector v;
		CBaseEntity *pPhysicsEntity = m_ePhysicsEnt;

		if ( !pPhysicsEntity )
			return;

		IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

		if ( !pPhysObj )
			return;

		PhysicsImpactSound(pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 1800);

		Vector PhysicsCenter	= pPhysicsEntity->WorldSpaceCenter();
		v						= pEnemy->WorldSpaceCenter() - PhysicsCenter;

		VectorNormalize(v);

		v	= v * 1800;
		v.z += 400;

		AngularImpulse AngVelocity(random->RandomFloat(-180, 180), 20, random->RandomFloat(-360, 360));
		pPhysObj->AddVelocity(&v, &AngVelocity);

		m_ePhysicsEnt	= NULL;
		m_fNextThrow	= gpGlobals->curtime + sk_grunt_throw_delay.GetFloat();
		m_fExpireThrow  = 0;

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// Calcula si el salto que realizar� es v�lido.
//=========================================================
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

	CBaseEntity *pVictim	= NULL;
	float pDamage			= ( highAttack ) ? sk_grunt_dmg_high.GetFloat() : sk_grunt_dmg_low.GetFloat();
	int pTypeDamage			= DMG_SLASH | DMG_ALWAYSGIB;

	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs() + 5;
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	int Melee_Attack_Dist		= ( highAttack ) ? MELEE_ATTACK1_DIST : MELEE_ATTACK2_DIST;
	int Melee_Attack_Impulse	= ( highAttack ) ? MELEE_ATTACK1_IMPULSE : MELEE_ATTACK2_IMPULSE;
	int Melee_Attack_Min_Punch	= ( highAttack ) ? MELEE_ATTACK1_MIN_PUNCH : MELEE_ATTACK2_MIN_PUNCH;
	int Melee_Attack_Max_Punch	= ( highAttack ) ? MELEE_ATTACK1_MAX_PUNCH : MELEE_ATTACK2_MAX_PUNCH;

	// Siempre debemos atacar a un Bullseye
	if ( GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		CTakeDamageInfo info(this, this, vec3_origin, GetAbsOrigin(), pDamage, pTypeDamage);
		GetEnemy()->TakeDamage(info);

		// Grrr!
		AttackSound(highAttack);
		return;
	}

	// Es un aliado del jugador.
	if ( GetEnemy()->Classify() == CLASS_PLAYER_ALLY || GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		CTakeDamageInfo info(this, this, vec3_origin, GetAbsOrigin(), (pDamage / 2), pTypeDamage);
		GetEnemy()->TakeDamage(info);

		Vector forward, up;
		AngleVectors(GetAbsAngles(), &forward, NULL, &up);
		Vector pImpulse = Melee_Attack_Impulse * (up + 2 * forward);

		// Lanzamos por los aires
		GetEnemy()->ApplyAbsVelocityImpulse(pImpulse);

		// Grrr!
		AttackSound(highAttack);
		return;
	}

	bool pAllVictims			= false;
	CBaseEntity *pLastVictim	= NULL;

	int pTrys		= 0; // Intentos.
	int pVictims	= 0; // Victimas

	// No hemos golpeado a todos los enemigos.
	while ( !pAllVictims )
	{
		// Obtenemos al desafortunado que he matado...
		// Le establecemos 0 de da�o para que no mate a los NPC antes de lanzarlos. (El da�o se lo proporcionamos manualmente)
		pVictim = CheckTraceHullAttack(Melee_Attack_Dist, vecMins, vecMaxs, 0, pTypeDamage, 1.0, true);
		++pTrys;

		// 5 intentos m�ximo.
		if ( pTrys >= 5 )
		{
			pAllVictims = true;
			break;
		}

		// �Le he dado!
		if ( pVictim && pVictim->IsAlive() )
		{
			if ( pVictim == pLastVictim )
				continue;

			++pVictims;
			pLastVictim = pVictim;

			// Grrr!
			AttackSound(highAttack);
		
			Vector forward, up;
			AngleVectors(GetAbsAngles(), &forward, NULL, &up);

			Vector pImpulse = Melee_Attack_Impulse * (up + 2 * forward);

			// Nuestra victima es un jugador.
			if ( pVictim->IsPlayer() )
			{
				int pPunch = random->RandomInt(Melee_Attack_Min_Punch, Melee_Attack_Max_Punch);

#ifdef APOCALYPSE
				// Reiniciar el cronometro.
				AttackTimer.Start(BOSS_MINTIME_TO_ATTACK);
#endif

				// Desorientar
				pVictim->ViewPunch(QAngle(pPunch, 0, -pPunch));

				// Lanzarlo por los aires.
				pVictim->ApplyAbsVelocityImpulse(pImpulse);

				// Da�o
				CTakeDamageInfo damage;
				damage.SetAttacker(this);
				damage.SetInflictor(this);
				damage.SetDamage(pDamage);
				damage.SetDamageType(pTypeDamage);

				pVictim->TakeDamage(damage);

#ifdef APOCALYPSE
				CIN_Player *pPlayer = ToInPlayer(pVictim);

				// El jugador tiene un arma y no esta oculta.
				// adem�s no esta en dificultad f�cil.
				if ( pPlayer && pPlayer->GetActiveWeapon() && pPlayer->GetActiveWeapon()->IsWeaponVisible() && !GameRules()->IsSkillLevel(SKILL_EASY) )
				{
						// !!!REFERENCE
						// En Left4Dead cuando un Tank avienta por los aires a un jugador el mismo
						// "desactiva" su arma hasta que cae, despu�s se crea la animaci�n de levantar y activar
						// el arma. (Mientras esta desactivada no se puede disparar)

						// Ocultar el arma del jugador.
						// FIXME 2: Si el arma ya esta "oculta" el juego lanza una excepci�n (Se va al carajo...)
						
						//DevMsg("[GRUNT] pPUNCH: %i \r\n", pPunch);

						// El lanzamiento fue muy poderoso, hacer que el jugador suelte el arma.
						// FIXME: Si el jugador al momento de soltar el arma tenia 100 balas de un m�ximo de 200
						// al recojer el arma su munici�n se restaura a 200. (Balas gratis)
						// FIXME2: En ocaciones el arma no es quitada �raz�n? y ocaciona un crash
						//if ( pPunch > 80 && random->RandomInt(0, 1) == 1 )
							//pPlayer->GetActiveWeapon()->Drop(pImpulse * 1.5);
						//else
						//{
						pPlayer->m_bGruntAttack = true;
						pPlayer->GetActiveWeapon()->Holster();
						//}
					
				}
#endif
			}

			// Nuestra victima es un NPC (o algo as�...)
			else
			{
				// Los Grunt son nuestros amigos.
				if ( pVictim->Classify() == CLASS_GRUNT )
					continue;

				// Lanzamos por los aires
				pVictim->ApplyAbsVelocityImpulse(pImpulse);

				// �GRRR! Quitense de mi camino. (Matamos al NPC)
				CTakeDamageInfo damage;
				damage.SetAttacker(this);
				damage.SetInflictor(this);
				damage.SetDamage(300);
				damage.SetDamageType(pTypeDamage);

				pVictim->TakeDamage(damage);
			}
		}
		else
		{
			// Todos los enemigos han sigo golpeados.
			pAllVictims = true;
		}
	}

	// He fallado...
	if ( pVictims == 0 )
		FailSound();

	// Jaja, chupate esa!
	else
	{
		// �Baile del �xito!
		if ( m_fNextSuccessDance <= gpGlobals->curtime )
		{
			YellSound();
			SetIdealActivity((Activity)ACT_AGRUNT_RAGE);

			m_fNextSuccessDance = gpGlobals->curtime + random->RandomInt(5, 15); // Hacerlo de nuevo en 5 a 15 segs.
		}
	}
}

//=========================================================
// Dispara el ca�on biologico de materia oscura.
//=========================================================
void CNPC_Grunt::FireCannon()
{
	// Aun no toca...
	if ( gpGlobals->curtime <= m_fNextRangeAttack1 )
		return;

	trace_t tr;
	Vector vecShootPos;

	// Obtenemos la ubicaci�n de la boquilla de nuestra arma.
	GetAttachment(LookupAttachment("muzzle"), vecShootPos);

	Vector vecShootDir;
	vecShootDir	= GetEnemy()->WorldSpaceCenter() - vecShootPos;

	Vector m_blastHit;
	Vector m_blastNormal;

	float flDist = VectorNormalize(vecShootDir);

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

	CreateConcussiveBlast(m_blastHit, m_blastNormal, this, 0.1);
	m_fNextRangeAttack1 = gpGlobals->curtime + random->RandomFloat(15.0, 30.0);
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe alto
//=========================================================
int CNPC_Grunt::MeleeAttack1Conditions(float flDot, float flDist)
{
	// El Grunt es muy poderoso, no podemos matar a los personajes vitales.
	if ( GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL || HasCondition(COND_CAN_THROW) )
		return COND_NONE;
	
	// Distancia y angulo correcto, �ataque!
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

	if ( tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt))) )
		return COND_CAN_MELEE_ATTACK1;

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
		return COND_CAN_MELEE_ATTACK1;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe bajo
//=========================================================
int CNPC_Grunt::MeleeAttack2Conditions(float flDot, float flDist)
{
	if ( HasCondition(COND_CAN_THROW) )
		return COND_NONE;

	if ( flDist <= 50 && flDot >= 0.7 )
		return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// Verifica si es conveniente hacer un ataque a distancia.
// En este caso: Ca�on biologico de materia oscura.
//=========================================================
int CNPC_Grunt::RangeAttack1Conditions(float flDot, float flDist)
{
	// Automaticamente esto se traduce a TASK_CANNON_AIM y TASK_CANNON_FIRE

	// A�n no toca disparar nuestro ca�on
	if ( gpGlobals->curtime <= m_fNextRangeAttack1 )
		return COND_NONE;

	// Perfecta distancia.
	if ( flDist <= 800 && flDot >= 0.7 )
		return COND_CAN_RANGE_ATTACK1;

	// Muy lejos.
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
// 
//=========================================================
int CNPC_Grunt::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// Muerte del NPC.
//=========================================================
void CNPC_Grunt::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);
}

//=========================================================
// 
//=========================================================
void CNPC_Grunt::GatherConditions()
{
	BaseClass::GatherConditions();

	// Toca lanzar un objeto, buscamos uno.
	if ( gpGlobals->curtime >= m_fNextThrow && m_ePhysicsEnt == NULL )
	{
		// �Hemos encontrado un objeto!
		if ( FindNearestPhysicsObject() )
		{
			// Si nos estamos moviendo cancelar el objetivo para ir a la ubicaci�n del objeto.
			if ( IsMoving() )
			{
				GetNavigator()->StopMoving();
				GetNavigator()->ClearGoal();
			}

			m_fExpireThrow = gpGlobals->curtime + sk_grunt_throw_maxthink.GetInt();
		}
	}

	// Ya tenemos al objeto a lanzar.
	if ( m_ePhysicsEnt != NULL )
	{
		// Al parecer algo salio mal y no pudimos lanzar el objeto.
		if ( m_fExpireThrow <= gpGlobals->curtime && m_fExpireThrow != 0 )
		{
			m_fExpireThrow	= 0;
			m_ePhysicsEnt	= NULL;

			return;
		}

		SetCondition(COND_CAN_THROW);
	}
	else
		ClearCondition(COND_CAN_THROW);
}

//=========================================================
// Selecciona una tarea programada dependiendo del combate
//=========================================================
int CNPC_Grunt::SelectCombatSchedule()
{
	// Iniciar la ejecuci�n para lanzar un objeto.
	if ( HasCondition(COND_CAN_THROW) )
		return SCHED_THROW;

	// Hemos perdido al enemigo, buscarlo o perseguirlo.
	if ( HasCondition(COND_ENEMY_OCCLUDED) || HasCondition(COND_ENEMY_TOO_FAR) || HasCondition(COND_TOO_FAR_TO_ATTACK) || HasCondition(COND_NOT_FACING_ATTACK) )
		return SCHED_CHASE_ENEMY;

	return BaseClass::SelectSchedule();
}

//=========================================================
// Selecciona una tarea programada dependiendo del estado
//=========================================================
int CNPC_Grunt::SelectSchedule()
{
	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	// Hora de usar nuestro ca�on.
	//if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
		//return SCHED_CANNON_ATTACK;

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
int CNPC_Grunt::TranslateSchedule(int scheduleType)
{
	switch ( scheduleType )
	{
		// Lanzar un objeto: Lanzarlo o movernos a la ubicaci�n del mismo.
		case SCHED_THROW:
			if ( DistToPhysicsEnt() > sk_grunt_throw_dist.GetFloat() )
				return SCHED_MOVE_THROWITEM;
			else
				return SCHED_THROW;
		break;

		// RANGE_ATTACK es lo mismo a nuestro ca�on.
		case SCHED_RANGE_ATTACK1:
			return SCHED_CANNON_ATTACK;
		break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

//=========================================================
// 
//=========================================================
NPC_STATE CNPC_Grunt::SelectIdealState()
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
void CNPC_Grunt::StartTask(const Task_t *pTask)
{
	switch ( pTask->iTask )
	{
		// Retrasamos lanzamiento de un objeto.
		case TASK_DELAY_THROW:
		{
			m_fNextThrow = gpGlobals->curtime + pTask->flTaskData;
			TaskComplete();

			break;
		}

		// Obtenemos la ruta hacia el objeto a lanzar.
		case TASK_GET_PATH_TO_PHYSOBJ:
		{
			if ( m_ePhysicsEnt == NULL )
				TaskFail("No hay ningun objeto para lanzar");

			Vector vecGoalPos;
			Vector vecDir;

			vecDir		= GetLocalOrigin() - m_ePhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z	= 0;

			AI_NavGoal_t goal( m_ePhysicsEnt->WorldSpaceCenter() );
			goal.pTarget = m_ePhysicsEnt;
			GetNavigator()->SetGoal(goal);

			TaskComplete();
			break;
		}

		// �Lanzar objeto!
		case TASK_THROW_PHYSOBJ:
		{
			if ( m_ePhysicsEnt == NULL )
				TaskFail("No hay ningun objeto para lanzar");

			else if ( DistToPhysicsEnt() > sk_grunt_throw_dist.GetFloat() )
				TaskFail("El objeto ya no esta a mi alcanze");

			else
				SetIdealActivity((Activity)ACT_SWATLEFTMID);

			break;
		}

		// Reproducir sonido de carga del ca�on.
		case TASK_CANNON_AIM:
		{
			SetWait(pTask->flTaskData);

			Vector vecShootPos;
			GetAttachment(LookupAttachment("muzzle"), vecShootPos);

			EntityMessageBegin( this, true );
				WRITE_BYTE( 2 );
				WRITE_VEC3COORD( vecShootPos );
			MessageEnd();

			CPASAttenuationFilter filter2(this, "NPC_Strider.Charge");
			EmitSound(filter2, entindex(), "NPC_Strider.Charge");

			break;
		}

		// ��Disparar ca�on!!
		case TASK_CANNON_FIRE:
		{
			FireCannon();
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
//
//=========================================================
void CNPC_Grunt::RunTask(const Task_t *pTask)
{
	switch ( pTask->iTask )
	{
		// Lanzar objeto.
		case TASK_THROW_PHYSOBJ:

			// Actividad finalizada.
			if ( IsActivityFinished() )
				TaskComplete();

		break;

		// Reproducir sonido de carga del ca�on.
		case TASK_CANNON_AIM:

			// El tiempo de espera ha acabado.
			if ( IsWaitFinished() )
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

//=========================================================
// Trata de encontrar el objeto "lanzable" m�s cercano.
//=========================================================
bool CNPC_Grunt::FindNearestPhysicsObject()
{
	// No tenemos a ningun enemigo a quien lanzar.
	if ( !GetEnemy() )
	{
		m_ePhysicsEnt = NULL;
		return false;
	}

	CBaseEntity *pFinalEntity	= NULL;
	CBaseEntity *pThrowEntity	= NULL;
	float flNearestDist			= 0;

	// Buscamos los objetos que podemos lanzar.
	do
	{
		// Objetos con f�sicas.
		pThrowEntity = gEntList.FindEntityByClassnameWithin(pThrowEntity, "prop_physics", GetAbsOrigin(), sk_grunt_throw_dist.GetFloat());
	
		// Ya no existe.
		if ( !pThrowEntity )
			continue;

		// No lo podemos ver.
		if ( !FVisible(pThrowEntity) )
			continue;

		// No se ha podido acceder a la informaci�n de su fisica.
		if ( !pThrowEntity->VPhysicsGetObject() )
			continue;

		// No se puede mover o en si.. lanzar.
		if ( !pThrowEntity->VPhysicsGetObject()->IsMoveable() )
			continue;

		Vector v_center	= pThrowEntity->WorldSpaceCenter();
		float flDist	= UTIL_DistApprox2D(GetAbsOrigin(), v_center);

		// Esta m�s lejos que el objeto anterior.
		if ( flDist > flNearestDist && flNearestDist != 0 )
			continue;

		// Calcular la distancia al enemigo.
		Vector vecDirToEnemy	= GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
		float dist				= VectorNormalize(vecDirToEnemy);
		vecDirToEnemy.z			= 0;

		// Este objeto esta muy cerca... pero �esta entre el npc y el enemigo?
		Vector vecDirToObject = pThrowEntity->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		// Obtenemos su peso.
		float pEntityMass = pThrowEntity->VPhysicsGetObject()->GetMass();

		// Muy liviano.
		if ( pEntityMass < sk_grunt_throw_minmass.GetFloat() )
			continue;
			
		// �Muy pesado!
		if ( pEntityMass > sk_grunt_throw_maxmass.GetFloat() )
			continue;

		if ( DotProduct(vecDirToEnemy, vecDirToObject) < 0.8 )
			continue;

		// No lanzar objetos que esten sobre mi cabeza.
		if ( v_center.z > EyePosition().z )
			continue;

		Vector vecGruntKnees;
		CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.25f), &vecGruntKnees);

		vcollide_t *pCollide = modelinfo->GetVCollide(pThrowEntity->GetModelIndex());
		
		Vector objMins, objMaxs;
		physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pThrowEntity->GetAbsOrigin(), pThrowEntity->GetAbsAngles());

		if ( objMaxs.z < vecGruntKnees.z )
			continue;

		// Este objeto es perfecto, guardamos su distancia por si encontramos otro m�s cerca.
		flNearestDist	= flDist;
		pFinalEntity	= pThrowEntity;

	} while(pThrowEntity);

	// No pudimos encontrar ning�n objeto.
	if ( !pFinalEntity )
	{
		m_ePhysicsEnt = NULL;
		return false;
	}

	m_ePhysicsEnt = pFinalEntity;
	return true;
}

float CNPC_Grunt::DistToPhysicsEnt()
{
	if ( m_ePhysicsEnt != NULL )
		return UTIL_DistApprox2D(GetAbsOrigin(), m_ePhysicsEnt->WorldSpaceCenter());

	return sk_grunt_throw_dist.GetFloat() + 1;
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
	DECLARE_TASK( TASK_CANNON_AIM )
	DECLARE_TASK( TASK_CANNON_FIRE )

	DECLARE_CONDITION( COND_CAN_THROW )
	DECLARE_CONDITION( COND_MELEE_OBSTRUCTION )

	DECLARE_ANIMEVENT( AE_AGRUNT_MELEE_ATTACK_HIGH )
	DECLARE_ANIMEVENT( AE_AGRUNT_MELEE_ATTACK_LOW )
	DECLARE_ANIMEVENT( AE_AGRUNT_RANGE_ATTACK1 )
	DECLARE_ANIMEVENT( AE_GRUNT_STEP_LEFT )
	DECLARE_ANIMEVENT( AE_GRUNT_STEP_RIGHT )
	DECLARE_ANIMEVENT( AE_SWAT_OBJECT )

	DECLARE_ACTIVITY( ACT_SWATLEFTMID )
	DECLARE_ACTIVITY( ACT_AGRUNT_RAGE )

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
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ENEMY_DEAD"
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
		"		COND_NEW_ENEMY"

	)

	DEFINE_SCHEDULE
	(
		SCHED_CANNON_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_SET_ACTIVITY							ACTIVITY:ACT_IDLE"
		"		TASK_WAIT									1"
		"		TASK_CANNON_AIM								1.25"
		"		TASK_CANNON_FIRE							0"
		"		TASK_WAIT									1"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"

	)

AI_END_CUSTOM_NPC()