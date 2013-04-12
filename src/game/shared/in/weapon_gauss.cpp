//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: tingtoms gauss cannon code
//
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "in_buttons.h"
#include "shake.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
#else
	#include "in_player.h"
	#include "explode.h"
	#include "te_particlesystem.h"
#endif

//#include "player.h"
//#include "gamerules.h"

//#include "decals.h"
#include "beam_shared.h"
#include "AmmoDef.h"
#include "IEffects.h"
//#include "engine/IEngineSound.h"

//#include "soundenvelope.h"
//#include "soundent.h"

#include "weapon_gauss.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables para la configuración.
//=========================================================

#ifndef CLIENT_DLL

ConVar sk_plr_dmg_gauss("sk_plr_dmg_gauss", "0", 0, "Daño causado por el arma Gauss");
ConVar sk_plr_max_dmg_gauss("sk_plr_max_dmg_gauss", "0", 0, "Daño máximo causado por el arma Gauss");

#endif

//=========================================================
// Guardado y definición de datos
//=========================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGaussGun, DT_WeaponGaussGun );

BEGIN_NETWORK_TABLE( CWeaponGaussGun, DT_WeaponGaussGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponGaussGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_gauss, CWeaponGaussGun);
PRECACHE_WEAPON_REGISTER(weapon_gauss);

//=========================================================
// Tabla de animaciones
//=========================================================

acttable_t	CWeaponGaussGun::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,						ACT_HL2MP_IDLE_AR2,                    false },
    { ACT_HL2MP_RUN,						ACT_HL2MP_RUN_AR2,                    false },
    { ACT_HL2MP_IDLE_CROUCH,				ACT_HL2MP_IDLE_CROUCH_AR2,            false },
    { ACT_HL2MP_WALK_CROUCH,				ACT_HL2MP_WALK_CROUCH_AR2,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,		ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,    false },
    { ACT_HL2MP_GESTURE_RELOAD,				ACT_HL2MP_GESTURE_RELOAD_AR2,        false },
    { ACT_HL2MP_JUMP,						ACT_HL2MP_JUMP_AR2,                    false },
    { ACT_RANGE_ATTACK1,					ACT_RANGE_ATTACK_AR2,                false },
};

IMPLEMENT_ACTTABLE(CWeaponGaussGun);

//=========================================================
// Constructor
//=========================================================
CWeaponGaussGun::CWeaponGaussGun()
{
	m_hViewModel		= NULL;
	m_flNextChargeTime	= 0;
	m_flChargeStartTime = 0;
	m_sndCharge			= NULL;
	m_bCharging			= false;
	m_bChargeIndicated	= false;
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CWeaponGaussGun::Precache()
{
	PrecacheScriptSound("Weapon_Gauss.ChargeLoop");
	PrecacheParticleSystem("plasmabeam");
	PrecacheParticleSystem(GAUSS_BEAM_SPRITE);

	BaseClass::Precache();
}

//=========================================================
// Creación del arma.
//=========================================================
void CWeaponGaussGun::Spawn()
{
	BaseClass::Spawn();
}

//=========================================================
// Disparo
//=========================================================
void CWeaponGaussGun::Fire()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	
	// ¿El jugador no ha sido creado?
	if ( !pOwner )
		return;

	m_bCharging = false;

	if ( m_hViewModel == NULL )
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if ( vm )
			m_hViewModel.Set(vm);
	}

	Vector	startPos	= pOwner->Weapon_ShootPosition();
	Vector	aimDir		= pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecUp, vecRight;
	VectorVectors(aimDir, vecRight, vecUp);

	float x, y, z;

	//Gassian spread
	do {
		x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);

	aimDir			= aimDir + x * GetBulletSpread().x * vecRight + y * GetBulletSpread().y * vecUp;
	Vector endPos	= startPos + (aimDir * MAX_TRACE_LENGTH);
	
	// Shoot a shot straight out
	trace_t	tr;
	UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);
	
#ifndef CLIENT_DLL

	ClearMultiDamage();

	CBaseEntity *pHit = tr.m_pEnt;	
	CTakeDamageInfo dmgInfo(this, pOwner, sk_plr_dmg_gauss.GetFloat(), DMG_SHOCK | DMG_DISSOLVE);

	if ( pHit != NULL )
	{
		CalculateBulletDamageForce(&dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos);
		pHit->DispatchTraceAttack(dmgInfo, aimDir, &tr);
	}
	
	if (tr.DidHitWorld())
	{
		float hitAngle = -DotProduct( tr.plane.normal, aimDir );

		if (hitAngle < 0.5f)
		{
			Vector vReflection;
		
			vReflection = 2.0 * tr.plane.normal * hitAngle + aimDir;			
			startPos	= tr.endpos;
			endPos		= startPos + (vReflection * MAX_TRACE_LENGTH);
			
			// Draw beam to reflection point
			DrawBeam(tr.startpos, tr.endpos, 15, true);

			CPVSFilter filter(tr.endpos);
			te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);

			UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

			//Find new reflection end position
			UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

			if (tr.m_pEnt != NULL)
			{
				dmgInfo.SetDamageForce(GetAmmoDef()->DamageForce(m_iPrimaryAmmoType) * vReflection);
				dmgInfo.SetDamagePosition(tr.endpos);
				tr.m_pEnt->DispatchTraceAttack(dmgInfo, vReflection, &tr);
			}

			// Connect reflection point to end
			DrawBeam(tr.startpos, tr.endpos, 10);
		}
		else
			DrawBeam(tr.startpos, tr.endpos, 15, true);
	}
	else
		DrawBeam(tr.startpos, tr.endpos, 15, true);
	
	ApplyMultiDamage();

#endif

	UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

	CPVSFilter filter(tr.endpos);
	te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

	AddViewKick();

	// Register a muzzleflash for the AI
#ifndef CLIENT_DLL
	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::ChargedFire()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( !pOwner )
		return;

	bool penetrated = false;

	//Play shock sounds
	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	StopChargeSound();

	m_bCharging = false;
	m_bChargeIndicated = false;

	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.2f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

	//Shoot a shot straight out
	Vector	startPos= pOwner->Weapon_ShootPosition();
	Vector	aimDir	= pOwner->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector	endPos	= startPos + ( aimDir * MAX_TRACE_LENGTH );
	
	trace_t	tr;
	UTIL_TraceLine( startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	
#ifndef CLIENT_DLL

	ClearMultiDamage();

	//Find how much damage to do
	float flChargeAmount = ( gpGlobals->curtime - m_flChargeStartTime ) / MAX_GAUSS_CHARGE_TIME;

	//Clamp this
	if ( flChargeAmount > 1.0f )
		flChargeAmount = 1.0f;

	//Determine the damage amount
	float flDamage = sk_plr_dmg_gauss.GetFloat() + ( ( sk_plr_max_dmg_gauss.GetFloat() - sk_plr_dmg_gauss.GetFloat() ) * flChargeAmount );

#endif

	CBaseEntity *pHit = tr.m_pEnt;

	if ( tr.DidHitWorld() )
	{
		//Try wall penetration
		UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );
		UTIL_DecalTrace( &tr, "RedGlowFade" );

		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		
		Vector	testPos = tr.endpos + ( aimDir * 48.0f );

		UTIL_TraceLine( testPos, tr.endpos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
			
		if ( tr.allsolid == false )
		{
			UTIL_DecalTrace( &tr, "RedGlowFade" );

			penetrated = true;
		}
	}

#ifndef CLIENT_DLL
	else if ( pHit != NULL )
	{
		CTakeDamageInfo dmgInfo( this, pOwner, flDamage, DMG_SHOCK | DMG_DISSOLVE );
		CalculateBulletDamageForce( &dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos );

		//Do direct damage to anything in our path
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}

	ApplyMultiDamage();
#endif

	UTIL_ImpactTrace( &tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss" );

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -4.0f, -8.0f );
	viewPunch.y = random->RandomFloat( -0.25f,  0.25f );
	viewPunch.z = 0;

	pOwner->ViewPunch( viewPunch );

	DrawBeam( startPos, tr.endpos, 25, true );

#ifndef CLIENT_DLL
	Vector	recoilForce = pOwner->BodyDirection2D() * -( flDamage * 10.0f );
	recoilForce[2] += 300.0f;//128

	pOwner->ApplyAbsVelocityImpulse( recoilForce );
#endif

	CPVSFilter filter( tr.endpos );
	te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

#ifndef CLIENT_DLL
	if ( penetrated == true )
		RadiusDamage( CTakeDamageInfo( this, this, flDamage, DMG_SHOCK ), tr.endpos, 200.0f, CLASS_NONE, NULL );

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::DrawBeam( const Vector &startPos, const Vector &endPos, float width, bool useMuzzle )
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	
	if (!pOwner)
		return;

	//Check to store off our view model index
	if (m_hViewModel == NULL)
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if (vm)
			m_hViewModel.Set(vm);
	}

	//Draw the main beam shaft
	CBeam *pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, width );
	
	if ( useMuzzle )
	{
		pBeam->PointEntInit( endPos, m_hViewModel );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( width / 4.0f );
		pBeam->SetEndWidth( width );
	}
	else
	{
		pBeam->SetStartPos( startPos );
		pBeam->SetEndPos( endPos );
		pBeam->SetWidth( width );
		pBeam->SetEndWidth( width / 4.0f );
	}

	pBeam->SetBrightness( 255 );
	pBeam->SetColor( 255, 145+random->RandomInt( -16, 16 ), 0 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );

	//Draw electric bolts along shaft
	for ( int i = 0; i < 3; i++ )
	{
		pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, (width/2.0f) + i );
		
		if ( useMuzzle )
		{
			pBeam->PointEntInit( endPos, m_hViewModel );
			pBeam->SetEndAttachment( 1 );
		}
		else
		{
			pBeam->SetStartPos( startPos );
			pBeam->SetEndPos( endPos );
		}
		
		pBeam->SetBrightness( random->RandomInt( 64, 255 ) );
		pBeam->SetColor( 255, 255, 150+random->RandomInt( 0, 64 ) );
		pBeam->RelinkBeam();
		pBeam->LiveForTime( 0.1f );
		pBeam->SetNoise( 1.6f * i );
		pBeam->SetEndWidth( 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::PrimaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );
	
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	
	//pOwner->m_fEffects |= EF_MUZZLEFLASH;

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	Fire();

	m_flCoilMaxVelocity = 0.0f;
	m_flCoilVelocity = 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::IncreaseCharge()
{
	if ( m_flNextChargeTime > gpGlobals->curtime )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	//Check our charge time
	if ( ( gpGlobals->curtime - m_flChargeStartTime ) > MAX_GAUSS_CHARGE_TIME )
	{
		//Notify the player they're at maximum charge
		if ( m_bChargeIndicated == false )
		{
			WeaponSound( SPECIAL2 );
			m_bChargeIndicated = true;
		}

	#ifndef CLIENT_DLL
		if ( ( gpGlobals->curtime - m_flChargeStartTime ) > DANGER_GAUSS_CHARGE_TIME )
		{
			//Damage the player
			WeaponSound( SPECIAL2 );
			
			// Add DMG_CRUSH because we don't want any physics force
			pOwner->TakeDamage( CTakeDamageInfo( this, this, 25, DMG_SHOCK | DMG_CRUSH ) );
			
			color32 gaussDamage = {255,128,0,128};
			UTIL_ScreenFade( pOwner, gaussDamage, 0.2f, 0.2f, FFADE_IN );

			m_flNextChargeTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 2.5f );
		}
	#endif

		return;
	}

	//Decrement power
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	//Make sure we can draw power
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		ChargedFire();
		return;
	}

	m_flNextChargeTime = gpGlobals->curtime + GAUSS_CHARGE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::SecondaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	if ( m_bCharging == false )
	{
		//Start looping animation
		SendWeaponAnim( ACT_GAUSS_SPINCYCLE );
		
		// Start looping sound
	#ifndef CLIENT_DLL
		if ( m_sndCharge == NULL )
		{
			CPASAttenuationFilter filter( this );
			m_sndCharge	= (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "weapons/gauss/chargeloop.wav", ATTN_NORM );
		}

		assert(m_sndCharge!=NULL);
		if ( m_sndCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCharge, 1.0f, 50 );
			(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCharge, 250, 3.0f );
		}
	#endif

		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		m_bChargeIndicated = false;

		//Decrement power
		pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	}

	IncreaseCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::AddViewKick()
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -0.5f, -0.2f );
	viewPunch.y = random->RandomFloat( -0.5f,  0.5f );
	viewPunch.z = 0;

	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::ItemPostFrame()
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
		return;

	if ( pPlayer->m_afButtonReleased & IN_ATTACK2 )
	{
		if ( m_bCharging )
			ChargedFire();
	}

#ifndef CLIENT_DLL
	m_flCoilVelocity	= UTIL_Approach( m_flCoilMaxVelocity, m_flCoilVelocity, 10.0f );
	m_flCoilAngle		= UTIL_AngleMod( m_flCoilAngle + ( m_flCoilVelocity * gpGlobals->frametime ) );

	static float fanAngle = 0.0f;

	fanAngle = UTIL_AngleMod( fanAngle + 2 );

	//Update spinning bits
	SetBoneController( 0, fanAngle );
	SetBoneController( 1, m_flCoilAngle );
#endif
	
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGaussGun::StopChargeSound()
{
#ifndef CLIENT_DLL
	if ( m_sndCharge != NULL )
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCharge, 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGaussGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopChargeSound();
	m_bCharging			= false;
	m_bChargeIndicated	= false;

	return BaseClass::Holster(pSwitchingTo);
}