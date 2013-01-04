#include "cbase.h" 
#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_suitpower_bar.h"

#include "c_baseplayer.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"

#include "ImageProgressBar.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>

#include "ConVar.h"
 
using namespace vgui;
 
#include "tier0/memdbgon.h" 
 
#define SUITPOWER_INIT -1

DECLARE_HUDELEMENT(CHudSuitPowerBar);
 
//=========================================================
// CHudSuitPowerBar
// Constructor
//=========================================================
 
CHudSuitPowerBar::CHudSuitPowerBar(const char * pElementName) : CHudElement(pElementName), ImageProgressBar(NULL, "HudSuitBar")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudSuitPowerBar::Init()
{
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudSuitPowerBar::Reset()
{
	// Valores predeterminados.
	m_flSuitPower	= SUITPOWER_INIT;
	m_statusBar		= 1;

	// Dirección de la barra.
	SetProgressDirection(ProgressBar::PROGRESS_EAST);

	// Textura de su contenido
	SetTopTexture("hud/healthbar_ticks_withglow_white");

	// Color de fondo del elemento = Transparente
	SetBgColor(Color(0,0,0,0));
}
 
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudSuitPowerBar::OnThink()
{
	float newPower = 0;

	// Salud del jugador.
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
 
	if (!pPlayer)
		return;
	
	// Evitar una salud menor a 0.
	newPower = max(pPlayer->m_HL2Local.m_flSuitPower, 0);
	
	if (newPower == m_flSuitPower)
		return;
 
	m_flSuitPower = newPower;

	if (newPower <= 40 && m_statusBar != 2)
	{
		m_statusBar = 2;
		SetTopTexture("hud/healthbar_ticks_withglow_red");
	}
	else if (newPower > 40 && m_statusBar != 1)
	{
		m_statusBar = 1;
		SetTopTexture("hud/healthbar_ticks_withglow_white");
	}

	// Las barras solo aceptan un valor entre 0 a 1.
	newPower = (newPower / 100);

	// Actualizar la barra.
	SetProgress(newPower);
}