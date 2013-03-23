#ifndef IN_PLAYER_H

#define IN_PLAYER_H
#pragma once

#include "hl2_player.h"
#include "director.h"

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

class CIn_Player : public CHL2_Player
{
public:
	DECLARE_CLASS(CIn_Player, CHL2_Player);

	CIn_Player();
	~CIn_Player();

	static CIn_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CIn_Player::s_PlayerEdict = ed;
		return (CIn_Player*)CreateEntityByName( className );
	}

	DECLARE_DATADESC();

	void Precache();
	void Spawn();
	void StartDirector();

	void PreThink();
	void PostThink();
	void PlayerDeathThink();

	void HandleSpeedChanges();
	void StartSprinting();
	void StopSprinting();
	void StartWalking();
	void StopWalking();

	void FlashlightTurnOn();
	float CalcWeaponSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0);
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);

	int OnTakeDamage(const CTakeDamageInfo &inputInfo);

	virtual void CreateRagdollEntity();

	// Sonido
	// [FIXME] Mover a un mejor lugar.
	CSoundPatch *EmitMusic(const char *pName);
	void StopMusic(CSoundPatch *pMusic);
	void VolumeMusic(CSoundPatch *pMusic, float newVolume);
	void FadeoutMusic(CSoundPatch *pMusic, float range = 1.5f);

private:
	// Variables

	CInDirector		Director;

	float			NextHealthRegeneration;
	float			BodyHurt;
	float			TasksTimer;
};

inline CIn_Player *GetInPlayer(CBasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<CIn_Player *>(pEntity);
	#else
		return static_cast<CIn_Player *>(pEntity);
	#endif

}

#endif // IN_PLAYER_H