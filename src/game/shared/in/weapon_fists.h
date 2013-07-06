//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_FISTS_H
#define WEAPON_FISTS_H

#include "basebludgeonweapon.h"

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CWeaponFists C_WeaponFists
#endif

#define	FISTS_RANGE		75.0f
#define	FISTS_REFIRE	0.4f

//-----------------------------------------------------------------------------
// CWeaponFists
//-----------------------------------------------------------------------------

class CWeaponFists : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponFists, CBaseHLBludgeonWeapon);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFists();

	float		GetRange()		{	return	FISTS_RANGE;	}
	float		GetFireRate()	{	return	FISTS_REFIRE;	}

	void		AddViewKick();
	float		GetDamageForActivity(Activity hitActivity);
	void		SecondaryAttack() { return; }

#ifndef CLIENT_DLL
	// Animation event
	virtual int WeaponMeleeAttack1Condition(float flDot, float flDist);
	virtual void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	void HandleAnimEventMeleeHit(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	CWeaponFists(const CWeaponFists &);
	
};

#endif // WEAPON_FISTS_H
