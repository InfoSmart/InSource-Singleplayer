//========= Copyright ? 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A shotgun.
//
//			Primary attack: single barrel shot.
//			Secondary attack: double barrel shot.
//
//=============================================================================//


#include "cbase.h"
#include "NPCEvent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
#else
	#include "in_player.h"
	#include "gamestats.h"
#endif

#include "basehlcombatweapon_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponShotgun C_WeaponShotgun
#endif

//=========================================================
// Definición de variables para la configuración.
//=========================================================

extern ConVar sk_auto_reload_time;
extern ConVar sk_plr_num_shotgun_pellets;

//=========================================================
// CWeaponShotgun
//=========================================================

class CWeaponShotgun : public CBaseHLCombatWeapon
{
	//DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponShotgun, CBaseHLCombatWeapon);
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	CNetworkVar( bool,	m_bNeedPump );		// When emptied completely
	CNetworkVar( bool,	m_bDelayedFire1 );	// Fire primary when finished reloading
	CNetworkVar( bool,	m_bDelayedFire2 );	// Fire secondary when finished reloading
	CNetworkVar( bool,	m_bDelayedReload );	// Reload when finished pump

public:
	void	Precache();

#ifndef CLIENT_DLL
	int		CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	virtual const Vector& GetBulletSpread();

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	virtual float			GetMinRestTime();
	virtual float			GetMaxRestTime();

	virtual float			GetFireRate();

	bool StartReload();
	bool Reload();
	void FillClip();
	void FinishReload();
	void CheckHolsterReload();
	void Pump();
	void ItemHolsterFrame();
	void ItemPostFrame();
	void PrimaryDisorient();
	void SecondaryDisorient();
	void PrimaryAttack();
	void SecondaryAttack();
	void DryFire();

	DECLARE_ACTTABLE();
	CWeaponShotgun();
};

//=========================================================
// Guardado y definición de datos
//=========================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponShotgun, DT_WeaponShotgun )

BEGIN_NETWORK_TABLE( CWeaponShotgun, DT_WeaponShotgun )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bNeedPump ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire1 ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire2 ) ),
	RecvPropBool( RECVINFO( m_bDelayedReload ) ),
#else
	SendPropBool( SENDINFO( m_bNeedPump ) ),
	SendPropBool( SENDINFO( m_bDelayedFire1 ) ),
	SendPropBool( SENDINFO( m_bDelayedFire2 ) ),
	SendPropBool( SENDINFO( m_bDelayedReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponShotgun )
	DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire1, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_shotgun, CWeaponShotgun);
PRECACHE_WEAPON_REGISTER(weapon_shotgun);

//=========================================================
// Actividades
//=========================================================

acttable_t	CWeaponShotgun::m_acttable[] = 
{
	/*
		NPC'S
	*/
	{ ACT_IDLE,						ACT_IDLE_SMG1,						true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

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

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
		
	/*
		JUGADORES
	*/
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_MP_WALK,						ACT_HL2MP_WALK_SHOTGUN,					false },
	{ ACT_MP_RUN,						ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_SHOTGUN,					false },
	{ ACT_MP_SWIM,						ACT_HL2MP_SWIM_SHOTGUN,					false },
	{ ACT_MP_SWIM_IDLE,					ACT_HL2MP_SWIM_IDLE_SHOTGUN,			false },
	
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
};

IMPLEMENT_ACTTABLE(CWeaponShotgun);

//=========================================================
// Guardar en caché objetos necesarios.
//=========================================================
void CWeaponShotgun::Precache()
{
	CBaseCombatWeapon::Precache();
}


//=========================================================
//=========================================================
const Vector& CWeaponShotgun::GetBulletSpread()
{
	static Vector VitalAllySpread	= VECTOR_CONE_3DEGREES;
	static Vector Spread			= VECTOR_CONE_10DEGREES;

#ifndef CLIENT_DLL
	// El dueño de esta arma es el jugador.
	if ( GetOwner() && GetOwner()->IsPlayer() )
	{
		CIN_Player *pPlayer		= ToInPlayer(GetOwner());
		const char *crosshair	= pPlayer->GetConVar("crosshair");

		// Esta agachado y con la mira puesta. Las balas se esparcirán al minimo.
		//if ( pPlayer->IsDucked() && crosshair == "0" )
			Spread = VECTOR_CONE_4DEGREES;

		// Esta agachado. 4 grados de esparcimiento.
		if ( pPlayer->IsDucked() )
			Spread = VECTOR_CONE_8DEGREES;

		// Esta con la mira. 3 grados de esparcimiento.
		//else if ( crosshair == "0" )
			Spread = VECTOR_CONE_6DEGREES;
	}
	// Es un aliado.
	else if( GetOwner() && (GetOwner()->Classify() == CLASS_PLAYER_ALLY_VITAL) )
		return VitalAllySpread;

#endif

	return Spread;
}

#ifndef CLIENT_DLL

//=========================================================
// Disparo primario desde un NPC.
//=========================================================
void CWeaponShotgun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *pNPC = pOperator->MyNPCPointer();

	ASSERT( pNPC != NULL );

	WeaponSound(SINGLE_NPC);
	pOperator->DoMuzzleFlash();
	m_iClip1--;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment(LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else 
	{
		vecShootOrigin	= pOperator->Weapon_ShootPosition();
		vecShootDir		= pNPC->GetActualShootTrajectory(vecShootOrigin);
	}

	pOperator->FireBullets(8, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
}

//=========================================================
// Forzar a un NPC a disparar.
//=========================================================
void CWeaponShotgun::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Hay que darle más munición.
	m_iClip1++;
	FireNPCPrimaryAttack(pOperator, true);
}

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CWeaponShotgun::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			FireNPCPrimaryAttack(pOperator, false);
		}
		break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose:	When we shipped HL2, the shotgun weapon did not override the
//			BaseCombatWeapon default rest time of 0.3 to 0.6 seconds. When
//			NPC's fight from a stationary position, their animation events
//			govern when they fire so the rate of fire is specified by the
//			animation. When NPC's move-and-shoot, the rate of fire is 
//			specifically controlled by the shot regulator, so it's imporant
//			that GetMinRestTime and GetMaxRestTime are implemented and provide
//			reasonable defaults for the weapon. To address difficulty concerns,
//			we are going to fix the combine's rate of shotgun fire in episodic.
//			This change will not affect Alyx using a shotgun in EP1. (sjb)
//-----------------------------------------------------------------------------
float CWeaponShotgun::GetMinRestTime()
{
#ifndef CLIENT_DLL
	if( hl2_episodic.GetBool() && GetOwner() && GetOwner()->Classify() == CLASS_COMBINE )
		return 1.2f;
#endif
	
	return BaseClass::GetMinRestTime();
}

//=========================================================
//=========================================================
float CWeaponShotgun::GetMaxRestTime()
{
#ifndef CLIENT_DLL
	if( hl2_episodic.GetBool() && GetOwner() && GetOwner()->Classify() == CLASS_COMBINE )
		return 1.5f;
#endif

	return BaseClass::GetMaxRestTime();
}

//=========================================================
// Tiempo entre disparos.
//=========================================================
float CWeaponShotgun::GetFireRate()
{
#ifndef CLIENT_DLL
	if( hl2_episodic.GetBool() && GetOwner() && GetOwner()->Classify() == CLASS_COMBINE )
		return 0.8f;
#endif

	return 0.7;
}

//=========================================================
// Empieza la recarga de munición.
// En este caso bala por bala.
//=========================================================
bool CWeaponShotgun::StartReload()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	// Nuestro dueño no existe ¿un fantasma?
	if ( !pOwner )
		return false;

	// Ya no hay munición disponible.
	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	// Ya tenemos toda la munición.
	if ( m_iClip1 >= GetMaxClip1() )
		return false;

	// If shotgun totally emptied then a pump animation is needed
	
	//NOTENOTE: This is kinda lame because the player doesn't get strong feedback on when the reload has finished,
	//			without the pump.  Technically, it's incorrect, but it's good for feedback...

	if ( m_iClip1 <= 0 )
		m_bNeedPump = true;

	int j = min(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if ( j <= 0 )
		return false;

	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	// Make shotgun shell visible
	SetBodygroup(1, 0);

	pOwner->m_flNextAttack	= gpGlobals->curtime;
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();

	m_bInReload = true;
	return true;
}

//=========================================================
// Recarga de munición.
//=========================================================
bool CWeaponShotgun::Reload()
{
	// Check that StartReload was called first
	if ( !m_bInReload )
		Warning("ERROR: Shotgun Reload called incorrectly!\n");

	CBaseCombatCharacter *pOwner = GetOwner();
	
	// Nuestro dueño no existe ¿un fantasma?
	if ( !pOwner )
		return false;

	// Ya no hay munición disponible.
	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	// Ya tenemos toda la munición.
	if ( m_iClip1 >= GetMaxClip1() )
		return false;

	int j = min(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if ( j <= 0 )
		return false;

	FillClip();

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim(ACT_VM_RELOAD);

	pOwner->m_flNextAttack	= gpGlobals->curtime;
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();

	return true;
}

//=========================================================
//=========================================================
void CWeaponShotgun::FinishReload()
{
	// Make shotgun shell invisible
	SetBodygroup(1, 1);

	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( !pOwner )
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);

	pOwner->m_flNextAttack	= gpGlobals->curtime;
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//=========================================================
//=========================================================
void CWeaponShotgun::FillClip()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( !pOwner )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
		}
	}
}

//=========================================================
//=========================================================
void CWeaponShotgun::Pump()
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if ( !pOwner )
		return;
	
	m_bNeedPump = false;	
	WeaponSound(SPECIAL1);

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_PUMP);

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//=========================================================
//=========================================================
void CWeaponShotgun::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//=========================================================
// Desorientación del disparo primario.
//=========================================================
void CWeaponShotgun::PrimaryDisorient()
{
#ifndef CLIENT_DLL
	// Only the player fires this way so we can cast
	CIN_Player *pPlayer = ToInPlayer(GetOwner());

	if ( !pPlayer )
		return;

	// Desorientar al jugador
	ConVarRef sk_plr_dmg_buckshot("sk_plr_dmg_buckshot");
	QAngle viewPunch;

	viewPunch.x = random->RandomFloat(-2, -1);
	viewPunch.y = random->RandomFloat(-2, 2);
	viewPunch.z = 0;

	pPlayer->ViewPunch(viewPunch);
#endif
}

//=========================================================
// Desorientación del disparo secundario.
//=========================================================
void CWeaponShotgun::SecondaryDisorient()
{
#ifndef CLIENT_DLL
	// Solo el jugador sufre de desorientación.
	CIN_Player *pPlayer = ToInPlayer(GetOwner());

	if ( !pPlayer )
		return;

	// Desorientar al jugador
	ConVarRef sk_plr_dmg_buckshot("sk_plr_dmg_buckshot");

	QAngle viewPunch;

	viewPunch.x = random->RandomFloat(-5, -5);
	viewPunch.y = 0;
	viewPunch.z = 0;

	pPlayer->ViewPunch(viewPunch);
#endif
}

//=========================================================
//=========================================================
void CWeaponShotgun::PrimaryAttack()
{
	// Solo el jugador puede usar este código.
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack	= gpGlobals->curtime + GetViewModelSequenceDuration();
	m_iClip1				-= 1;

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector	vecAiming	= pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);	
	
#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 1.0);
#endif
	
	FireBulletsInfo_t info( 7, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);
	
	PrimaryDisorient();

#ifndef CLIENT_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner());
#endif

	// Traje: Munición agotada.
	if ( !m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0); 

	if ( m_iClip1 )
		m_bNeedPump = true;

	m_iPrimaryAttacks++;

#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
#endif
}

//=========================================================
//=========================================================
void CWeaponShotgun::SecondaryAttack()
{
	// Solo el jugador puede usar este código.
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	pPlayer->m_nButtons &= ~IN_ATTACK2;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(WPN_DOUBLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack	= gpGlobals->curtime + GetViewModelSequenceDuration();
	m_iClip1				-= 2;	// Shotgun uses same clip for primary and secondary attacks

	Vector vecSrc	 = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);	

	// Fire the bullets
	FireBulletsInfo_t info( 12, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);
	SecondaryDisorient();

#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 1.0);
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2);
#endif

	if ( !m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	if( m_iClip1 )
		m_bNeedPump = true;

	m_iSecondaryAttacks++;

#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );
#endif
}
	
//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponShotgun::ItemPostFrame()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if ( !pOwner )
		return;

	if ( m_bInReload )
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ( (pOwner->m_nButtons & IN_ATTACK ) && (m_iClip1 >= 1) )
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire1 = true;
		}

		// If I'm secondary firing and have one round stop reloading and fire
		else if ( (pOwner->m_nButtons & IN_ATTACK2 ) && (m_iClip1 >= 2) )
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire2 = true;
		}

		else if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
		{
			// If out of ammo end reload
			if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			{
				FinishReload();
				return;
			}

			// If clip not full reload again
			if ( m_iClip1 < GetMaxClip1() )
			{
				Reload();
				return;
			}

			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}
	else
	{			
		// Make shotgun shell invisible
		SetBodygroup(1, 1);
	}

	if ( (m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		Pump();
		return;
	}
	
	// Shotgun uses same timing and ammo for secondary attack
	if ( (m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2)&&(m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		m_bDelayedFire2 = false;
		
		if ( (m_iClip1 <= 1 && UsesClipsForAmmo1()) )
		{
			// If only one shell is left, do a single shot instead	
			if ( m_iClip1 == 1 )
				PrimaryAttack();

			else if ( !pOwner->GetAmmoCount(m_iPrimaryAmmoType) )
				DryFire();

			else
				StartReload();
		}

		// Fire underwater?
		else if ( GetOwner()->GetWaterLevel() == 3 && m_bFiresUnderwater == false )
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if ( pOwner->m_afButtonPressed & IN_ATTACK )
				 m_flNextPrimaryAttack = gpGlobals->curtime;

			SecondaryAttack();
		}
	}
	else if ( (m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;

		if ( (m_iClip1 <= 0 && UsesClipsForAmmo1()) || ( !UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType) ) )
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
				DryFire();
			else
				StartReload();
		}

		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}

		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

			if ( pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK )
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			
			PrimaryAttack();
		}
	}

	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else 
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			// weapon isn't useable, switch.
			if ( !(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon( this ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle( );
		return;
	}

}

//=========================================================
// Constructor
//=========================================================
CWeaponShotgun::CWeaponShotgun()
{
	m_bReloadsSingly = true;

	m_bNeedPump		= false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;

	m_fMinRange1		= 0.0;
	m_fMaxRange1		= 500;
	m_fMinRange2		= 0.0;
	m_fMaxRange2		= 200;
}

//=========================================================
// Purpose: 
//=========================================================
void CWeaponShotgun::ItemHolsterFrame()
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;
	
		if ( GetOwner() == NULL )
			return;

		if ( m_iClip1 == GetMaxClip1() )
			return;

		// Just load the clip with no animations
		int ammoFill = min( (GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount( GetPrimaryAmmoType() ) );
		
		GetOwner()->RemoveAmmo( ammoFill, GetPrimaryAmmoType() );
		m_iClip1 += ammoFill;
	}
}