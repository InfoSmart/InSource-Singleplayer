
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

#include <vgui/ILocalize.h>

using namespace vgui;

#include "tier0/memdbgon.h"

// NOTE: Una barra no parece muy atractiva, por ahora solo con el texto. (CHudBloodText)
/*
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

	C_IN_Player *pPlayer = (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

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
*/

//=========================================================
// CHudBloodText - Indicador de sangre.
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
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_HEALTH);
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

	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_BLOOD");

	if ( tempString )
		SetLabelText(tempString);
	else
		SetLabelText(L"BLOOD");

	SetDisplayValue(m_iBlood);
}

//=========================================================
// Pensamiento
//=========================================================
void CHudBloodText::OnThink()
{
	float newBlood		= 0;

	C_IN_Player *pPlayer = (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	newBlood = max(pPlayer->GetBlood(), 0);

	if ( newBlood == m_iBlood )
		return;

	m_iBlood = newBlood;
	SetDisplayValue(newBlood);
}

//==================================================================================================================
//==================================================================================================================

//=========================================================
// CHudHungerText - Indicador de hambre.
//=========================================================
class CHudHungerText : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudHungerText, CHudNumericDisplay);

public:
	CHudHungerText(const char *pElementName);

	virtual void Init();
	virtual void Reset();
	virtual void OnThink();

private:
	float m_iHunger;
};

DECLARE_HUDELEMENT(CHudHungerText);

//=========================================================
// Constructor
//=========================================================
CHudHungerText::CHudHungerText(const char * pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudHungerText")
{ 
	// Oculto cuando: El jugador muera, la interfaz de salud/munición desaparesca.
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_HEALTH);
}

//=========================================================
// Init
//=========================================================
void CHudHungerText::Init()
{
	Reset();
}

//=========================================================
// Reiniciar
//=========================================================
void CHudHungerText::Reset()
{
	m_iHunger = -1;

	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_HUNGER");

	if ( tempString )
		SetLabelText(tempString);
	else
		SetLabelText(L"HUNGER");

	SetDisplayValue(m_iHunger);
}

//=========================================================
// Pensamiento
//=========================================================
void CHudHungerText::OnThink()
{
	float newHunger			= 0;
	C_IN_Player *pPlayer	= (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	newHunger = max(pPlayer->GetHunger(), 0);

	if ( newHunger == m_iHunger )
		return;

	m_iHunger = newHunger;
	SetDisplayValue(newHunger);
}

//==================================================================================================================
//==================================================================================================================

//=========================================================
// CHudHungerText - Indicador de sangre.
//=========================================================
class CHudThirstText : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudThirstText, CHudNumericDisplay);

public:
	CHudThirstText(const char *pElementName);

	virtual void Init();
	virtual void Reset();
	virtual void OnThink();

private:
	float m_iThirst;
};

DECLARE_HUDELEMENT(CHudThirstText);

//=========================================================
// Constructor
//=========================================================
CHudThirstText::CHudThirstText(const char * pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudThirstText")
{ 
	// Oculto cuando: El jugador muera, la interfaz de salud/munición desaparesca.
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_HEALTH);
}

//=========================================================
// Init
//=========================================================
void CHudThirstText::Init()
{
	Reset();
}

//=========================================================
// Reiniciar
//=========================================================
void CHudThirstText::Reset()
{
	m_iThirst = -1;

	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_THIRST");

	if ( tempString )
		SetLabelText(tempString);
	else
		SetLabelText(L"THIRST");

	SetDisplayValue(m_iThirst);
}

//=========================================================
// Pensamiento
//=========================================================
void CHudThirstText::OnThink()
{
	float newThirst			= 0;
	C_IN_Player *pPlayer	= (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	newThirst = max(pPlayer->GetThirst(), 0);

	if ( newThirst == m_iThirst )
		return;

	m_iThirst = newThirst;
	SetDisplayValue(newThirst);
}