#ifndef IN_PLAYER_H

#define IN_PLAYER_H
#pragma once

#include "hl2_player.h"
#include "director.h"

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

class CIN_Player : public CHL2_Player
{
public:
	DECLARE_CLASS(CIN_Player, CHL2_Player);

	CIN_Player();
	~CIN_Player();

	DECLARE_DATADESC();

	void Precache();
	void Spawn();

	void PreThink();
	void PostThink();
	void PlayerDeathThink();

	void StartDirector();

	void HandleSpeedChanges();
	void StartSprinting();
	void StopSprinting();
	void StartWalking();
	void StopWalking();

	void FlashlightTurnOn();
	float CalcWeaponSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0);
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);

	int OnTakeDamage(const CTakeDamageInfo &inputInfo);

	void HandleAnimEvent(animevent_t *pEvent);

	// Sonido
	// [FIXME] Mover a un mejor lugar.
	CSoundPatch *EmitMusic(const char *pName);
	void StopMusic(CSoundPatch *pMusic);
	void VolumeMusic(CSoundPatch *pMusic, float newVolume);
	void FadeoutMusic(CSoundPatch *pMusic, float range = 1.5f);

private:
	// Variables

	CInDirector		Director;

	float			RegenRemander;
	float			BodyHurt;
	float			TasksTimer;
};

inline CIN_Player *ToInPlayer(CBasePlayer *pEntity)
{
	if (!pEntity)
		return NULL;

	#if _DEBUG
		return dynamic_cast<CIN_Player *>(pEntity);
	#else
		return static_cast<CIN_Player *>(pEntity);
	#endif

}

#endif // IN_PLAYER_H