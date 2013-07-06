//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego
//
// Base para crear reglas de juego basados en las de Apocalypse.
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#ifndef IN_GAMERULES_H

#define IN_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2_gamerules.h"

#ifdef CLIENT_DLL
	#define CInGameRules C_InGameRules
#endif

#define VEC_CROUCH_TRACE_MIN	InGameRules()->GetHL2MPViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	InGameRules()->GetHL2MPViewVectors()->m_vCrouchTraceMax

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

class CInGameRules : public CHalfLife2
{
public:
	DECLARE_CLASS(CInGameRules, CHalfLife2);

	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

#ifndef CLIENT_DLL
	CInGameRules();
	~CInGameRules();

	virtual void InitDefaultAIRelationships();
#endif

	virtual bool IsMultiplayer() { return false; };		// Multiplayer: Si es Deathmatch, Coop o Survival entonces si es Multiplayer.
	virtual bool IsDeathmatch() { return false; };		// Deathmatch: Un equipo contra otro.
	virtual bool IsCoOp() { return false; };			// Coop: Cooperativo.
	virtual bool IsSurvival() { return false; };		// Survival: Sobrevivir.
	virtual bool IncludeDirector() { return true; };	// Incluir/Crear al Director.

private:
	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();

		virtual bool ShouldCollide(int collisionGroup0, int collisionGroup1);
		virtual const char* AIClassText(int classType);
		virtual const char *GetGameDescription();

		virtual void PlayerSpawn(CBasePlayer *pPlayer);
		virtual void SpawnDirector();
		virtual int Inventory_GetItemID(const char *pName);
	#endif
};

extern CInGameRules *g_pInGameRules;

inline CInGameRules* InGameRules()
{
	return static_cast<CInGameRules*>(g_pGameRules);
}

#endif // IN_GAMERULES_H