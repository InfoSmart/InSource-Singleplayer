//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Arma: MP5
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

extern ConVar sk_plr_dmg_mp5_grenade;

//=========================================================
// CWeaponMP5
//=========================================================

class CWeaponMP5 : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponMP5, CHLSelectFireMachineGun);

	CWeaponMP5();

	DECLARE_SERVERCLASS();
	
	void	Precache();
	void	AddViewKick();

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip(CBaseCombatCharacter *pOwner);
	bool	Reload();

	float		GetFireRate() { return 0.075f; }	// 13.3hz
	int			CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }
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

IMPLEMENT_SERVERCLASS_ST(CWeaponMP5, DT_WeaponMP5)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_mp5, CWeaponMP5);
PRECACHE_WEAPON_REGISTER(weapon_mp5);

BEGIN_DATADESC(CWeaponMP5)

	DEFINE_FIELD(m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(m_flNextGrenadeCheck, FIELD_TIME),

END_DATADESC()

acttable_t	CWeaponMP5::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponMP5);

//=========================================================
// CWeaponMP5
//=========================================================
CWeaponMP5::CWeaponMP5()
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1400;

	m_bAltFiresUnderwater = false;
}

//=========================================================
// Precache()
// Guardar en caché objetos necesarios.
//=========================================================
void CWeaponMP5::Precache()
{
	BaseClass::Precache();
}

//=========================================================
// Equip()
// Equipar arma al jugador o a un NPC.
//=========================================================
void CWeaponMP5::Equip(CBaseCombatCharacter *pOwner)
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
void CWeaponMP5::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
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
void CWeaponMP5::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Hay que darle más munición.
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;

	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//=========================================================
// Operator_HandleAnimEvent()
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CWeaponMP5::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	const char *pName = EventList_NameForIndex(pEvent->event);
	Msg("Evento: %s \n", pName);

	switch(pEvent->event)
	{
		case AE_NPC_WEAPON_FIRE:
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
Activity CWeaponMP5::GetPrimaryAttackActivity()
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_IDLE;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_PRIMARYATTACK;

	return ACT_VM_PRIMARYATTACK;
}

//=========================================================
// Reload()
// Recarga de munición.
//=========================================================
bool CWeaponMP5::Reload()
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
void CWeaponMP5::AddViewKick()
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	// Grados
	#define	SLIDE_LIMIT			2.0f	// Segundos
	
	// 
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}


//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponMP5::GetProficiencyValues()
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
