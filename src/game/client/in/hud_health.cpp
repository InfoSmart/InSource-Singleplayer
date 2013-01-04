//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD - Indicación de la salud.
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ConVar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_HEALTH -1

//-----------------------------------------------------------------------------
// Mostrar la salud del jugador en el HUD.
//-----------------------------------------------------------------------------
class CHudHealth : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudHealth, CHudNumericDisplay);

public:
	CHudHealth(const char *pElementName);
	virtual void Init();
	virtual void VidInit();
	virtual void Reset();
	virtual void OnThink();
	void MsgFunc_Damage(bf_read &msg);

private:
	int		m_iHealth;	
	int		m_bitsDamage;
};	

DECLARE_HUDELEMENT(CHudHealth);
DECLARE_HUD_MESSAGE(CHudHealth, Damage);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CHudHealth::CHudHealth(const char *pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudHealth")
{
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

//-----------------------------------------------------------------------------
// Init()
// Inicialización del indicador.
//-----------------------------------------------------------------------------
void CHudHealth::Init()
{
	HOOK_HUD_MESSAGE(CHudHealth, Damage);
	Reset();
}

//-----------------------------------------------------------------------------
// Reset()
// Reseteo del indicador.
//-----------------------------------------------------------------------------
void CHudHealth::Reset()
{
	// Resetear variables.
	m_iHealth		= INIT_HEALTH;
	m_bitsDamage	= 0;

	// Mostrar el valor inicial (La salud actual)
	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// VidInit() 
//-----------------------------------------------------------------------------
void CHudHealth::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// OnThink() 
// Ejecución de tareas.
//-----------------------------------------------------------------------------
void CHudHealth::OnThink()
{
	int newHealth		= 0;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

	if (local)
	{
		// Evitar numeros negativos.
		newHealth = max(local->GetHealth(), 0);
	}

	// Solo actualizar el indicador si ha habido cambios.
	if (newHealth == m_iHealth)
		return;

	m_iHealth = newHealth;

	// Ejecutar animaciones del HUD.
	// Véase /scripts/HudAnimations.txt
	// Nota: Las animaciones controlan el color del indicador.

	if (m_iHealth >= 20)
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedAbove20");
	else if (m_iHealth > 0)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedBelow20");
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthLow");
	}

	// Mostrar el nuevo valor.
	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// MsgFunc_Damage() 
// Detección para la perdida de salud (El jugador esta siendo atacado)
//-----------------------------------------------------------------------------
void CHudHealth::MsgFunc_Damage( bf_read &msg )
{

	int armor		= msg.ReadByte();	// Traje?
	int damageTaken = msg.ReadByte();	// Salud
	long bitsDamage = msg.ReadLong();	// Tipo de daño.

	bitsDamage; // Se mantiene para el envio, pero no se usa aquí.

	Vector vecFrom;

	vecFrom.x = msg.ReadBitCoord();
	vecFrom.y = msg.ReadBitCoord();
	vecFrom.z = msg.ReadBitCoord();

	// El jugador esta siendo atacado ahora mismo.
	if (damageTaken > 0 || armor > 0)
	{
		// Iniciar animación en el HUD.
		if (damageTaken > 0)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthDamageTaken");
	}
}