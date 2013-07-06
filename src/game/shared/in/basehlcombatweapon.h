//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "basehlcombatweapon_shared.h"

#ifndef BASEHLCOMBATWEAPON_H
#define BASEHLCOMBATWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "ge_screeneffects.h"

#define CHLMachineGun C_HLMachineGun
#endif

//=========================================================
// Machine gun base class
//=========================================================
class CHLMachineGun : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CHLMachineGun, CBaseHLCombatWeapon );
	DECLARE_DATADESC();

	CHLMachineGun();
	
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	virtual void SetGlow(bool state, Color glowColor = Color(255,255,255));
#else
	virtual void PostDataUpdate(DataUpdateType_t updateType);
#endif

	void	PrimaryAttack();

	// Default calls through to m_hOwner, but plasma weapons can override and shoot projectiles here.
	virtual void	ItemPostFrame();
	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual float	GetFireRate() { return 0; };
	virtual bool	Deploy();

#ifndef CLIENT_DLL
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist );
	int				WeaponSoundRealtime( WeaponSound_t shoot_type );
#endif

	virtual const Vector &GetBulletSpread();

	// utility function
	static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

protected:

	int	m_nShotsFired;	// Number of consecutive shots fired

	float	m_flNextSoundTime;	// real-time clock of when to make next sound

private:
	CHLMachineGun( const CHLMachineGun & );

#ifdef CLIENT_DLL
	CEntGlowEffect *m_pEntGlowEffect;
	bool m_bClientGlow;
#endif

	CNetworkVar( bool, m_bEnableGlow );
	CNetworkVar( color32, m_GlowColor );

	bool m_bGlowSetting;
};

//=========================================================
// Machine guns capable of switching between full auto and 
// burst fire modes.
//=========================================================
// Mode settings for select fire weapons
enum
{
	FIREMODE_FULLAUTO = 1,
	FIREMODE_SEMI,
	FIREMODE_3RNDBURST,
};

#ifdef CLIENT_DLL
#define CHLSelectFireMachineGun C_HLSelectFireMachineGun
#endif

//=========================================================
//	>> CHLSelectFireMachineGun
//=========================================================
class CHLSelectFireMachineGun : public CHLMachineGun
{
	DECLARE_CLASS( CHLSelectFireMachineGun, CHLMachineGun );
public:

	CHLSelectFireMachineGun();
	
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual float	GetBurstCycleRate();
	virtual float	GetFireRate();

	virtual bool	Deploy();
	virtual void	WeaponSound( WeaponSound_t shoot_type, float soundtime = 0.0f );

	virtual int		GetBurstSize() { return 3; };

	void			BurstThink();

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

#ifndef CLIENT_DLL
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual int		WeaponRangeAttack2Condition( float flDot, float flDist );
#endif

protected:
	int m_iBurstSize;
	int	m_iFireMode;

private:
	CHLSelectFireMachineGun( const CHLSelectFireMachineGun & );
};
#endif // BASEHLCOMBATWEAPON_H
