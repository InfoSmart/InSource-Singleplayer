//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "NPCevent.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "in_player.h"
	#include "te_effect_dispatch.h"
	#include "prop_combine_ball.h"
	#include "npc_combine.h"
	#include "rumble_shared.h"
	#include "gamestats.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables para la configuración.
//=========================================================

#ifndef CLIENT_DLL

ConVar sk_weapon_ar2_alt_fire_radius	("sk_weapon_ar2_alt_fire_radius",		"10",	0, "");
ConVar sk_weapon_ar2_alt_fire_duration	("sk_weapon_ar2_alt_fire_duration",		"6",	0, "Duración de la bola de energía.");
ConVar sk_weapon_ar2_alt_fire_mass		("sk_weapon_ar2_alt_fire_mass",			"150",	0, "");

#endif

//=========================================================
// Guardado y definición de datos
//=========================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAR2, DT_WeaponAR2 )

BEGIN_NETWORK_TABLE( CWeaponAR2, DT_WeaponAR2 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAR2 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_ar2, CWeaponAR2);
PRECACHE_WEAPON_REGISTER(weapon_ar2);

/*
BEGIN_DATADESC( CWeaponAR2 )

	DEFINE_FIELD( m_flDelayedFire,	FIELD_TIME ),
	DEFINE_FIELD( m_bShotDelayed,	FIELD_BOOLEAN ),

END_DATADESC()
*/

//=========================================================
// Actividades
//=========================================================

acttable_t	CWeaponAR2::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },

	{ ACT_HL2MP_IDLE,						ACT_HL2MP_IDLE_AR2,                    false },
    { ACT_HL2MP_RUN,						ACT_HL2MP_RUN_AR2,                    false },
    { ACT_HL2MP_IDLE_CROUCH,				ACT_HL2MP_IDLE_CROUCH_AR2,            false },
    { ACT_HL2MP_WALK_CROUCH,				ACT_HL2MP_WALK_CROUCH_AR2,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,		ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,    false },
    { ACT_HL2MP_GESTURE_RELOAD,				ACT_HL2MP_IDLE_AR2,						false },
    { ACT_HL2MP_JUMP,						ACT_HL2MP_JUMP_AR2,                    false },
    { ACT_RANGE_ATTACK1,					ACT_RANGE_ATTACK_AR2,					false },
	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponAR2);

//=========================================================
// Constructor
//=========================================================
CWeaponAR2::CWeaponAR2()
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;

	m_fMinRange2	= 256;
	m_fMaxRange2	= 1024;

	m_nShotsFired	= 0;
	m_nVentPose		= -1;

	m_bAltFiresUnderwater = false;

}

//=========================================================
// Guardar en caché objetos necesarios.
//=========================================================
void CWeaponAR2::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("prop_combine_ball");
	UTIL_PrecacheOther("env_entity_dissolver");
#endif

}

//=========================================================
// Handle grenade detonate in-air (even when no ammo is left)
//=========================================================
void CWeaponAR2::ItemPostFrame()
{
	// See if we need to fire off our secondary round
	if ( m_bShotDelayed && gpGlobals->curtime > m_flDelayedFire )
		DelayedAttack();

	// Update our pose parameter for the vents
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if ( pOwner )
	{
		CBaseViewModel *pVM = pOwner->GetViewModel();

		if ( pVM )
		{
			if ( m_nVentPose == -1 )
				m_nVentPose = pVM->LookupPoseParameter("VentPoses");
			
			float flVentPose = RemapValClamped(m_nShotsFired, 0, 5, 0.0f, 1.0f);
			pVM->SetPoseParameter(m_nVentPose, flVentPose);
		}
	}

	BaseClass::ItemPostFrame();
}

//=========================================================
// Selecciona una actividad de combate dependiendo del escenario.
//=========================================================
Activity CWeaponAR2::GetPrimaryAttackActivity()
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//=========================================================
//=========================================================
void CWeaponAR2::DoImpactEffect( trace_t &tr, int nDamageType )
{
	CEffectData data;

	data.m_vOrigin = tr.endpos + (tr.plane.normal * 1.0f);
	data.m_vNormal = tr.plane.normal;

	DispatchEffect("AR2Impact", data);

	BaseClass::DoImpactEffect(tr, nDamageType);
}

//=========================================================
//=========================================================
const Vector& CWeaponAR2::GetBulletSpread()
{
	static Vector Spread = VECTOR_CONE_3DEGREES;

	// El dueño de esta arma es el jugador.
	if ( GetOwner() && GetOwner()->IsPlayer() )
	{
		// Valor predeterminado para el jugador.
		Spread = VECTOR_CONE_8DEGREES;

		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

		// Esta agachado. Las balas se esparcirán al minimo.
		if ( pPlayer->IsDucked() )
			Spread = VECTOR_CONE_3DEGREES;
	}

	return Spread;
}

//=========================================================
//=========================================================
void CWeaponAR2::DelayedAttack()
{
	m_bShotDelayed = false;
	
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	
	if ( !pOwner )
		return;

	// Deplete the clip completely
	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	m_flNextSecondaryAttack = pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();

	// Register a muzzleflash for the AI
	pOwner->DoMuzzleFlash();

#ifndef CLIENT_DLL
	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
#endif
	
	WeaponSound(WPN_DOUBLE);

	// Fire the bullets
	Vector vecSrc		= pOwner->Weapon_ShootPosition( );
	Vector vecAiming	= pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
	Vector impactPoint	= vecSrc + (vecAiming * MAX_TRACE_LENGTH);

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

#ifndef CLIENT_DLL

	// Fire the combine ball
	CreateCombineBall(vecSrc, 
						vecVelocity, 
						sk_weapon_ar2_alt_fire_radius.GetFloat(), 
						sk_weapon_ar2_alt_fire_mass.GetFloat(),
						sk_weapon_ar2_alt_fire_duration.GetFloat(),
						pOwner);

	// View effects
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade(pOwner, white, 0.2, 0, FFADE_IN);


	
	CIN_Player *pPlayer = ToInPlayer(GetOwner());

	// Desorientamos al jugador.
	ConVarRef sk_plr_dmg_ar2("sk_plr_dmg_ar2");

	// InSource
	// Efectos de "principalmente manejando armas"
	if ( pPlayer->GetConVar("in_beginner_weapon") == "1" )
	{
		// Efecto de golpe (Camara hacia arriba y abajo)
		QAngle	viewPunch;

		viewPunch.x = random->RandomFloat(-8.0f, -16.0f);
		viewPunch.y = random->RandomFloat(-0.35f,  0.35f);
		viewPunch.z = 0;

		pOwner->ViewPunch(viewPunch);

		// Efecto de empuje (Camara hacia atras)
		Vector recoilForce = pOwner->BodyDirection2D() * - (sk_plr_dmg_ar2.GetFloat() * 10.0f);
		recoilForce[2] += random->RandomFloat(80.0f, 120.0f);

		pOwner->ApplyAbsVelocityImpulse(recoilForce);

		// Iván: No tengo idea de que hace pero estaba ahí...
		QAngle angles = pOwner->GetLocalAngles();

		angles.x += random->RandomInt(-20, 20);
		angles.y += random->RandomInt(-20, 20);
		angles.z = 0;
	
		pOwner->SnapEyeAngles(angles, true);
	}
	else
	{
		QAngle angles = pOwner->GetLocalAngles();

		angles.x += random->RandomInt(-4, 4);
		angles.y += random->RandomInt(-4, 4);
		angles.z = 0;

		pOwner->SnapEyeAngles(angles, true);

		pOwner->ViewPunch(QAngle(random->RandomInt(-8, -12), random->RandomInt(1, 2), 0));
	}

#endif

	// Decrease ammo
	pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
}

//=========================================================
//=========================================================
void CWeaponAR2::SecondaryAttack()
{
	if ( m_bShotDelayed )
		return;

	// Cannot fire underwater
	if ( GetOwner() && GetOwner()->GetWaterLevel() == 3 )
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		BaseClass::WeaponSound(EMPTY);
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	m_bShotDelayed			= true;
	m_flNextPrimaryAttack	= m_flNextSecondaryAttack = m_flDelayedFire = gpGlobals->curtime + 0.5f;

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

#ifndef CLIENT_DLL
	if( pPlayer )
		pPlayer->RumbleEffect(RUMBLE_AR2_ALT_FIRE, 0, RUMBLE_FLAG_RESTART);
#endif

	SendWeaponAnim(ACT_VM_FIDGET);
	WeaponSound(SPECIAL1);

	m_iSecondaryAttacks++;
	//gamestats->Event_WeaponFired(pPlayer, false, GetClassname());
}

//=========================================================
// Purpose: Override if we're waiting to release a shot
// Output : Returns true on success, false on failure.
//=========================================================
bool CWeaponAR2::CanHolster()
{
	if ( m_bShotDelayed )
		return false;

	return BaseClass::CanHolster();
}

//=========================================================
// Purpose: Override if we're waiting to release a shot
//=========================================================
bool CWeaponAR2::Reload()
{
	if ( m_bShotDelayed )
		return false;

	return BaseClass::Reload();
}

#ifndef CLIENT_DLL

//=========================================================
// Disparo primario desde un NPC.
//=========================================================
void CWeaponAR2::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles)
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *pNPC = pOperator->MyNPCPointer();

	ASSERT( pNPC != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin	= pOperator->Weapon_ShootPosition();
		vecShootDir		= pNPC->GetActualShootTrajectory(vecShootOrigin);
	}

	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
	m_iClip1--;
}

//=========================================================
// Disparo secundario desde un NPC.
//=========================================================
void CWeaponAR2::FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles)
{
	WeaponSound(WPN_DOUBLE);

	if ( !GetOwner() )
		return;
		
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();

	if ( !pNPC )
		return;
	
	// Fire!
	Vector vecSrc;
	Vector vecAiming;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecSrc, angShootDir);
		AngleVectors(angShootDir, &vecAiming);
	}
	else 
	{
		vecSrc = pNPC->Weapon_ShootPosition();		
		Vector vecTarget;

		CNPC_Combine *pSoldier = dynamic_cast<CNPC_Combine *>(pNPC);

		if ( pSoldier )
		{
			// In the distant misty past, elite soldiers tried to use bank shots.
			// Therefore, we must ask them specifically what direction they are shooting.
			vecTarget = pSoldier->GetAltFireTarget();
		}
		else
		{
			// All other users of the AR2 alt-fire shoot directly at their enemy.
			if ( !pNPC->GetEnemy() )
				return;
				
			vecTarget = pNPC->GetEnemy()->BodyTarget(vecSrc);
		}

		vecAiming = vecTarget - vecSrc;
		VectorNormalize(vecAiming);
	}

	Vector impactPoint = vecSrc + (vecAiming * MAX_TRACE_LENGTH);

	float flAmmoRatio	= 1.0f;
	float flDuration	= RemapValClamped(flAmmoRatio, 0.0f, 1.0f, 0.5f, sk_weapon_ar2_alt_fire_duration.GetFloat());
	float flRadius		= RemapValClamped(flAmmoRatio, 0.0f, 1.0f, 4.0f, sk_weapon_ar2_alt_fire_radius.GetFloat());

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

	// Fire the combine ball
	CreateCombineBall(vecSrc, 
		vecVelocity, 
		flRadius, 
		sk_weapon_ar2_alt_fire_mass.GetFloat(),
		flDuration,
		pNPC);
}

//=========================================================
// Forzar a un NPC a disparar.
//=========================================================
void CWeaponAR2::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	if ( bSecondary )
		FireNPCSecondaryAttack(pOperator, true);
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;
		FireNPCPrimaryAttack(pOperator, true);
	}
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CWeaponAR2::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{ 
		case EVENT_WEAPON_AR2:
		{
			FireNPCPrimaryAttack( pOperator, false );
		}
		break;

		case EVENT_WEAPON_AR2_ALTFIRE:
		{
			FireNPCSecondaryAttack( pOperator, false );
		}
		break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::AddViewKick()
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	8.0f	// Grados
	#define	SLIDE_LIMIT			5.0f	// Segundos
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	float flDuration = m_fFireDuration;

#ifndef CLIENT_DLL

	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		// On the 360 (or in any configuration using the 360 aiming scheme), don't let the
		// AR2 progressive into the late, highly inaccurate stages of its kick. Just
		// spoof the time to make it look (to the kicking code) like we haven't been
		// firing for very long.
		flDuration = min(flDuration, 0.75f);
	}

#endif

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, flDuration, SLIDE_LIMIT);
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponAR2::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 3.0,		0.85	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);
	return proficiencyTable;
}
