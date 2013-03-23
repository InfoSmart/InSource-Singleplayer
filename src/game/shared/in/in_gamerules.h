//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Reglas de juego para InSource.
//
//====================================================================================//

#ifndef IN_GAMERULES_H

#define IN_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2_gamerules.h"

#ifdef CLIENT_DLL
	#define CInSource C_InSource
#endif

class CInSource : public CHalfLife2
{
public:
	DECLARE_CLASS(CInSource, CHalfLife2);

	#ifndef CLIENT_DLL
	virtual void InitDefaultAIRelationships();
	#endif

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		virtual void PlayerSpawn(CBasePlayer *pPlayer);
		virtual const char* AIClassText(int classType);
		virtual const char *GetGameDescription();
	#endif
};

inline CInSource* InGameRules()
{
	return static_cast<CInSource*>(g_pGameRules);
}

#endif // IN_GAMERULES_H