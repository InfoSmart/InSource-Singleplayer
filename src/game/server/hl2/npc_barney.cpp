//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de comandos para la consola.
//=========================================================

ConVar sk_barney_health("sk_barney_health", "0");

//=========================================================
// Configuración del NPC
//=========================================================

// Modelo
#define MODEL_BASE	"models/barney.mdl"

// ¿Qué capacidades tendrá?
// Saltar
#define CAPABILITIES	bits_CAP_MOVE_JUMP

// Color de la sangre.
#define BLOOD			BLOOD_COLOR_RED

// Color de renderizado
#define RENDER_COLOR	255, 255, 255, 255

// Distancia de visibilidad.
//#define SEE_DIST		9000.0f

// Campo de visión
//#define FOV				VIEW_FIELD_FULL

// Propiedades
// No disolverse (Con la bola de energía) - No morir con la super arma de gravedad.
#define EFLAGS			EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION

//=========================================================
// CNPC_Barney
//=========================================================
class CNPC_Barney : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS(CNPC_Barney, CNPC_PlayerCompanion);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel();
		BaseClass::Precache();

		PrecacheScriptSound( "NPC_Barney.FootstepLeft" );
		PrecacheScriptSound( "NPC_Barney.FootstepRight" );
		PrecacheScriptSound( "NPC_Barney.Die" );

		PrecacheInstancedScene( "scenes/Expressions/BarneyIdle.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyAlert.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyCombat.vcd" );
	}

	void	Spawn();
	void	SelectModel();
	Class_T Classify();

	bool	IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	void	Weapon_Equip(CBaseCombatWeapon *pWeapon);

	bool CreateBehaviors();
	void HandleAnimEvent(animevent_t *pEvent);

	bool ShouldLookForBetterWeapon() { return false; }

	void OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior);

	void DeathSound(const CTakeDamageInfo &info);
	void GatherConditions();
	void UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS(npc_barney, CNPC_Barney);

//=========================================================
// SERVERCLASS
//=========================================================
IMPLEMENT_SERVERCLASS_ST(CNPC_Barney, DT_NPC_Barney)
END_SEND_TABLE()


//=========================================================
// Guardado/Restauración de informarción.
//=========================================================
BEGIN_DATADESC( CNPC_Barney )
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_USEFUNC( UseFunc ),
END_DATADESC()

//=========================================================
// Selecciona el modelo.
//=========================================================
void CNPC_Barney::SelectModel()
{
	SetModelName(AllocPooledString(MODEL_BASE));
}

//=========================================================
// Crear un nuevo NPC
//=========================================================
void CNPC_Barney::Spawn()
{
	Precache();

	// Color de sangre
	SetBloodColor(BLOOD);

	// Renderizado
	SetRenderColor(RENDER_COLOR);
	//SetDistLook(SEE_DIST);

	// Reseteo de variables.
	m_iHealth	= sk_barney_health.GetInt();
	//m_NPCState	= NPC_STATE_IDLE;

	m_iszIdleExpression		= MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	m_iszAlertExpression	= MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	m_iszCombatExpression	= MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	// Capacidades
	CapabilitiesAdd(CAPABILITIES);

	// Caracteristicas
	AddEFlags(EFLAGS);
	
	BaseClass::Spawn();
	NPCInit();
	
	SetUse(&CNPC_Barney::UseFunc);
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
/*
void CNPC_Barney:Precache()
{
		// Prevents a warning
		SelectModel();
		BaseClass::Precache();

		PrecacheScriptSound("NPC_Barney.FootstepLeft");
		PrecacheScriptSound("NPC_Barney.FootstepRight");
		PrecacheScriptSound("NPC_Barney.Die");

		PrecacheInstancedScene("scenes/Expressions/BarneyIdle.vcd");
		PrecacheInstancedScene("scenes/Expressions/BarneyAlert.vcd");
		PrecacheInstancedScene("scenes/Expressions/BarneyCombat.vcd");
	}
	*/

//=========================================================
// Devuelve el tipo de NPC.
// Con el fin de usarse en la tabla de relaciones.
//=========================================================
Class_T	CNPC_Barney::Classify()
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CNPC_Barney::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 220.0f;
	const float MAX_JUMP_DISTANCE	= 512.0f;
	const float MAX_JUMP_DROP		= 384.0f;

	return BaseClass::IsJumpLegal(startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE);
}

//=========================================================
// Equipa un arma.
//=========================================================
void CNPC_Barney::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip(pWeapon);
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CNPC_Barney::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		// Paso izquierdo.
		case NPC_EVENT_LEFTFOOT:
		{
			EmitSound("NPC_Barney.FootstepLeft", pEvent->eventtime);
		}
		break;

		// Paso derecho.
		case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound("NPC_Barney.FootstepRight", pEvent->eventtime);
		}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Sonido al morir.
//=========================================================
void CNPC_Barney::DeathSound( const CTakeDamageInfo &info )
{
	// Ya no reproducir sentencias.
	SentenceStop();

	EmitSound("npc_barney.die");

}

//=========================================================
//=========================================================
bool CNPC_Barney::CreateBehaviors()
{
	BaseClass::CreateBehaviors();
	AddBehavior(&m_FuncTankBehavior);

	return true;
}

//=========================================================
//=========================================================
void CNPC_Barney::OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior)
{
	if ( pNewBehavior == &m_FuncTankBehavior )
		m_bReadinessCapable = false;

	else if ( pOldBehavior == &m_FuncTankBehavior )
		m_bReadinessCapable = IsReadinessCapable();

	BaseClass::OnChangeRunningBehavior(pOldBehavior, pNewBehavior);
}

//-----------------------------------------------------------------------------
//=========================================================
//=========================================================
void CNPC_Barney::GatherConditions()
{
	BaseClass::GatherConditions();

	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		( ( m_NPCState == NPC_STATE_SCRIPT ) && CanSpeakWhileScripting() ) )
	{
		DoCustomSpeechAI();
	}
}

//=========================================================
//=========================================================
void CNPC_Barney::UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed(TLK_USE);
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput(pActivator, pCaller);
}

//=========================================================
//=========================================================

// INTELIGENCIA ARTIFICIAL

//=========================================================
//=========================================================

AI_BEGIN_CUSTOM_NPC(npc_barney, CNPC_Barney)

AI_END_CUSTOM_NPC()
