//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "NPCEvent.h"
#include "basehlcombatweapon.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_in_player.h"
#else if
#include "in_player.h"
#endif

#ifndef CLIENT_DLL
#include "gamestats.h"
#include "te_effect_dispatch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeapon357 C_Weapon357
#endif

//=========================================================
// CWeapon357
//=========================================================

class CWeapon357 : public CBaseHLCombatWeapon
{
public:
	
	DECLARE_CLASS(CWeapon357, CBaseHLCombatWeapon);
	CWeapon357();

	void	PrimaryDisorient();
	void	PrimaryAttack();
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	float	WeaponAutoAimScale()	{ return 0.6f; }

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

private:
	CWeapon357( const CWeapon357 & );
};

//=========================================================
// Guardado y definición de datos
//=========================================================

IMPLEMENT_NETWORKCLASS_ALIASED( Weapon357, DT_Weapon357 )

BEGIN_NETWORK_TABLE( CWeapon357, DT_Weapon357 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeapon357 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_357, CWeapon357);
PRECACHE_WEAPON_REGISTER(weapon_357);

//=========================================================
// Actividades
//=========================================================

acttable_t CWeapon357::m_acttable[] =
{
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_REVOLVER,					false },
	{ ACT_MP_WALK,						ACT_HL2MP_WALK_REVOLVER,					false },
	{ ACT_MP_RUN,						ACT_HL2MP_RUN_REVOLVER,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_REVOLVER,				false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_REVOLVER,				false },
	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_REVOLVER,					false },
	{ ACT_MP_SWIM,						ACT_HL2MP_SWIM_REVOLVER,					false },
	{ ACT_MP_SWIM_IDLE,					ACT_HL2MP_SWIM_IDLE_REVOLVER,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_REVOLVER,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_REVOLVER,			false },
};

IMPLEMENT_ACTTABLE(CWeapon357);

//=========================================================
// Constructor
//=========================================================
CWeapon357::CWeapon357()
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CWeapon357::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
#ifndef CLIENT_DLL
		case EVENT_WEAPON_RELOAD:
		{
			CEffectData data;

			// Emit six spent shells
			for ( int i = 0; i < 6; i++ )
			{
				data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
				data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
				data.m_nEntIndex = entindex();

				DispatchEffect( "ShellEject", data );
			}

			break;
		}
#endif
	}
}

//=========================================================
// Desorientación del disparo primario.
//=========================================================
void CWeapon357::PrimaryDisorient()
{
#ifndef CLIENT_DLL
	// Only the player fires this way so we can cast
	CIN_Player *pPlayer = ToInPlayer(GetOwner());

	if ( !pPlayer )
		return;

	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt(-1, 1);
	angles.y += random->RandomInt(-1, 1);
	angles.z = 0;

	pPlayer->SnapEyeAngles(angles, true);
	pPlayer->ViewPunch(QAngle(-8, random->RandomFloat(-2, 2), 0));
#endif
}

//=========================================================
// Disparo primario.
//=========================================================
void CWeapon357::PrimaryAttack()
{
	// Only the player fires this way so we can cast
	CIN_Player *pPlayer			= ToInPlayer(GetOwner());
	CBasePlayer *pBasePlayer	= ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	m_iPrimaryAttacks++;

#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
#endif

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pBasePlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
	info.m_pAttacker = pPlayer;

	pPlayer->FireBullets(info);
	//pPlayer->FireBullets(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);

	PrimaryDisorient();

#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());
#endif

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
}
