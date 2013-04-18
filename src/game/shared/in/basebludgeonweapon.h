//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		The class from which all bludgeon melee
//				weapons are derived. 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BASEBLUDGEONWEAPON_H
#define BASEBLUDGEONWEAPON_H

#include "basehlcombatweapon.h"

#ifdef CLIENT_DLL
#define CBaseHLBludgeonWeapon C_BaseHLBludgeonWeapon
#endif

//=========================================================
// CBaseHLBludgeonWeapon 
//=========================================================
class CBaseHLBludgeonWeapon : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CBaseHLBludgeonWeapon, CBaseHLCombatWeapon );
	CBaseHLBludgeonWeapon();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual	void	Spawn();
	virtual	void	Precache();
	
	//Attack functions
	virtual	void	PrimaryAttack();
	virtual	void	SecondaryAttack();
	
	virtual void	ItemPostFrame();

	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity()	{	return	ACT_VM_HITCENTER;	}
	virtual Activity	GetSecondaryAttackActivity()	{	return	ACT_VM_HITCENTER2;	}

	virtual	float	GetFireRate()								{	return	0.2f;	}
	virtual float	GetRange()								{	return	32.0f;	}
	virtual	float	GetDamageForActivity( Activity hitActivity )	{	return	1.0f;	}

#ifndef CLIENT_DLL
	virtual int		CapabilitiesGet();
	virtual	int		WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif

protected:
	virtual	void	ImpactEffect( trace_t &trace );

private:
	bool			ImpactWater( const Vector &start, const Vector &end );
	void			Swing( int bIsSecondary );
	void			Hit( trace_t &traceHit, Activity nHitActivity, bool bIsSecondary );
	Activity		ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );
};

#endif