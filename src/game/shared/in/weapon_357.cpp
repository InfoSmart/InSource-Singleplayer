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

//#include "te_effect_dispatch.h"
//#include "gamestats.h"

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
    { ACT_HL2MP_IDLE,						ACT_HL2MP_IDLE_PISTOL,                   false },
    { ACT_HL2MP_RUN,						ACT_HL2MP_RUN_PISTOL,                    false },
    { ACT_HL2MP_IDLE_CROUCH,				ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
    { ACT_HL2MP_WALK_CROUCH,				ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,		ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,   false },
    { ACT_HL2MP_GESTURE_RELOAD,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,         false },
    { ACT_HL2MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,                   false },
    { ACT_RANGE_ATTACK1,					ACT_RANGE_ATTACK_PISTOL,                 false },
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
void CWeapon357::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	/*CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
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
	}*/
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

	// Desorientar al jugador
	ConVarRef sk_plr_dmg_357("sk_plr_dmg_357");

	// InSource
	// Efectos de "principalmente manejando armas"
	if ( pPlayer->GetConVar("in_beginner_weapon") == "1" )
	{
		// Efecto de golpe (Camara hacia arriba y abajo)
		QAngle	viewPunch;

		viewPunch.x = random->RandomFloat(-8.0f, -16.0f);
		viewPunch.y = random->RandomFloat(-0.35f,  0.35f);
		viewPunch.z = 0;

		pPlayer->ViewPunch(viewPunch);

	
		// Efecto de empuje (Camara hacia atras)
		Vector recoilForce = pPlayer->BodyDirection2D() * - (sk_plr_dmg_357.GetFloat() * 10.0f);
		recoilForce[2] += random->RandomFloat(80.0f, 120.0f);

		pPlayer->ApplyAbsVelocityImpulse(recoilForce);
	

		// Iván: No tengo idea de que hace pero estaba ahí...
		QAngle angles = pPlayer->GetLocalAngles();

		angles.x += random->RandomInt(-20, 20);
		angles.y += random->RandomInt(-20, 20);
		angles.z = 0;
		pPlayer->SnapEyeAngles(angles, true);
	}
	else
	{
		// ¡No es noob! Efectos minimos
		QAngle angles = pPlayer->GetLocalAngles();

		angles.x += random->RandomInt(-1, 1);
		angles.y += random->RandomInt(-1, 1);
		angles.z = 0;

		pPlayer->SnapEyeAngles(angles, true);
		pPlayer->ViewPunch(QAngle(-8, random->RandomFloat(-2, 2), 0));
	}
#endif
}

//=========================================================
// Disparo primario.
//=========================================================
void CWeapon357::PrimaryAttack()
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	m_iPrimaryAttacks++;
	//gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);	

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;

	pPlayer->FireBullets(info);
	//pPlayer->FireBullets(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
	//pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	PrimaryDisorient();

	//CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
}
