#ifndef IMAGEPROGRESSBAR_H
#define IMAGEPROGRESSBAR_H
 
#ifdef _WIN32
#pragma once
#endif
 
#include <vgui/VGUI.h>
#include <vgui_controls/ProgressBar.h>
 
using namespace vgui;
 
class ImageProgressBar : public ContinuousProgressBar
{
	DECLARE_CLASS_SIMPLE(ImageProgressBar, ContinuousProgressBar);
 
public:
	ImageProgressBar(Panel *parent, const char *panelName);
	ImageProgressBar(Panel *parent, const char *panelName, const char *topTexturename, const char *bottomTextureName);
 
	virtual void Paint();
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);
 
	void	SetTopTexture(const char *topTextureName);
	void	SetBottomTexture(const char *bottomTextureName);
 
private:
	int		m_iTopTextureId;
	int		m_iBottomTextureId;
	char	m_szTopTextureName[64];
	char	m_szBottomTextureName[64];
};
 
#endif