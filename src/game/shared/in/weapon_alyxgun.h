//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_ALYXGUN_H
#define WEAPON_ALYXGUN_H

#include "basehlcombatweapon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef CLIENT_DLL
#define CWeaponAlyxGun C_WeaponAlyxGun
#endif

class CWeaponAlyxGun : public CHLSelectFireMachineGun
{
	//DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAlyxGun, CHLSelectFireMachineGun );

	CWeaponAlyxGun();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual int		GetMinBurst() { return 4; }
	virtual int		GetMaxBurst() { return 7; }
	virtual float	GetMinRestTime();
	virtual float	GetMaxRestTime();

	float	GetFireRate() { return 0.1f; }
	virtual const Vector& GetBulletSpread();

#ifndef CLIENT_DLL

	int		CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack1Condition( float flDot, float flDist );
	int		WeaponRangeAttack2Condition( float flDot, float flDist );	

	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

#endif

	float m_flTooCloseTimer;

private:
	CWeaponAlyxGun( const CWeaponAlyxGun & );

};

#endif // WEAPON_ALYXGUN_H
