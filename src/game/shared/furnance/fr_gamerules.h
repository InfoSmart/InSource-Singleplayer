//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego de Furnance.
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#ifndef FR_GAMERULES_H

#define FR_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "in_gamerules.h"

#ifdef CLIENT_DLL
	#define CFrGameRules C_FrGameRules
#endif

class CFrGameRules : public CInGameRules
{
public:
	DECLARE_CLASS(CFrGameRules, CInGameRules);

#ifndef CLIENT_DLL
	CFrGameRules();
	~CFrGameRules();

	virtual void InitDefaultAIRelationships();

	virtual const char* AIClassText(int classType);
	virtual const char *GetGameDescription();

	virtual void PlayerSpawn(CBasePlayer *pPlayer);
#endif // CLIENT_DLL

	virtual bool IsMultiplayer() { return false; };		// Multiplayer: Si es Deathmatch, Coop o Survival entonces si es Multiplayer.
	virtual bool IsDeathmatch() { return false; };		// Deathmatch: Un equipo contra otro.
	virtual bool IsCoOp() { return false; };			// Coop: Cooperativo.
	virtual bool IsSurvival() { return false; };		// Survival: Sobrevivir.
	virtual bool IncludeDirector() { return false; };	// ¿Incluir/Crear al Director?

};

#endif // FR_GAMERULES_H