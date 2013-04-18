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

#include "c_basehlplayer.h"

#include "beamdraw.h"
#include "flashlighteffect.h"

enum
{
	INVENTORY_POCKET = 1,
	INVENTORY_BACKPACK,
	INVENTORY_ALL
};

class C_IN_Player : public C_BaseHLPlayer
{
public:
	DECLARE_CLASS( C_IN_Player, C_BaseHLPlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_IN_Player();
	~C_IN_Player() { }

	static C_IN_Player* GetLocalINPlayer();

	float GetBlood() const { return m_HL2Local.m_iBlood; }
	float GetHunger() const { return m_HL2Local.m_iHunger; }
	float GetThirst() const { return m_HL2Local.m_iThirst; }

	virtual void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
	virtual void DoImpactEffect(trace_t &tr, int nDamageType);
	virtual bool ShouldDraw();
	virtual float GetFOV();

	virtual const QAngle& EyeAngles();

	virtual void AddEntity();
	virtual void UpdateFlashlight();
	virtual void ReleaseFlashlight();

	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual const char *GetConVar(const char *pConVar);

	//=========================================================
	// FUNCIONES RELACIONADAS AL INVENTARIO
	//=========================================================

	bool Inventory_HasItem(int pEntity, int pSection = INVENTORY_POCKET);
	int Inventory_GetItem(int Position, int pSection = INVENTORY_POCKET);
	const char *Inventory_GetItemName(int Position, int pSection = INVENTORY_POCKET);
	const char *Inventory_GetItemClassName(int Position, int pSection = INVENTORY_POCKET);

	virtual int Inventory_GetItemCount(int pSection = INVENTORY_POCKET);

private:
	CFlashlightEffect *m_pFlashlight;
	Beam_t	*m_pFlashlightBeam;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;
};

inline C_IN_Player *GetInPlayer(C_BasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_IN_Player *>(pEntity);
	#else
		return static_cast<C_IN_Player *>(pEntity);
	#endif

}

inline C_IN_Player *ToInPlayer(C_BaseEntity *pEntity)
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_IN_Player *>(pEntity);
	#else
		return static_cast<C_IN_Player *>(pEntity);
	#endif
}

#endif