//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "weapon_alyxgun.h"

#ifndef CLIENT_DLL
#include "ai_basenpc.h"
#include "globalstate.h"
#include "in_player.h"
#else
#include "c_in_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Configuración
//=========================================================

#define TOOCLOSETIMER_OFF	0.0f
#define ALYX_TOOCLOSETIMER	1.0f		// Time an enemy must be tooclose before Alyx is allowed to shoot it.

//=========================================================
// Guardado y definición de datos
//=========================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAlyxGun, DT_WeaponAlyxGun )

BEGIN_NETWORK_TABLE(CWeaponAlyxGun, DT_WeaponAlyxGun)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAlyxGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_alyxgun, CWeaponAlyxGun);
PRECACHE_WEAPON_REGISTER(weapon_alyxgun);

//=========================================================
// Actividades
//=========================================================

acttable_t	CWeaponAlyxGun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	true },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

	{ ACT_HL2MP_IDLE,						ACT_HL2MP_IDLE_PISTOL,                    false },
    { ACT_HL2MP_RUN,						ACT_HL2MP_RUN_PISTOL,                    false },
    { ACT_HL2MP_IDLE_CROUCH,				ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
    { ACT_HL2MP_WALK_CROUCH,				ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,		ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,    false },
    { ACT_HL2MP_GESTURE_RELOAD,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_HL2MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,                    false },
    { ACT_RANGE_ATTACK1,					ACT_RANGE_ATTACK_PISTOL,                false },
};

IMPLEMENT_ACTTABLE(CWeaponAlyxGun);

//=========================================================
// Constructor
//=========================================================
CWeaponAlyxGun::CWeaponAlyxGun()
{
	m_fMinRange1		= 1;
	m_fMaxRange1		= 5000;

	m_flTooCloseTimer	= TOOCLOSETIMER_OFF;

#ifdef HL2_EPISODIC
	m_fMinRange1		= 60;
	m_fMaxRange1		= 2048;
#endif
}

#ifndef CLIENT_DLL

//=========================================================
// Try to encourage Alyx not to use her weapon at point blank range,
//	but don't prevent her from defending herself if cornered.
//=========================================================
int CWeaponAlyxGun::WeaponRangeAttack1Condition(float flDot, float flDist)
{
		if( flDist < m_fMinRange1 )
		{
			// If Alyx is not able to fire because an enemy is too close, start a timer.
			// If the condition persists, allow her to ignore it and defend herself. The idea
			// is to stop Alyx being content to fire point blank at enemies if she's able to move
			// away, without making her defenseless if she's not able to move.
			float flTime;

			if( m_flTooCloseTimer == TOOCLOSETIMER_OFF )
				m_flTooCloseTimer = gpGlobals->curtime;

			flTime = gpGlobals->curtime - m_flTooCloseTimer;

			// Fake the range to allow Alyx to shoot.
			if( flTime > ALYX_TOOCLOSETIMER )
				flDist = m_fMinRange1 + 1.0f;
		}
		else
			m_flTooCloseTimer = TOOCLOSETIMER_OFF;

		int nBaseCondition = BaseClass::WeaponRangeAttack1Condition(flDot, flDist);

		// While in a vehicle, we extend our aiming cone (this relies on COND_NOT_FACING_ATTACK 
		// TODO: This needs to be rolled in at the animation level
		if ( GetOwner()->IsInAVehicle() )
		{
			Vector vecRoughDirection = ( GetOwner()->GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );
			Vector vecRight;

			GetVectors(NULL, &vecRight, NULL);

			bool bRightSide		= (DotProduct( vecRoughDirection, vecRight ) > 0.0f);
			float flTargetDot	= ( bRightSide ) ? -0.7f : 0.0f;
		
			if ( nBaseCondition == COND_NOT_FACING_ATTACK && flDot >= flTargetDot )
				nBaseCondition = COND_CAN_RANGE_ATTACK1;
		}

		return nBaseCondition;
}

//=========================================================
//=========================================================
int CWeaponAlyxGun::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE;
}

//=========================================================
// Disparo primario desde un NPC.
//=========================================================
void CWeaponAlyxGun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
  		Vector vecShootOrigin, vecShootDir;
		CAI_BaseNPC *pNPC = pOperator->MyNPCPointer();

		ASSERT( pNPC != NULL );

		if ( bUseWeaponAngles )
		{
			QAngle	angShootDir;
			GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
			AngleVectors(angShootDir, &vecShootDir);
		}
		else 
		{
			vecShootOrigin	= pOperator->Weapon_ShootPosition();
 			vecShootDir		= pNPC->GetActualShootTrajectory(vecShootOrigin);
		}

		WeaponSound(SINGLE_NPC);

		if ( hl2_episodic.GetBool() )
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
		else
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

		pOperator->DoMuzzleFlash();

		// Never fire Alyx's last bullet just in case there's an emergency
		// and she needs to be able to shoot without reloading.
		if( m_iClip1 > 1 )
			m_iClip1 = m_iClip1 - 1;
}

//=========================================================
// Forzar a un NPC a disparar.
//=========================================================
void CWeaponAlyxGun::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	// HACK: We need the gun to fire its muzzle flash
	if ( bSecondary == false )
		SetActivity( ACT_RANGE_ATTACK_PISTOL, 0.0f );

	FireNPCPrimaryAttack(pOperator, true);
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CWeaponAlyxGun::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			FireNPCPrimaryAttack(pOperator, false);
			break;
		}
		
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//=========================================================
// Whether or not Alyx is in the injured mode
//=========================================================
bool IsAlyxInInjuredMode()
{
	#ifndef CLIENT_DLL
		return ( GlobalEntity_GetState("ep2_alyx_injured") == GLOBAL_ON );
	#else
		return false;
	#endif
}

//=========================================================
//=========================================================
const Vector& CWeaponAlyxGun::GetBulletSpread()
{
	static Vector Spread				= VECTOR_CONE_2DEGREES;
	static Vector InjuredAlyxSpread		= VECTOR_CONE_6DEGREES;

#ifndef CLIENT_DLL
	if ( GetOwner() && GetOwner()->Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		if ( IsAlyxInInjuredMode() )
			return InjuredAlyxSpread;
		else
			return Spread;
	}


	// El dueño de esta arma es el jugador.
	if ( GetOwner() && GetOwner()->IsPlayer() )
	{
		// Valor predeterminado.
		Spread = VECTOR_CONE_5DEGREES;

		CIN_Player *pPlayer = ToInPlayer(GetOwner());
		const char *crosshair	= pPlayer->GetConVar("crosshair");

		// Esta agachado y con la mira puesta. Las balas se esparcirán al minimo.
		if ( pPlayer->IsDucked() && crosshair == "0" )
			Spread = VECTOR_CONE_1DEGREES;

		// Esta agachado. 3 grados de esparcimiento.
		else if ( pPlayer->IsDucked() )
			Spread = VECTOR_CONE_3DEGREES;

		// Esta con la mira. 2 grados de esparcimiento.
		else if ( crosshair == "0" )
			Spread = VECTOR_CONE_2DEGREES;
	}
#endif

	return Spread;
}

//=========================================================
//=========================================================
float CWeaponAlyxGun::GetMinRestTime()
{
	if ( IsAlyxInInjuredMode() )
		return 1.5f;

	return BaseClass::GetMinRestTime();
}

//=========================================================
//=========================================================
float CWeaponAlyxGun::GetMaxRestTime()
{
	if ( IsAlyxInInjuredMode() )
		return 3.0f;

	return BaseClass::GetMaxRestTime();
}