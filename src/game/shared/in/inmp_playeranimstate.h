//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2MP_PLAYERANIMSTATE_H
#define HL2MP_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_HL2_Player;
#define CHL2_Player C_HL2_Player
#else
class CHL2_Player;
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CHL2MPPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CHL2MPPlayerAnimState, CMultiPlayerAnimState );

	CHL2MPPlayerAnimState();
	CHL2MPPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CHL2MPPlayerAnimState();

	void InitHL2MPAnimState( CHL2_Player *pPlayer );
	CHL2_Player *GetHL2MPPlayer( void )							{ return m_pHL2MPPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	bool	HandleMoving( Activity &idealActivity );
	bool	HandleJumping( Activity &idealActivity );
	bool	HandleDucking( Activity &idealActivity );
	bool	HandleSwimming( Activity &idealActivity );

	virtual float GetCurrentMaxGroundSpeed();
private:
	//Tony; temp till 9way!
	bool						SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void				EstimateYaw( void );
	virtual void				ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	
	CHL2_Player   *m_pHL2MPPlayer;
	bool		m_bInAirWalk;
	float		m_flHoldDeployedPoseUntilTime;
};

CHL2MPPlayerAnimState *CreateHL2MPPlayerAnimState( CHL2_Player *pPlayer );



#endif // HL2MP_PLAYERANIMSTATE_H
