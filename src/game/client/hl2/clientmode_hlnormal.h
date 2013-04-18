//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_HLNORMAL_H )
#define CLIENTMODE_HLNORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeHLNormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeHLNormal, ClientModeShared );

	ClientModeHLNormal();
	~ClientModeHLNormal();

	virtual void	Init();
	virtual int		GetDeathMessageStartHeight();
	virtual bool	ShouldDrawCrosshair();

	virtual void	LevelInit(const char* newmap);
	virtual void	LevelShutdown();
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

extern ClientModeHLNormal* GetClientModeHLNormal();

#endif // CLIENTMODE_HLNORMAL_H
