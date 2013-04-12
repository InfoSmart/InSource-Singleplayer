//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONAR2_H
#define	WEAPONAR2_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponAR2 C_WeaponAR2
#endif

class CWeaponAR2 : public CHLMachineGun
{
public:
	DECLARE_CLASS( CWeaponAR2, CHLMachineGun );

	CWeaponAR2();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame();
	void	Precache();
	
	void	SecondaryAttack();
	void	DelayedAttack();

	const char *GetTracerType() { return "AR2Tracer"; }

	void	AddViewKick();

#ifndef CLIENT_DLL

	void	FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void	FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void	Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	int		CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }

#endif

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }
	float	GetFireRate() { return 0.1f; }

	bool	CanHolster();
	bool	Reload();

	Activity	GetPrimaryAttackActivity();
	
	void	DoImpactEffect(trace_t &tr, int nDamageType);

	virtual const Vector& GetBulletSpread();
	const WeaponProficiencyInfo_t *GetProficiencyValues();

protected:

	float					m_flDelayedFire;
	bool					m_bShotDelayed;
	int						m_nVentPose;
	
	DECLARE_ACTTABLE();
	//DECLARE_DATADESC();

private:
	CWeaponAR2( const CWeaponAR2 & );
};


#endif	//WEAPONAR2_H
