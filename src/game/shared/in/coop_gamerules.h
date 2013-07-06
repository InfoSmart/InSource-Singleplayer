//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas de
//
//====================================================================================//

#ifndef IN_COOP_GAMERULES_H
#define IN_COOP_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "inmp_gamerules.h"

#ifdef CLIENT_DLL
	#define CInCoopGameRules C_InCoopGameRules
#endif

class CInCoopGameRules : public CInMPGameRules
{
public:
	DECLARE_CLASS(CInCoopGameRules, CInMPGameRules);

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		CInCoopGameRules();
		~CInCoopGameRules();

		virtual bool IsCoOp() { return true; }
		virtual bool IncludeDirector() { return true; }

		virtual void Precache();
		virtual void Think();

		virtual const char *GetGameDescription();

		virtual bool ClientConnected(edict_t *pClient, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

		virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info);
	#endif
};

inline CInCoopGameRules* InCoopGameRules()
{
	return static_cast<CInCoopGameRules*>(g_pGameRules);
}

#endif // IN_COOP_GAMERULES_H