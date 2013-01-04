#include "cbase.h" 
#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_squadhealth_m1.h"

#include "c_basehlplayer.h"
#include "iclientmode.h"

#include "ImageProgressBar.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>

#include "ConVar.h"
 
using namespace vgui;
 
#include "tier0/memdbgon.h" 
 
#define HEALTH_INIT -1

DECLARE_HUDELEMENT(CHudSquadHealthM1);
 
//=========================================================
// CHudSquadHealthM1
// Constructor
//=========================================================
 
CHudSquadHealthM1::CHudSquadHealthM1(const char * pElementName) : CHudElement(pElementName), ImageProgressBar(NULL, "HudSquadMemberOne")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudSquadHealthM1::Init()
{
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudSquadHealthM1::Reset()
{
	// Valores predeterminados.
	m_fHealth	= HEALTH_INIT;

	// Dirección de la barra.
	SetProgressDirection(ProgressBar::PROGRESS_EAST);

	// Textura de su contenido
	SetTopTexture("hud/healthbar_withglow_green");

	// Oculto al empezar.
	SetVisible(false);

	// Color de fondo del elemento = Transparente
	SetBgColor(Color(0,0,0,0));
}
 
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudSquadHealthM1::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	ConVarRef sk_citizen_health("sk_citizen_health");

	if(!pPlayer)
		return;

	int squadMembers	= pPlayer->m_HL2Local.m_iSquadMemberCount;

	if(squadMembers >= 1)
	{
		if(!IsVisible())
			SetVisible(true);

		if(m_fHealth != pPlayer->m_HL2Local.m_iSquadMemberHealthOne)
		{
			m_fHealth = pPlayer->m_HL2Local.m_iSquadMemberHealthOne;
			SetProgress((m_fHealth / sk_citizen_health.GetFloat()));
		}
	}
	else
	{
		if(IsVisible())
			SetVisible(false);
	}
}