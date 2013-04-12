#if !defined HUD_SQUADHEALTH_M1_H
#define HUD_SQUADHEALTH_M1_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include "ImageProgressBar.h"
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudSquadHealthM1 : public CHudElement, public ImageProgressBar
{
	DECLARE_CLASS_SIMPLE(CHudSquadHealthM1, ImageProgressBar);
 
public:
	CHudSquadHealthM1(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
 
private:
	float m_fHealth;
 
};
 
#endif // HUD_SQUADHEALTH_M1_H