//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Reglas de juego (singleplayer) para InSource.
//
//====================================================================================//

#ifndef IN_GAMERULES_H

#define IN_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2_gamerules.h"

#ifdef CLIENT_DLL
	#define CInSourceSPRules C_InSourceSPRules
#endif

class CInSourceSPRules : public CHalfLife2
{
public:
	DECLARE_CLASS(CInSourceSPRules, CHalfLife2);

	#ifndef CLIENT_DLL
	virtual void InitDefaultAIRelationships();
	#endif

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		virtual bool ShouldCollide(int collisionGroup0, int collisionGroup1);
		virtual const char* AIClassText(int classType);
		virtual const char *GetGameDescription();
	#endif
};

inline CInSourceSPRules* InSinglePlayerGameRules()
{
	return static_cast<CInSourceSPRules*>(g_pGameRules);
}

#endif // IN_GAMERULES_H