#if !defined HUD_SQUADSTATUS_H
#define HUD_SQUADSTATUS_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include <vgui_controls/ImagePanel.h>

#include "ImageProgressBar.h";

using namespace vgui;
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudSquadStatus : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudSquadStatus, Panel);

public:
	CHudSquadStatus(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
	virtual void MsgFunc_SquadMemberDied(bf_read &msg);
 
private:
	float m_iSquadMembers;
	bool m_bSquadMemberAdded;
	bool m_bSquadMembersFollowing;
	bool m_bSquadMemberJustDied;

	ImagePanel*  m_pMemberOne;
	ImagePanel*  m_pMemberTwo;
	ImagePanel*  m_pMemberThree;
 
};
 
#endif // HUD_SQUADSTATUS_H