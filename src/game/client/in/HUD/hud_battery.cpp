//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD - Energía del traje.
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BAT	-1

//-----------------------------------------------------------------------------
// Mostrar la energía del traje en el HUD.
//-----------------------------------------------------------------------------
class CHudBattery : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CHudBattery, CHudNumericDisplay);

public:
	CHudBattery(const char *pElementName );
	void Init();
	void Reset();
	void VidInit();
	void OnThink();
	void MsgFunc_Battery(bf_read &msg );
	bool ShouldDraw();
	
private:
	int		m_iBat;	
	int		m_iNewBat;
};

DECLARE_HUDELEMENT(CHudBattery);
DECLARE_HUD_MESSAGE(CHudBattery, Battery);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CHudBattery::CHudBattery(const char *pElementName) : BaseClass(NULL, "HudSuit"), CHudElement(pElementName)
{
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT);
}

//-----------------------------------------------------------------------------
// Init()
// Inicialización del indicador.
//-----------------------------------------------------------------------------
void CHudBattery::Init()
{
	HOOK_HUD_MESSAGE(CHudBattery, Battery);
	Reset();

	m_iBat		= INIT_BAT;
	m_iNewBat   = 0;
}

//-----------------------------------------------------------------------------
// Reset()
// Reseteo del indicador.
//-----------------------------------------------------------------------------
void CHudBattery::Reset()
{
	// Mostrar el valor inicial (La energía actual)
	SetDisplayValue(m_iBat);
}

//-----------------------------------------------------------------------------
// VidInit() 
//-----------------------------------------------------------------------------
void CHudBattery::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// ShouldDraw()
// Detectar cuando es el momento de dibujar el indicador.
//-----------------------------------------------------------------------------
bool CHudBattery::ShouldDraw()
{
	bool bNeedsDraw = (m_iBat != m_iNewBat) || (GetAlpha() > 0);
	return (bNeedsDraw && CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// OnThink() 
// Ejecución de tareas.
//-----------------------------------------------------------------------------
void CHudBattery::OnThink()
{
	// Solo actualizar el indicador si ha habido cambios.
	if (m_iBat == m_iNewBat)
		return;

	// Ejecutar animaciones del HUD.
	// Véase /scripts/HudAnimations.txt
	// Nota: Las animaciones controlan el color del indicador.

	if (!m_iNewBat)
	 	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerZero");

	else if (m_iNewBat < m_iBat)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitDamageTaken");

		if (m_iNewBat < 20)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitArmorLow");
	}
	else
	{
		if (m_iBat == INIT_BAT || m_iBat == 0 || m_iNewBat >= 20)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedAbove20");
		else
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedBelow20");
	}

	m_iBat = m_iNewBat;

	// Mostrar el nuevo valor.
	SetDisplayValue(m_iBat);
}

//-----------------------------------------------------------------------------
// MsgFunc_Battery()
// Obtener la energía actual del traje.
//-----------------------------------------------------------------------------
void CHudBattery::MsgFunc_Battery(bf_read &msg)
{
	m_iNewBat = msg.ReadShort();
}
