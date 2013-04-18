#ifndef IN_PLAYER_H

#define IN_PLAYER_H
#pragma once

#include "hl2_player.h"
#include "director.h"

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

 // Los slots es el tamaño total de la variable que almacena los objetos del inventario (Si lo cambias, también deberías cambiarlo en c_in_player.h del ClientSide)

enum
{
	INVENTORY_POCKET = 1,
	INVENTORY_BACKPACK,
	INVENTORY_ALL
};

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

	const char *GetConVar(const char *pConVar);
	void ExecCommand(const char *pCommand);

	float GetBlood() { return m_iBlood; }
	float GetHunger() { return m_iHunger; }
	float GetThirst() { return m_iThirst; }

	void Precache();
	void Spawn();
	void StartDirector();
	CBaseEntity *EntSelectSpawnPoint();

	void PreThink();
	void PostThink();
	void PlayerDeathThink();

	void BloodThink();
	void HungerThink();
	void ThirstThink();

	// FUNCIONES RELACIONADAS AL MOVIMIENTO
	void HandleSpeedChanges();
	void StartSprinting();
	void StopSprinting();
	void StartWalking();
	void StopWalking();

	// FUNCIONES RELACIONADAS A LAS ARMAS
	void FlashlightTurnOn();
	float CalcWeaponSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0);
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);

	//  FUNCIONES RELACIONADAS AL DAÑO/SALUD
	virtual int OnTakeDamage(const CTakeDamageInfo &inputInfo);

	virtual int TakeBlood(float flBlood);
	virtual bool ScarredBloodWound();

	virtual int TakeHunger(float flHunger);

	// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
	virtual void CreateRagdollEntity();

	// FUNCIONES DE UTILIDAD
	virtual int PlayerGender();
	virtual bool ClientCommand(const CCommand &args);

	// Música
	// [FIXME] Mover a un mejor lugar.
	CSoundPatch *EmitMusic(const char *pName);
	void StopMusic(CSoundPatch *pMusic);
	void VolumeMusic(CSoundPatch *pMusic, float newVolume);
	void FadeoutMusic(CSoundPatch *pMusic, float range = 1.5f);

	//=========================================================
	// FUNCIONES RELACIONADAS AL INVENTARIO
	//=========================================================

	virtual int Inventory_GetItemID(const char *pName);	
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

	CNetworkQAngle( m_angEyeAngles );

private:
	// Variables
	const char *Items[60];

	CInDirector		Director;
	CBaseEntity		*LastSpawn;

	float			NextPainSound;
	float			NextHealthRegeneration;
	float			BodyHurt;
	float			TasksTimer;
	int				StressLevel;

	bool			m_bBloodWound;
	int				m_iBloodTime;	// Última vez en el que una herida de "sangre" se ha abierto.
	int				m_iBloodSpawn;	// Última vez en que una herida de sangre esta sangrando.
	float			m_iBlood;		// Cantidad de sangre.

	int				m_iHungerTime;
	float			m_iHunger;

	int				m_iThirstTime;
	float			m_iThirst;
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