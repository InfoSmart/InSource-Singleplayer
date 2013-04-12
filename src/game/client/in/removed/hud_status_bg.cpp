#include "cbase.h" 

#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_status_bg.h"

#include "iclientmode.h"

#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
 
using namespace vgui;
 
#include "tier0/memdbgon.h" 
 
#define HEALTH_INIT -1

DECLARE_HUDELEMENT(CHudStatusBG);
 
//=========================================================
// CHudStatusBG
// Constructor
//=========================================================
 
CHudStatusBG::CHudStatusBG(const char * pElementName) : CHudElement(pElementName), ImagePanel(NULL, "HudHealthStatus")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudStatusBG::Init()
{
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudStatusBG::Reset()
{
	SetImage(scheme()->GetImage("healthbar_bg_player", false));
	SetBgColor(Color(0,0,0,0));
}
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudStatusBG::OnThink()
{
	BaseClass::OnThink();
}