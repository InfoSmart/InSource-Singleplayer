#if !defined HUD_STATUSBG_H
#define HUD_STATUSBG_H 
 
#ifdef _WIN32
#pragma once
#endif
 
#include "hudelement.h"
#include <vgui_controls/ImagePanel.h>

using namespace vgui;
 
//-----------------------------------------------------------------------------
// Purpose: Shows the hull bar
//-----------------------------------------------------------------------------
 
class CHudStatusBG : public CHudElement, public ImagePanel
{
	DECLARE_CLASS_SIMPLE(CHudStatusBG, ImagePanel);

public:
	CHudStatusBG(const char * pElementName);
 
	virtual void Init();
	virtual void Reset();
	virtual void OnThink();
	//virtual void Paint();
 
private:
	float m_nImport;
 
};
 
#endif // HUD_SUITPOWER_H