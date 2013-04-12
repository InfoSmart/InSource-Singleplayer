
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"

#include "ImageProgressBar.h"
#include "hud_numericdisplay.h"
#include "convar.h"
#include "iclientmode.h"

#include "c_baseplayer.h"
#include "c_in_player.h"

using namespace vgui;

#include "tier0/memdbgon.h"

//=========================================================
// CHudBlood - Barra
//=========================================================

class CHudBlood : public CHudElement, public ImageProgressBar
{
	DECLARE_CLASS_SIMPLE(CHudBlood, ImageProgressBar);

public:
	CHudBlood(const char *pElementName);

	virtual void Init();
	virtual void Reset();
	virtual void OnThink();

protected:
	ImageProgressBar *pBloodProgressBar;

private:
	float m_iBlood;
};

DECLARE_HUDELEMENT(CHudBlood);

//=========================================================
// Constructor
//=========================================================
CHudBlood::CHudBlood(const char * pElementName) : CHudElement(pElementName), ImageProgressBar(NULL, "HudBloodBar")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
 
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

//=========================================================
// Init
//=========================================================
void CHudBlood::Init()
{
	Reset();
}

//=========================================================
// Reiniciar
//=========================================================
void CHudBlood::Reset()
{
	m_iBlood = -1;

	// DIrección de la barra: Este
	SetProgressDirection(ProgressBar::PROGRESS_EAST);

	// Textura de la barra.
	SetTopTexture("hud/healthbar_red");

	// Color de fondo transparente.
	SetBgColor(Color(0,0,0,0));
}

//=========================================================
// Pensamiento
//=========================================================
void CHudBlood::OnThink()
{
	float newBlood		= 0;

	C_INPlayer *pPlayer = (C_INPlayer *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	newBlood = max(pPlayer->GetBlood(), 0);

	if ( newBlood == m_iBlood )
		return;

	ConVarRef sv_in_maxblood("sv_in_maxblood");

	m_iBlood = newBlood;
	newBlood = ( newBlood / sv_in_maxblood.GetFloat() );

	SetProgress(newBlood);
}

//=========================================================
// CHudBloodText - Texto encima de la barra.
//=========================================================

class CHudBloodText : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudBloodText, CHudNumericDisplay);

public:
	CHudBloodText(const char *pElementName);

	virtual void Init();
	virtual void Reset();
	virtual void OnThink();

private:
	float m_iBlood;
};

DECLARE_HUDELEMENT(CHudBloodText);

//=========================================================
// Constructor
//=========================================================
CHudBloodText::CHudBloodText(const char * pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudBloodText")
{ 
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

//=========================================================
// Init
//=========================================================
void CHudBloodText::Init()
{
	Reset();
}

//=========================================================
// Reiniciar
//=========================================================
void CHudBloodText::Reset()
{
	m_iBlood = -1;

	SetDisplayValue(m_iBlood);

	// Color de fondo transparente.
	SetBgColor(Color(0,0,0,0));
}

//=========================================================
// Pensamiento
//=========================================================
void CHudBloodText::OnThink()
{
	float newBlood		= 0;

	C_INPlayer *pPlayer = (C_INPlayer *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	newBlood = max(pPlayer->GetBlood(), 0);

	if ( newBlood == m_iBlood )
		return;

	m_iBlood = newBlood;
	SetDisplayValue(newBlood);
}