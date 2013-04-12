#include "cbase.h" 

#include "hud.h" 
#include "hud_macros.h" 

#include "hudelement.h"
#include "hud_squadstatus.h"

#include "c_basehlplayer.h"
#include "iclientmode.h"

#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
 
#include "tier0/memdbgon.h" 

DECLARE_HUDELEMENT(CHudSquadStatus);
DECLARE_HUD_MESSAGE(CHudSquadStatus, SquadMemberDied);

using namespace vgui;
 
//=========================================================
// CHudSquadStatus
// Constructor
//=========================================================
 
CHudSquadStatus::CHudSquadStatus(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudSquadStatus")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}
 
//=========================================================
// Init()
// Inicializa el elemento HUD.
//========================================================= 
void CHudSquadStatus::Init()
{
	HOOK_HUD_MESSAGE(CHudSquadStatus, SquadMemberDied);
	Reset();
}
 
//=========================================================
// Reset()
// Reinicia el elemento HUD.
//=========================================================  
void CHudSquadStatus::Reset()
{
	// Valores predeterminados.
	m_iSquadMembers				= 0;
	m_bSquadMemberAdded			= false;
	m_bSquadMembersFollowing	= true;
	m_bSquadMemberJustDied		= false;

	// Inicializar las imagenes.
	m_pMemberOne	= new ImagePanel(this, "HudSquadMemberOne");
	m_pMemberTwo	= new ImagePanel(this, "HudSquadMemberTwo");
	m_pMemberThree	= new ImagePanel(this, "HudSquadMemberThree");

	// Miembro 1
	m_pMemberOne->SetImage(scheme()->GetImage("healthbar_bg_1", true));
	m_pMemberOne->SetVisible(false);
	m_pMemberOne->SetBounds(1, 5, 256, 128);

	// Miembro 2
	m_pMemberTwo->SetImage(scheme()->GetImage("healthbar_bg_1", false));
	m_pMemberTwo->SetVisible(false);
	m_pMemberTwo->SetBounds(260, 5, 256, 128);

	// Miembro 3
	m_pMemberThree->SetImage(scheme()->GetImage("healthbar_bg_1", false));
	m_pMemberThree->SetVisible(false);
	m_pMemberThree->SetBounds(520, 5, 256, 128);

	SetBgColor(Color(0,0,0,0));
}
 
//=========================================================
// OnThink()
// Bucle de tareas.
//=========================================================  
void CHudSquadStatus::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	ConVarRef sk_citizen_health("sk_citizen_health");

	if(!pPlayer)
		return;

	int squadMembers	= pPlayer->m_HL2Local.m_iSquadMemberCount;
	bool following		= pPlayer->m_HL2Local.m_fSquadInFollowMode;

	//if (squadMembers == m_iSquadMembers)
		//return;

	if(squadMembers >= 1)
	{
		if(!m_pMemberOne->IsVisible())
			m_pMemberOne->SetVisible(true);
	}

	if(squadMembers >= 2)
	{
		if(!m_pMemberTwo->IsVisible())
			m_pMemberTwo->SetVisible(true);
	}

	if(squadMembers == 3)
	{
		if(!m_pMemberThree->IsVisible())
			m_pMemberThree->SetVisible(true);
	}

	m_iSquadMembers				= squadMembers;
	m_bSquadMembersFollowing	= m_bSquadMembersFollowing;
}

//=========================================================
// Paint()
// Pintar elemento.
//=========================================================  
void CHudSquadStatus::MsgFunc_SquadMemberDied(bf_read &msg)
{
	m_bSquadMemberJustDied = true;
}