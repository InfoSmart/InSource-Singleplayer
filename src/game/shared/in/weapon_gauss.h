//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: tingtoms gauss cannon code
//
//=============================================================================

#ifndef WEAPON_GAUSS_H
#define WEAPON_GAUSS_H
#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"

#define GAUSS_BEAM_SPRITE "sprites/laserbeam.vmt"

#define	GAUSS_CHARGE_TIME			0.2f
#define	MAX_GAUSS_CHARGE			16
#define	MAX_GAUSS_CHARGE_TIME		3
#define	DANGER_GAUSS_CHARGE_TIME	10

#ifdef CLIENT_DLL
#define CWeaponGaussGun C_WeaponGaussGun
#endif

//=============================================================================
// Gauss cannon
//=============================================================================

class CWeaponGaussGun : public CBaseHLCombatWeapon
{
	//DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponGaussGun, CBaseHLCombatWeapon );

	CWeaponGaussGun();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	void	Spawn();
	void	Precache();
	void	PrimaryAttack();
	void	SecondaryAttack();
	void	AddViewKick();

	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void	ItemPostFrame();

	float	GetFireRate() { return 0.2f; }

	virtual const Vector &GetBulletSpread()
	{
		static Vector cone = VECTOR_CONE_1DEGREES;	
		return cone;
	}

private:
	CWeaponGaussGun( const CWeaponGaussGun & );

protected:

	void	Fire();
	void	ChargedFire();

	void	StopChargeSound();

	void	DrawBeam( const Vector &startPos, const Vector &endPos, float width, bool useMuzzle = false );
	void	IncreaseCharge();

	EHANDLE			m_hViewModel;
	float			m_flNextChargeTime;
	CSoundPatch		*m_sndCharge;

	float			m_flChargeStartTime;
	bool			m_bCharging;
	bool			m_bChargeIndicated;

	float			m_flCoilMaxVelocity;
	float			m_flCoilVelocity;
	float			m_flCoilAngle;
};

#endif // WEAPON_GAUSS_H