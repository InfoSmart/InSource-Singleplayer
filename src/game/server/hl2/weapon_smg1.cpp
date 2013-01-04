//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Arma: SMG1
//
// Metralleta.
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "grenade_ar2.h"
#include "AI_Memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_plr_dmg_smg1_grenade;

//=========================================================
// CWeaponSMG1
//=========================================================

class CWeaponSMG1 : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponSMG1, CHLSelectFireMachineGun);

	CWeaponSMG1();

	DECLARE_SERVERCLASS();
	
	void	Precache();
	void	AddViewKick();
	void	SecondaryAttack();

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip(CBaseCombatCharacter *pOwner);
	bool	Reload();

	float	GetFireRate() { return 0.075f; }	// 13.3hz
	int		CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack2Condition(float flDot, float flDist);
	Activity	GetPrimaryAttackActivity();

	virtual const Vector& GetBulletSpread()
	{
		static const Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	DECLARE_ACTTABLE();

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSMG1, DT_WeaponSMG1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_smg1, CWeaponSMG1);
PRECACHE_WEAPON_REGISTER(weapon_smg1);

BEGIN_DATADESC(CWeaponSMG1)

	DEFINE_FIELD(m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(m_flNextGrenadeCheck, FIELD_TIME),

END_DATADESC()

acttable_t	CWeaponSMG1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
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
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },

	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_SMG1,                    false },
    { ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_SMG1,                    false },
    { ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_SMG1,            false },
    { ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_SMG1,            false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,    false },
    { ACT_HL2MP_GESTURE_RELOAD,            ACT_GESTURE_RELOAD_SMG1,        false },
    { ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_SMG1,                    false },
    { ACT_RANGE_ATTACK1,                ACT_RANGE_ATTACK_SMG1,                false },
};

IMPLEMENT_ACTTABLE(CWeaponSMG1);

//=========================================================
// CWeaponSMG1
//=========================================================
CWeaponSMG1::CWeaponSMG1()
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1400;

	m_bAltFiresUnderwater = false;
}

//=========================================================
// Precache()
// Guardar en cach� objetos necesarios.
//=========================================================
void CWeaponSMG1::Precache()
{
	UTIL_PrecacheOther("grenade_ar2");
	BaseClass::Precache();
}

//=========================================================
// Equip()
// Equipar arma al jugador o a un NPC.
//=========================================================
void CWeaponSMG1::Equip(CBaseCombatCharacter *pOwner)
{
	if(pOwner->Classify() == CLASS_PLAYER_ALLY)
		m_fMaxRange1 = 3000;
	else
		m_fMaxRange1 = 1400;

	BaseClass::Equip(pOwner);
}

//=========================================================
// FireNPCPrimaryAttack()
// Disparo primario desde un NPC.
//=========================================================
void CWeaponSMG1::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	
	pOperator->FireBullets(5, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0);
	pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}

//=========================================================
// Operator_ForceNPCFire()
// Forzar a un NPC a disparar.
//=========================================================
void CWeaponSMG1::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Hay que darle m�s munici�n.
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;

	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//=========================================================
// Operator_HandleAnimEvent()
// Ejecuta una acci�n al momento que el modelo hace
// la animaci�n correspondiente.
//=========================================================
void CWeaponSMG1::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG1:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = pOperator->Weapon_ShootPosition();
			}

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT(npc != NULL);
			vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

			FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
		}
		break;

		default:
			BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

//=========================================================
// GetPrimaryAttackActivity()
// Selecciona una actividad de combate dependiendo del escenario.
//=========================================================
Activity CWeaponSMG1::GetPrimaryAttackActivity()
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
// Reload()
// Recarga de munici�n.
//=========================================================
bool CWeaponSMG1::Reload()
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

	if (fRet)
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;
		WeaponSound(RELOAD);
	}

	return fRet;
}

//=========================================================
// AddViewKick()
// Efecto de la fuerza del arma.
//=========================================================
void CWeaponSMG1::AddViewKick()
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	// Gradso
	#define	SLIDE_LIMIT			2.0f	// Segundos
	
	// 
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}

//=========================================================
// SecondaryAttack()
// Ataque secundario.
//=========================================================
void CWeaponSMG1::SecondaryAttack()
{
	// Solo el jugador puede lanzar el ataque secundario.
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	
	if (pPlayer == NULL)
		return;

	// Debemos tener munici�n y no estar bajo el agua.
	if ((pPlayer->GetAmmoCount(m_iSecondaryAmmoType) <= 0 ) || (pPlayer->GetWaterLevel() == 3))
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		BaseClass::WeaponSound(EMPTY);
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

		return;
	}

	if(m_bInReload)
		m_bInReload = false;

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound(WPN_DOUBLE);

	pPlayer->RumbleEffect(RUMBLE_357, 0, RUMBLE_FLAGS_NONE);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;

	// Don't autoaim on grenade tosses
	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow);
	VectorScale(vecThrow, 1000.0f, vecThrow);
	
	//Create the grenade
	QAngle angles;
	VectorAngles(vecThrow, angles);

	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecSrc, angles, pPlayer);
	pGrenade->SetAbsVelocity(vecThrow);

	pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE); 
	pGrenade->SetThrower(GetOwner());
	pGrenade->SetDamage(sk_plr_dmg_smg1_grenade.GetFloat());

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Decrease ammo
	pPlayer->RemoveAmmo(1, m_iSecondaryAmmoType);

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);	

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, false, GetClassname());
}

#define	COMBINE_MIN_GRENADE_CLEAR_DIST 256

//=========================================================
// WeaponRangeAttack2Condition()
// Verificar si es conveniente hacer un ataque secundario.
// Solo para NPC.
//=========================================================
int CWeaponSMG1::WeaponRangeAttack2Condition(float flDot, float flDist)
{
	// A menos que tengas mucho tiempo, por ahora nada de lanzar BOMBAS para los NPC.
	return COND_NONE;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponSMG1::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
