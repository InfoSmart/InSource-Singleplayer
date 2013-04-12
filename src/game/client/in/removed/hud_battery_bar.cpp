#include "cbase.h" 
#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_battery_bar.h"

#include "iclientmode.h"

#include "ImageProgressBar.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>

#include "ConVar.h"
 
using namespace vgui;
 
#include "tier0/memdbgon.h" 
 
#define BATTERY_INIT -1

DECLARE_HUDELEMENT(CHudBatteryBar);
DECLARE_HUD_MESSAGE(CHudBatteryBar, Battery);
 
//=========================================================
// CHudBatteryBar
// Constructor
//=========================================================
 
CHudBatteryBar::CHudBatteryBar(const char * pElementName) : CHudElement(pElementName), ImageProgressBar(NULL, "HudBatteryBar")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudBatteryBar::Init()
{
	HOOK_HUD_MESSAGE(CHudBatteryBar, Battery);
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudBatteryBar::Reset()
{
	// Valores predeterminados.
	m_flBattery	= BATTERY_INIT;
	m_iNewBat	= 0;

	// Dirección de la barra.
	SetProgressDirection(ProgressBar::PROGRESS_WEST);

	// Textura de su contenido
	SetTopTexture("hud/healthbar_withglow_white");

	// Color de fondo del elemento = Transparente
	SetBgColor(Color(0,0,0,0));
}
 
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudBatteryBar::OnThink()
{
	float newPower = 0;

	// Evitar una bateria menor a 0.
	newPower = max(m_iNewBat, 0);
	
	if (newPower == m_flBattery)
		return;
 
	m_flBattery = newPower;

	// Las barras solo aceptan un valor entre 0 a 1.
	newPower = (newPower / 300);

	// Actualizar la barra.
	SetProgress(newPower);
}

//=========================================================
// MsgFunc_Battery()
// Recibe un mensaje indicando una actualización.
//========================================================= 
void CHudBatteryBar::MsgFunc_Battery(bf_read &msg)
{
	m_iNewBat = msg.ReadShort();
}