#if !defined HUD_SUITPOWER_BAR_H
#define HUD_SUITPOWER_BAR_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ImageProgressBar.h"
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudSuitPowerBar : public CHudElement, public ImageProgressBar
{
	DECLARE_CLASS_SIMPLE(CHudSuitPowerBar, ImageProgressBar);
 
public:
	CHudSuitPowerBar(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
 
private:
	float m_flSuitPower;
	int m_statusBar;
 
};
 
#endif // HUD_SUITPOWER_H