//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Reglas de juego (multiplayer) para InSource.
//
//====================================================================================//

#ifndef IN_MULTIPLAYER_GAMERULES_H
#define IN_MULTIPLAYER_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "insp_gamerules.h"

#ifndef CLIENT_DLL
	#include "env_sound.h"
#endif

#ifdef CLIENT_DLL
	#define CInSourceMPRules C_InSourceMPRules
#endif

class HL2MPViewVectors : public CViewVectors
{
public:
	HL2MPViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax ) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;	
};

class CInSourceMPRules : public CInSourceSPRules
{
public:
	DECLARE_CLASS(CInSourceMPRules, CInSourceSPRules);

	virtual const unsigned char *GetEncryptionKey() { return (unsigned char *)"x9Ke0BY7"; }

	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		CInSourceMPRules();
		virtual void Precache();
		virtual void Think();
		virtual void FliesThink();

		virtual bool IsMultiplayer() { return true; };
		virtual bool IsDeathmatch() { return false; };
		virtual bool IsCoOp() { return false; };

		virtual void PlayerSpawn(CBasePlayer *pPlayer);
		virtual const char *GetGameDescription();

		virtual bool ClientConnected(edict_t *pClient, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
		virtual void ClientDisconnected(edict_t *pClient);

		virtual CBasePlayer *GetDeathScorer(CBaseEntity *pKiller, CBaseEntity *pInflictor);
		virtual void DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info);

		virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info);
		virtual const char *NPCName(const char *pClass);

		int NextAmbientMusic;
		CEnvSound *pMusicAmbient;

		int NextFliesSound;
		int NextCleanFlies;
	#endif
};

inline CInSourceMPRules* InSourceMultiPlayerRules()
{
	return static_cast<CInSourceMPRules*>(g_pGameRules);
}

#endif // IN_MULTIPLAYER_GAMERULES_H