//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_CROWBAR_H
#define WEAPON_CROWBAR_H

#include "basebludgeonweapon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef CLIENT_DLL
#define CWeaponCrowbar C_WeaponCrowbar
#endif

#define	CROWBAR_RANGE	75.0f
#define	CROWBAR_REFIRE	0.4f

//-----------------------------------------------------------------------------
// CWeaponCrowbar
//-----------------------------------------------------------------------------

class CWeaponCrowbar : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponCrowbar, CBaseHLBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponCrowbar();

	float		GetRange()		{	return	CROWBAR_RANGE;	}
	float		GetFireRate()	{	return	CROWBAR_REFIRE;	}

	void		AddViewKick();
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack()	{ return;	}

#ifndef CLIENT_DLL
	// Animation event
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	CWeaponCrowbar( const CWeaponCrowbar & );
	
};

#endif // WEAPON_CROWBAR_H
