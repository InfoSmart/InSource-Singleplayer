#ifndef IN_PLAYER_H

#define IN_PLAYER_H
#pragma once

class CIN_Player;

#include "hl2_playerlocaldata.h"
#include "hl2_player.h"

#include "director.h"
#include "singleplayer_animstate.h"

#include "player_pickup.h"
#include "in_player_shared.h"

#include "ai_speech.h"
#include "ai_basenpc.h"

//CAI_ExpresserHost

class CIN_Player : public CHL2_Player
{
public:
	DECLARE_CLASS(CIN_Player, CHL2_Player);

	CIN_Player();
	~CIN_Player();

	static CIN_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CIN_Player::s_PlayerEdict = ed;
		return (CIN_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// FUNCIONES INTERNAS
	const char *GetConVar(const char *pConVar);
	void ExecCommand(const char *pCommand);

	// FUNCIONES DE DEVOLUCIÓN DE DATOS
	float GetBlood() { return m_iBlood; }
	float GetHunger() { return m_iHunger; }
	float GetThirst() { return m_iThirst; }
	bool AutoHealthRegeneration() { return false; }
	bool PlayingDyingSound() { return ( pDyingSound == NULL ) ? false : true; }

	// FUNCIONES PRINCIPALES
	virtual void UpdateOnRemove();
	virtual void Precache();
	virtual void Spawn();

	virtual void PostConstructor(const char *szClassname);
	virtual CBaseEntity *EntSelectSpawnPoint();

	virtual void Event_Killed(const CTakeDamageInfo &info);
	virtual void Event_Dying();	

	virtual bool WantsLagCompensationOnEntity(const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const;

	// FUNCIONES DE PENSAMIENTO
	virtual void PreThink();
	virtual void PostThink();
	virtual void PlayerDeathThink();

	virtual void TiredThink();
	virtual void TimescaleThink();

	void BloodThink();
	void HungerThink();
	void ThirstThink();

	// FUNCIONES RELACIONADAS AL MOVIMIENTO
	virtual void HandleSpeedChanges();
	virtual void StartSprinting();
	virtual void StopSprinting();
	virtual void StartWalking();
	virtual void StopWalking();

	// FUNCIONES RELACIONADAS A LAS ARMAS
	int Accuracy();
	void FlashlightTurnOn();

	float CalculateSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0); // Shared!
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);

	void FireBullets(const FireBulletsInfo_t &info);

	//  FUNCIONES RELACIONADAS AL DAÑO/SALUD
	virtual void HealthRegeneration();
	virtual int OnTakeDamage(const CTakeDamageInfo &inputInfo);

	virtual int TakeBlood(float flBlood);
	virtual bool ScarredBloodWound();

	virtual int TakeHunger(float flHunger);
	virtual int TakeThirst(float flThirst);

	// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
	virtual int	 Restore(IRestore &restore);
	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);
	virtual void SetAnimation(PLAYER_ANIM playerAnim);

	virtual bool BecomeRagdollOnClient(const Vector &force); 
	virtual void CreateRagdollEntity(const CTakeDamageInfo &info);

	virtual void SetupBones(matrix3x4_t *pBoneToWorld, int boneMask);

	virtual bool IsModelValid(const char *ppModel);
	virtual const char *GetRandomModel();

	//virtual CAI_Expresser *CreateExpresser();
	//CAI_Expresser *GetExpresser();

	virtual void ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet);
	Activity TranslateActivity(Activity baseAct, bool *pRequired);

	// FUNCIONES RELACIONADAS A LA VOZ/CHAT
	virtual bool SpeakIfAllowed(AIConcept_t concept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter);
	virtual bool CanHearAndReadChatFrom(CBasePlayer *pPlayer);

	// FUNCIONES DE UTILIDAD
	virtual PlayerGender Gender();
	virtual bool ClientCommand(const CCommand &args);
	virtual void CheatImpulseCommands(int iImpulse);

	// FUNCIONES RELACIONADAS A STEAM
#ifndef NO_STEAM
	bool		GetSteamID(CSteamID *pID);
	uint64		GetSteamIDAsUInt64();
#endif

	//=========================================================
	// FUNCIONES RELACIONADAS AL INVENTARIO
	//=========================================================

	virtual bool Inventory_HasItem(int pEntity, int pSection = INVENTORY_POCKET);
	virtual int Inventory_AddItem(const char *pName, int pSection = INVENTORY_POCKET);

	virtual int Inventory_AddItemByID(int pEntity, int pSection = INVENTORY_POCKET);

	virtual bool Inventory_MoveItem(int pEntity, int pFrom = INVENTORY_POCKET, int pTo = INVENTORY_BACKPACK);
	virtual bool Inventory_MoveItemByName(const char *pName, int pFrom = INVENTORY_POCKET, int pTo = INVENTORY_BACKPACK);

	virtual bool Inventory_RemoveItem(int pEntity, int pSection = INVENTORY_POCKET);
	virtual bool Inventory_RemoveItemByPos(int Position, int pSection = INVENTORY_POCKET);

	virtual void Inventory_DropAll();
	virtual bool Inventory_DropItem(int pEntity, int pSection = INVENTORY_POCKET, bool dropRandom = false);
	virtual bool Inventory_DropItemByPos(int Position, int pSection = INVENTORY_POCKET, bool dropRandom = false);
	virtual bool Inventory_DropItemByName(const char *pName, int pSection = INVENTORY_POCKET, bool dropRandom = false);

	virtual int Inventory_GetItemCount(int pSection = INVENTORY_POCKET);

	virtual int Inventory_GetItem(int Position, int pSection = INVENTORY_POCKET);
	virtual const char *Inventory_GetItemName(int pEntity);
	virtual const char *Inventory_GetItemNameByPos(int Position, int pSection = INVENTORY_POCKET);

	virtual void Inventory_UseItem(int pEntity, int pSection = INVENTORY_POCKET);
	virtual void Inventory_UseItemByPos(int Position, int pSection = INVENTORY_POCKET);
	virtual void Inventory_UseItemByName(const char *pName, int pSection = INVENTORY_POCKET);

	//=========================================================
	// FUNCIONES RELACIONADAS AL LOOT
	//=========================================================

	virtual void Inventory_LootItemByName(int pLoot, const char *pName);

	CNetworkQAngle( m_angEyeAngles );
	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

public:
	// Variables
	const char *Items[60];

	CDirector		Director;
	CBaseEntity		*m_LastSpawn;

	CSoundPatch		*pDyingSound;

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );

#ifdef APOCALYPSE
	bool			m_bGruntAttack;
	float			m_fRecoverGruntAttack;
#endif

	float			m_fNextPainSound;
	float			m_fNextHealthRegeneration;
	float			m_fBodyHurt;
	int				m_iStressLevel;

	bool			m_bBloodWound;
	int				m_iBloodTime;	// Última vez en el que una herida de "sangre" se ha abierto.
	int				m_iBloodSpawn;	// Última vez en que una herida de sangre esta sangrando.
	float			m_iBlood;		// Cantidad de sangre.

	int				m_iHungerTime;
	float			m_iHunger;

	int				m_iThirstTime;
	float			m_iThirst;

	CSinglePlayerAnimState *m_PlayerAnimState;
	CAI_Expresser *m_pExpresser;

	int m_iIgnoreGlobalChat;

	CUtlVector<const char*> m_pPlayerModels;
};

class CINRagdoll : public CBaseAnimatingOverlay, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS(CINRagdoll, CBaseAnimatingOverlay);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CINRagdoll()
	{
		m_hPlayer.Set(NULL);
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

inline CIN_Player *GetInPlayer(CBasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<CIN_Player *>(pEntity);
	#else
		return static_cast<CIN_Player *>(pEntity);
	#endif

}

inline CIN_Player *ToInPlayer(CBaseEntity *pEntity)
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CIN_Player *>(pEntity);
}

#endif // IN_PLAYER_H