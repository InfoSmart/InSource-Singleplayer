//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#ifndef SCP_GAMERULES_H

#define SCP_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "in_gamerules.h"

#ifdef CLIENT_DLL
	#define CSCPGameRules C_SCPGameRules
#endif

class CSCPGameRules : public CInGameRules
{
public:
	DECLARE_CLASS(CSCPGameRules, CInGameRules);

#ifndef CLIENT_DLL
	CSCPGameRules();
	~CSCPGameRules();

	virtual void InitDefaultAIRelationships();
#endif

	virtual bool IsMultiplayer() { return false; };		// Multiplayer: Si es Deathmatch, Coop o Survival entonces si es Multiplayer.
	virtual bool IsDeathmatch() { return false; };		// Deathmatch: Un equipo contra otro.
	virtual bool IsCoOp() { return false; };			// Coop: Cooperativo.
	virtual bool IsSurvival() { return false; };		// Survival: Sobrevivir.
	virtual bool IncludeDirector() { return false; };	// Incluir/Crear al Director.

private:
	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		virtual bool ShouldCollide(int collisionGroup0, int collisionGroup1);
		virtual const char* AIClassText(int classType);
		virtual const char *GetGameDescription();

		virtual void PlayerSpawn(CBasePlayer *pPlayer);
	#endif
};

inline CSCPGameRules* SCPGameRules()
{
	return static_cast<CSCPGameRules*>(g_pGameRules);
}

#endif // IN_GAMERULES_H