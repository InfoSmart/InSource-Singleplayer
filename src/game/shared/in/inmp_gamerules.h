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

#include "in_gamerules.h"

#ifndef CLIENT_DLL
	#include "env_sound.h"
#endif

#ifdef CLIENT_DLL
	#define CInSourceMPRules C_InSourceMPRules
#endif

class CInMPGameRules : public CInGameRules
{
public:
	DECLARE_CLASS(CInMPGameRules, CInGameRules);

	// Llave de encriptación única para los archivos especiales.
	// @TODO: Esta es la llave de HL2MP, cambiarla.
	virtual const unsigned char *GetEncryptionKey() { return (unsigned char *)"x9Ke0BY7"; }

private:

	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		CInMPGameRules();
		~CInMPGameRules();

		virtual bool IsMultiplayer() { return true; };		// Estamos en Multiplayer
		virtual bool IncludeDirector() { return false; };	// Por ahora en Multiplayer el Director no funciona.

		virtual void Think();
		virtual void PlayerSpawn(CBasePlayer *pPlayer);
		virtual const char *GetGameDescription();

		virtual void ClientDisconnected(edict_t *pClient);

		virtual CBasePlayer *GetDeathScorer(CBaseEntity *pKiller, CBaseEntity *pInflictor);
		virtual void DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info);

		virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info);
		virtual const char *NPCName(const char *pClass);
	#endif
};

inline CInMPGameRules* InMPGameRules()
{
	return static_cast<CInMPGameRules*>(g_pGameRules);
}

#endif // IN_MULTIPLAYER_GAMERULES_H