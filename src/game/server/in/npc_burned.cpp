//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BURNED
//
// Zombi quemado capaz de derribarte con 1 golpe.
// Inspiración: WITCH de Left 4 Dead
//
//=====================================================================================//

#include "cbase.h"
#include "npc_burned.h"

#include "npcevent.h"
#include "activitylist.h"

#include "in_utils.h"
#include "physobj.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Comandos de consola.
//=========================================================

ConVar sk_burned_health("sk_burned_health", "300", FCVAR_CHEAT, "Define la salud del Burned.");
ConVar sk_burned_debug("sk_burned_debug", "0", 0, "");

//=========================================================
// Configuración del NPC
//=========================================================

// Modelo
#define BURNED_MODEL			"models/player/charple.mdl"		// Algo mejor

// ¿Qué capacidades tendrá?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define BURNED_CAPABILITIES			
#define BURNED_IRKED_CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB

// Color de la sangre.
#define BURNED_BLOOD			BLOOD_COLOR_RED

// Distancia de visibilidad.
#define BURNED_SEE_DIST			9000.0f

// Campo de visión
#define BURNED_FOV				VIEW_FIELD_FULL

// Propiedades
// No morir con la super arma de gravedad.
#define BURNED_EFLAGS			EFL_NO_MEGAPHYSCANNON_RAGDOLL

//=========================================================
// Animaciones
//=========================================================


//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( npc_burned, CNPC_Burned );

BEGIN_DATADESC( CNPC_Burned )
END_DATADESC()

//=========================================================
// Crear un nuevo NPC
//=========================================================
void CNPC_Burned::Spawn()
{
	Precache();

	// Modelo y color de sangre.
	SetModel(BURNED_MODEL);
	SetBloodColor(BURNED_BLOOD);

	// Tamaño
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);

	SetDistLook(BURNED_SEE_DIST);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth			= sk_burned_health.GetFloat();
	m_NPCState			= NPC_STATE_IDLE;
	m_flFieldOfView		= BURNED_FOV;

	// Caracteristicas
	AddEFlags(BURNED_EFLAGS);

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Inicializa los parámetros del NPC
//=========================================================
void CNPC_Burned::NPCInit()
{
	BaseClass::NPCInit();

	// ¿Ha sido enfadado?
	// @TODO: Definir si estará enfadado al iniciar.
	IsIrked		= false;

	// Capacidades
	CapabilitiesClear();

	// No podrá hacer nada hasta que se encuentre enfadado.
	if ( IsIrked )
		CapabilitiesAdd(BURNED_IRKED_CAPABILITIES);
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CNPC_Burned::Precache()
{
	// Modelo
	PrecacheModel(BURNED_MODEL);

	BaseClass::Precache();
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CNPC_Burned::Think()
{
	BaseClass::Think();

	// No me han enfadado.
	if ( !IsIrked )
	{
		// ¡Algun jugador me esta mirando!
		if ( UTIL_IsPlayersVisible(GetAbsOrigin()) )
		{
			if ( sk_burned_debug.GetBool() )
				DevMsg("[BURNED] Alguien me esta mirando... \r\n");

			// Te doy 5 segundos para dejar de mirarme....
			if ( !DetectIrkedTime.HasStarted() )
				DetectIrkedTime.Start(5);

			// ¡¡ TE DIJE QUE DEJARAS DE MIRARME !!
			if ( DetectIrkedTime.HasStarted() && DetectIrkedTime.IsElapsed() )
				StartIrk();
		}
		else
			DetectIrkedTime.Invalidate();
	}
	else if ( IsIrked && !GetEnemy() )
	{
		float flDistance = 0.0f;
		CIN_Player *pPlayer = UTIL_GetNearestInPlayer(GetAbsOrigin(), flDistance);

		// Tu, te matare.
		if ( pPlayer )
		{
			SetEnemy(pPlayer);
			UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
		}
	}
}

//=========================================================
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T CNPC_Burned::Classify()
{
	if ( IsIrked )
		return CLASS_BURNED;
	else
		return CLASS_NONE;
}

//=========================================================
// ¡¡¡ ARGHH !!!
//=========================================================
void CNPC_Burned::StartIrk()
{
	DevMsg("[BURNED] ME HE FASTIDIADO !!!! \r\n");

	DetectIrkedTime.Invalidate();
	IsIrked	= true;

	CapabilitiesClear();
	CapabilitiesAdd(BURNED_IRKED_CAPABILITIES);
}

//=========================================================
// Al tomar daño y aún estar vivo.
//=========================================================
int CNPC_Burned::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	// ARGH!!!
	if ( !IsIrked )
		StartIrk();

	return BaseClass::OnTakeDamage_Alive(info);
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_Burned::HandleAnimEvent(animevent_t *pEvent)
{
	const char *pName = EventList_NameForIndex(pEvent->event);
	DevMsg("[BURNED] Se ha producido el evento %s \n", pName);

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CNPC_Burned::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 220.0f;
	const float MAX_JUMP_DISTANCE	= 512.0f;
	const float MAX_JUMP_DROP		= 384.0f;

	return BaseClass::IsJumpLegal(startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE);
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe alto
//=========================================================
int CNPC_Burned::MeleeAttack1Conditions(float flDot, float flDist)
{
	// No estamos enfadados.
	if ( !IsIrked )
		return COND_NONE;

	// El Burn es muy poderoso. No hacer nada en los aliados vitales.
	if ( GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL )
		return COND_NONE;

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

Activity pWalkAnims[] =
{
	ACT_HL2MP_WALK_ZOMBIE_01,
	ACT_HL2MP_WALK_ZOMBIE_02,
	ACT_HL2MP_WALK_ZOMBIE_03,
	ACT_HL2MP_WALK_ZOMBIE_04,
	ACT_HL2MP_WALK_ZOMBIE_05,
	ACT_HL2MP_WALK_ZOMBIE_06
};

//=========================================================
// Traduce una actividad a otra.
//=========================================================
Activity CNPC_Burned::NPC_TranslateActivity(Activity baseAct)
{
	// Mientras no estemos enfadados siempre será esta animación.
	if ( !IsIrked )
		return ACT_GMOD_SHOWOFF_DUCK_02;

	switch ( baseAct )
	{
		case ACT_WALK:
			return pWalkAnims[RandomInt(0, ARRAYSIZE(pWalkAnims)-1)];
		break;

		case ACT_RUN:
			return ACT_HL2MP_RUN_ZOMBIE;
		break;

		case ACT_IDLE:
			return ACT_HL2MP_IDLE_ZOMBIE;
		break;

		case ACT_MELEE_ATTACK1:
			return ACT_HL2MP_RUN_ZOMBIE; // @TODO: 
		break;
	}

	return BaseClass::NPC_TranslateActivity(baseAct);
}