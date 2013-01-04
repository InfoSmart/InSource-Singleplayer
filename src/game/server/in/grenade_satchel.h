//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Satchel Charge
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	SATCHEL_H
#define	SATCHEL_H

#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"
#include "weapon_slam.h"

class CSoundPatch;
class CSprite;

class CSatchelCharge : public CBaseGrenade
{

public:
	DECLARE_CLASS(CSatchelCharge, CBaseGrenade);

	void			Spawn();
	void			Precache();
	void			BounceSound();
	void			SatchelTouch(CBaseEntity *pOther);
	void			SatchelThink();
	
	// Input handlers
	void			InputExplode(inputdata_t &inputdata);

	float			m_flNextBounceSoundTime;
	bool			m_bInAir;
	Vector			m_vLastPosition;

public:
	CWeapon_SLAM*	m_pMyWeaponSLAM;
	bool			m_bIsAttached;
	void			Deactivate();

	CSatchelCharge();
	~CSatchelCharge();

	DECLARE_DATADESC();

private:
	void				CreateEffects();
	CHandle<CSprite>	m_hGlowSprite;
};

#endif	//SATCHEL_H
