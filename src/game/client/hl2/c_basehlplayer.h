//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if !defined( C_BASEHLPLAYER_H )
#define C_BASEHLPLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseplayer.h"
#include "c_hl2_playerlocaldata.h"

class C_BaseHLPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_BaseHLPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_BaseHLPlayer();

	virtual void		OnDataChanged( DataUpdateType_t updateType );

	void				Weapon_DropPrimary();
		
	float				GetFOV();
	void				Zoom(float FOVOffset, float time);
	float				GetZoom();
	bool				IsZoomed()	{ return m_HL2Local.m_bZooming; }

	//Tony; minor cosmetic really, fix confusion by simply renaming this one; everything calls IsSprinting(), and this isn't really even used.
	bool				IsSprintActive() { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_SPRINT; }
	bool				IsFlashlightActive() { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_FLASHLIGHT; }
	bool				IsBreatherActive() { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_BREATHER; }

	virtual int			DrawModel( int flags );

	LadderMove_t		*GetLadderMove() { return &m_HL2Local.m_LadderMove; }
	virtual void		ExitLadder();
	bool				IsSprinting() const { return m_fIsSprinting; }

	virtual void        CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void        CalcThirdPersonDeathView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	
	// Input handling
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	void			PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd );
	void			PerformClientSideNPCSpeedModifiers( float flFrameTime, CUserCmd *pCmd );

	bool				IsWeaponLowered() { return m_HL2Local.m_bWeaponLowered; }

	virtual IRagdoll* GetRepresentativeRagdoll() const;

public:

	C_HL2PlayerLocalData		m_HL2Local;
	EHANDLE				m_hClosestNPC;
	float				m_flSpeedModTime;
	bool				m_fIsSprinting;

private:
	C_BaseHLPlayer( const C_BaseHLPlayer & ); // not defined, not accessible
	
	bool				TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir );

	float				m_flZoomStart;
	float				m_flZoomEnd;
	float				m_flZoomRate;
	float				m_flZoomStartTime;

	bool				m_bPlayUseDenySound;		// Signaled by PlayerUse, but can be unset by HL2 ladder code...
	float				m_flSpeedMod;
	float				m_flExitSpeedMod;

	EHANDLE	m_hRagdoll;


friend class CHL2GameMovement;
};

class C_BaseHLRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_BaseHLRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();
	
	C_BaseHLRagdoll();
	~C_BaseHLRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;
	
	void AddEntity();
	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	void UpdateOnRemove();
	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	
private:
	
	C_BaseHLRagdoll( const C_BaseHLRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pDestinationEntity );
	void CreateHL2Ragdoll();

private:

	EHANDLE	m_hPlayer;

	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

#endif
