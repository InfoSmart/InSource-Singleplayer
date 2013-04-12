//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hlnormal.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time("hud_deathnotice_time", "6", 0);

// Player entries in a death notice
struct DeathNoticePlayer
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	CHudTexture *iconDeath;
	int			iSuicide;
	float		flDisplayTime;
	bool		bHeadshot;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );
public:
	CHudDeathNotice( const char *pElementName );

	void Init();
	void VidInit();
	virtual bool ShouldDraw();
	virtual void Paint();
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	void RetireExpiredDeathNotices();
	
	virtual void FireGameEvent(IGameEvent * event);

private:

	CPanelAnimationVarAliasType(float, m_flLineHeight, "LineHeight", "15", "proportional_float");
	CPanelAnimationVar(float, m_flMaxDeathNotices, "MaxDeathNotices", "4");
	CPanelAnimationVar(bool, m_bRightJustify, "RightJustify", "1");
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "HudNumbersTimer");

	// Texture for skull symbol
	CHudTexture		*m_iconD_skull;  
	CHudTexture		*m_iconD_headshot;  

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

using namespace vgui;
DECLARE_HUDELEMENT(CHudDeathNotice);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_iconD_headshot	= NULL;
	m_iconD_skull		= NULL;

	SetHiddenBits(HIDEHUD_MISCSTATUS);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init()
{
	ListenForGameEvent("player_death");	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit()
{
	m_iconD_skull = gHUD.GetIcon("d_skull");
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw()
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	if ( !m_iconD_skull )
		return;

	int yStart = GetClientModeHLNormal()->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(255, 255, 255, 255);

	int iCount = m_DeathNotices.Count();

	for ( int i = 0; i < iCount; i++ )
	{
		CHudTexture *icon = m_DeathNotices[i].iconDeath;

		if ( !icon )
			continue;

		wchar_t victim[256];
		wchar_t killer[256];

		g_pVGuiLocalize->ConvertANSIToUnicode(m_DeathNotices[i].Victim.szName, victim, sizeof(victim));
		g_pVGuiLocalize->ConvertANSIToUnicode(m_DeathNotices[i].Killer.szName, killer, sizeof(killer));

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth(m_hTextFont, victim);
		int y	= yStart + (m_flLineHeight * i);

		int iconWide;
		int iconTall;

		if( icon->bRenderUsingFont )
		{
			iconWide = surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
			iconTall = surface()->GetFontTall( icon->hFont );
		}
		else
		{
			float scale = ( (float)ScreenHeight() / 480.0f );	//scale based on 640x480
			iconWide = (int)( scale * (float)icon->Width() );
			iconTall = (int)( scale * (float)icon->Height() );
		}

		int x;
		if ( m_bRightJustify )
			x =	GetWide() - len - iconWide;
		else
			x = 0;
		
		// Only draw killers name if it wasn't a suicide
		if ( !m_DeathNotices[i].iSuicide )
		{
			if ( m_bRightJustify )
				x -= UTIL_ComputeStringWidth(m_hTextFont, killer);

			// Draw killer's name
			surface()->DrawSetTextPos(x, y);
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawUnicodeString(killer);
			surface()->DrawGetTextPos(x, y);
		}

		Color iconColor(255, 80, 0, 255);

		// Draw death weapon
		//If we're using a font char, this will ignore iconTall and iconWide
		icon->DrawSelf(x, y, iconWide, iconTall, iconColor);
		x += iconWide;

		// Draw victims name
		surface()->DrawSetTextPos(x, y);
		surface()->DrawSetTextFont(m_hTextFont);	//reset the font, draw icon can change it
		surface()->DrawUnicodeString(victim);
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices()
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();

	for ( int i = iSize-1; i >= 0; i-- )
	{
		if ( m_DeathNotices[i].flDisplayTime < gpGlobals->curtime )
			m_DeathNotices.Remove(i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudDeathNotice::FireGameEvent( IGameEvent * event )
{
	if ( !g_PR || hud_deathnotice_time.GetFloat() == 0 )
		return;

	// the event should be "player_death"
	int Killer				= engine->GetPlayerForUserID(event->GetInt("attacker"));
	int Victim				= engine->GetPlayerForUserID(event->GetInt("userid"));
	const char *Weapon		= event->GetString("weapon");

	char WeaponIcon[128];

	if ( Weapon && *Weapon )
		Q_snprintf(WeaponIcon, sizeof(WeaponIcon), "death_%s", Weapon);
	else
		WeaponIcon[0] = 0;

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	// Get the names of the players
	const char *KillerName = g_PR->GetPlayerName(Killer);
	const char *VictimName = g_PR->GetPlayerName(Victim);

	if ( !KillerName || KillerName == "" || KillerName == "ERRORNAME" )
	{
		KillerName	= Weapon;
		Weapon		= "";
	}

	if ( !KillerName )
		KillerName = "";

	if ( !VictimName )
		VictimName = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = Killer;
	deathMsg.Victim.iEntIndex = Victim;

	Q_strncpy( deathMsg.Killer.szName, KillerName, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( deathMsg.Victim.szName, VictimName, MAX_PLAYER_NAME_LENGTH );

	deathMsg.flDisplayTime	= gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.iSuicide		= ( KillerName == "" || KillerName == "worldspawn" || Killer == Victim );

	// Try and find the death identifier in the icon list
	deathMsg.iconDeath = gHUD.GetIcon(WeaponIcon);

	// Can't find it, so use the default skull & crossbones icon
	if ( !deathMsg.iconDeath || deathMsg.iSuicide )
		deathMsg.iconDeath = m_iconD_skull;

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);

	char sDeathMsg[512];

	// Record the death notice in the console
	if ( deathMsg.iSuicide )
	{
		if ( KillerName == "worldspawn" )
			KillerName == "";

		if ( !strcmp(WeaponIcon, "death_worldspawn") )
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s ha muerto.\n", deathMsg.Victim.szName );
		else	//d_world
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s se ha suicidado.\n", deathMsg.Victim.szName );
	}
	else
	{
		Q_snprintf(sDeathMsg, sizeof( sDeathMsg ), "%s ha matado a %s", deathMsg.Killer.szName, deathMsg.Victim.szName);

		if ( Weapon != "" && Weapon )
			Q_strncat( sDeathMsg, VarArgs( " con %s.\n", Weapon ), sizeof(sDeathMsg), COPY_ALL_CHARACTERS );
	}

	Msg("[MUERTE] %s", sDeathMsg);
}



