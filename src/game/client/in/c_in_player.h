//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Funciones del jugador. Client-Side
//
//====================================================================================//

#ifndef	C_IN_PLAYER_H
#define C_IN_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CIN_Player C_INPlayer
#endif

#include "c_basehlplayer.h"

class C_INPlayer : public C_BaseHLPlayer
{
public:
	DECLARE_CLASS( C_INPlayer, C_BaseHLPlayer );
	DECLARE_CLIENTCLASS();

	C_INPlayer();

	float GetBlood() const { return m_HL2Local.m_iBlood; }

	virtual void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
	virtual void DoImpactEffect(trace_t &tr, int nDamageType);
	virtual bool ShouldDraw();
	virtual float GetFOV();

	virtual const char *GetConVar(const char *pConVar);
	
};

inline C_INPlayer *GetInPlayer(C_BasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_INPlayer *>(pEntity);
	#else
		return static_cast<C_INPlayer *>(pEntity);
	#endif

}

inline C_INPlayer *ToInPlayer(C_BaseEntity *pEntity)
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_INPlayer *>(pEntity);
	#else
		return static_cast<C_INPlayer *>(pEntity);
	#endif
}

#endif