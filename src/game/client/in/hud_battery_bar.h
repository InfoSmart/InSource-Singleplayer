#if !defined HUD_BATTERY_BAR_H
#define HUD_BATTERY_BAR_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include "ImageProgressBar.h"
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudBatteryBar : public CHudElement, public ImageProgressBar
{
	DECLARE_CLASS_SIMPLE(CHudBatteryBar, ImageProgressBar);
 
public:
	CHudBatteryBar(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
	void MsgFunc_Battery(bf_read &msg);
 
private:
	float m_flBattery;
	float m_iNewBat;
 
};
 
#endif // HUD_BATTERY_BAR_H