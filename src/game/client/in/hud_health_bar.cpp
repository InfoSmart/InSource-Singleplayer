#include "cbase.h" 
#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_health_bar.h"

#include "c_baseplayer.h" 
#include "iclientmode.h"

#include "ImageProgressBar.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>

#include "ConVar.h"
 
using namespace vgui;
 
#include "tier0/memdbgon.h" 
 
#define HEALTH_INIT -1

DECLARE_HUDELEMENT(CHudHealthBar);
 
//=========================================================
// CHudHealthBar
// Constructor
//=========================================================
 
CHudHealthBar::CHudHealthBar(const char * pElementName) : CHudElement(pElementName), ImageProgressBar(NULL, "HudHealthBar")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudHealthBar::Init()
{
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudHealthBar::Reset()
{
	// Valores predeterminados.
	m_iHealth		= HEALTH_INIT;
	m_bitsDamage	= 0;
	m_statusBar		= 1;

	// Dirección de la barra.
	SetProgressDirection(ProgressBar::PROGRESS_EAST);

	// Textura de su contenido
	SetTopTexture("hud/healthbar_withglow_green");

	// Color de fondo del elemento = Transparente
	SetBgColor(Color(0,0,0,0));
}
 
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudHealthBar::OnThink()
{
	float newHealth = 0;

	// Salud del jugador.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
 
	if (!pPlayer)
		return;
	
	// Evitar una salud menor a 0.
	newHealth = max(pPlayer->GetHealth(), 0);
	
	if (newHealth == m_iHealth)
		return;
 
	m_iHealth = newHealth;

	// Decidir que color de la barra es más adecuada.
	if(newHealth <= 20)
	{
		if(m_statusBar != 3)
		{
			m_statusBar = 3;
			SetTopTexture("hud/healthbar_withglow_red");
		}
	}
	
	else if(newHealth <= 60)
	{
		if(m_statusBar != 2)
		{
			m_statusBar = 2;
			SetTopTexture("hud/healthbar_withglow_orange");
		}
	}

	else if(newHealth > 60)
	{
		if(m_statusBar != 1)
		{
			m_statusBar = 1;
			SetTopTexture("hud/healthbar_withglow_green");
		}
	}

	// Las barras solo aceptan un valor entre 0 a 1.
	newHealth = (newHealth / 100);

	// Actualizar la barra.
	SetProgress(newHealth);
}