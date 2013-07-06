//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas de
//
//====================================================================================//

#ifndef IN_SURVIVAL_GAMERULES_H
#define IN_SURVIVAL_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "inmp_gamerules.h"

#ifdef CLIENT_DLL
	#define CInSurvivalGameRules C_InSurvivalGameRules
#endif

class CInSurvivalGameRules : public CInMPGameRules
{
public:
	DECLARE_CLASS(CInSurvivalGameRules, CInMPGameRules);

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		CInSurvivalGameRules();
		~CInSurvivalGameRules();

		virtual bool IsSurvival() { return true; };

		virtual void Precache();
		virtual void Think();
		virtual void FliesThink();

		virtual const char *GetGameDescription();

		virtual bool ClientConnected(edict_t *pClient, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

		virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info);

		int NextAmbientMusic;
		CEnvSound *pMusicAmbient;

		int NextFliesSound;
		int NextCleanFlies;
	#endif
};

inline CInSurvivalGameRules* InSurvivalGameRules()
{
	return static_cast<CInSurvivalGameRules*>(g_pGameRules);
}

#endif // IN_SURVIVAL_GAMERULES_H