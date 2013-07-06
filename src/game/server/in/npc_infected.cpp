//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Infectado
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "game.h"

#include "npcevent.h"
#include "activitylist.h"

#include "npc_infected.h"
#include "in_player.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "physobj.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de comandos para la consola.
//=========================================================

ConVar sk_infected_health("sk_infected_health", "10", 0, "Define la salud de los infectados");

//=========================================================
// Configuración del NPC
//=========================================================

// ¿Qué capacidades tendrá?
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_MOVE_JUMP

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_RED

// Color de renderizado
#define RENDER_COLOR	255, 255, 255, 255

// Distancia de visibilidad.
#define SEE_DIST		10.0f

// Campo de visión
#define FOV				0.5f

//=========================================================
// Modelos
//=========================================================

static const char *pModels[] =
{
	"classic.mdl",

	"zombie_guard.mdl"
	"zombie_sci.mdl"
};

//=========================================================
// Tareas programadas
//=========================================================

//=========================================================
// Tareas
//=========================================================

//=========================================================
// Condiciones
//=========================================================

//=========================================================
// Eventos de animaciones
//=========================================================

//=========================================================
// Animaciones
//=========================================================

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( npc_infected, CInfected );

BEGIN_DATADESC( CInfected )

END_DATADESC()

//=========================================================
// Crear un nuevo Infectado
//=========================================================
void CInfected::Spawn()
{
	Precache();

	int pRandomModel = random->RandomInt(0, ARRAYSIZE(pModels) - 1);

	// Modelo y color de sangre.
	SetModel(pModels[pRandomModel]);
	SetBloodColor(BLOOD);

	// Tamaño
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Navegación, estado físico y opciones extra.
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);

	SetRenderColor(RENDER_COLOR);
	SetDistLook(SEE_DIST);

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd(CAPABILITIES);

	// Reseteo de variables.
	// Salud, estado del NPC y vista.
	m_iHealth		= sk_infected_health.GetFloat();
	m_NPCState		= NPC_STATE_IDLE;
	m_flFieldOfView = FOV;

	NPCInit();
	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CInfected::Precache()
{
	for ( int i = 0; i <= ARRAYSIZE(pModels) - 1; ++i )
	{
		PrecacheModel(CFmtStr("models/zombie/%s", pModels[i]));
		//Msg("Guardando en cache: %s \r\n", CFmtStr("models/zombie/%s", pModels[i]));
	}

	BaseClass::Precache();
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CInfected::Think()
{
}

//=========================================================
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T CInfected::Classify()
{
	return CLASS_INFECTED;
}

//=========================================================
// Reproducir sonido al azar de descanso.
//=========================================================
void CInfected::IdleSound()
{
	EmitSound("Infected.Idle");
}

//=========================================================
// Reproducir sonido de dolor.
//=========================================================
void CInfected::PainSound(const CTakeDamageInfo &info)
{
	//if ( gpGlobals->curtime >= m_fNextPainSound )
	{
		EmitSound("Infected.Pain");
		//m_fNextPainSound = gpGlobals->curtime + random->RandomFloat(1.0, 3.0);
	}
}

//=========================================================
// Reproducir sonido de alerta.
//=========================================================
void CInfected::AlertSound()
{
	//if ( gpGlobals->curtime >= m_fNextAlertSound )
	{
		EmitSound("Infected.Alert");
		//m_fNextAlertSound = gpGlobals->curtime + random->RandomFloat(.5, 2.0);
	}
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CInfected::DeathSound(const CTakeDamageInfo &info)
{
	EmitSound("Infected.Death");
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CInfected::HandleAnimEvent(animevent_t *pEvent)
{
	const char *pName = EventList_NameForIndex(pEvent->event);
	DevMsg("[INFECTADO] Se ha producido el evento %s \n", pName);
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CInfected::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 20.0f;
	const float MAX_JUMP_DISTANCE	= 80.0f;
	const float MAX_JUMP_DROP		= 40.0f;

	return BaseClass::IsJumpLegal(startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE);
}

//=========================================================
// Ataque cuerpo a cuerpo.
//=========================================================
void CInfected::MeleeAttack(bool highAttack)
{
}

//=========================================================
// Verifica si es conveniente hacer un ataque cuerpo a cuerpo.
// En este caso: Golpe alto
//=========================================================
int CInfected::MeleeAttack1Conditions(float flDot, float flDist)
{	
	// Distancia y angulo correcto, ¡ataque!
	if ( flDist <= 20 && flDot >= 0.7 )
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
int CInfected::MeleeAttack2Conditions(float flDot, float flDist)
{
	if ( flDist <= 30 && flDot >= 0.7 )
		return COND_CAN_MELEE_ATTACK2;
	
	return COND_TOO_FAR_TO_ATTACK;
}