#if !defined HUD_HEALTH_BAR_H
#define HUD_HEALTH_BAR_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ImageProgressBar.h"
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudHealthBar : public CHudElement, public ImageProgressBar
{
	DECLARE_CLASS_SIMPLE(CHudHealthBar, ImageProgressBar);
 
public:
	CHudHealthBar(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
 
protected:
	ImageProgressBar *m_pHealthProgressBar;
 
private:
	float m_iHealth;
	int m_bitsDamage;
	int m_statusBar;
 
};
 
#endif // HUD_SUITPOWER_H