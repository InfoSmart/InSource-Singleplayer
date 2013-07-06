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
#include "c_fr_player.h"

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

//-----------------------------------------------------------------------------
// Mostrar la cantidad de Nectar del jugador.
//-----------------------------------------------------------------------------
class CHudNectar : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudNectar, CHudNumericDisplay);

public:
	CHudNectar(const char *pElementName);

	virtual void Init();
	virtual void Reset();
	virtual void OnThink();

private:
	int		m_iNectar;	
};	

DECLARE_HUDELEMENT(CHudNectar);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CHudNectar::CHudNectar(const char *pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudSuit")
{
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

//=========================================================
// Init
//=========================================================
void CHudNectar::Init()
{
	Reset();
}

//=========================================================
// Reiniciar
//=========================================================
void CHudNectar::Reset()
{
	m_iNectar = -1;

	// Mostrar el valor inicial (El nectar actual)
	SetDisplayValue(m_iNectar);
}

//=========================================================
// Pensamiento
//=========================================================
void CHudNectar::OnThink()
{
	int newNectar			= 0;
	C_FR_Player *pPlayer	= C_FR_Player::GetLocalFRPlayer();

	if ( !pPlayer )
		return;

	newNectar = max(pPlayer->GetNectar(), 0);

	if ( newNectar == m_iNectar )
		return;

	m_iNectar = newNectar;

	// Mostrar el nuevo valor.
	SetDisplayValue(m_iNectar);
}