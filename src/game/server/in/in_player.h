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

	static CIN_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CIN_Player::s_PlayerEdict = ed;
		return (CIN_Player*)CreateEntityByName( className );
	}

	DECLARE_DATADESC();

	const char *GetConVar(const char *pConVar);
	void ExecCommand(const char *pCommand);

	float		GetBlood()		{ return m_iBlood; }

	void Precache();
	void Spawn();
	void StartDirector();
	CBaseEntity *EntSelectSpawnPoint();

	void PreThink();
	void PostThink();
	void PlayerDeathThink();

	void BloodThink();

	void HandleSpeedChanges();
	void StartSprinting();
	void StopSprinting();
	void StartWalking();
	void StopWalking();

	void FlashlightTurnOn();
	float CalcWeaponSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0);
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);

	int OnTakeDamage(const CTakeDamageInfo &inputInfo);
	int TakeBlood(float flBlood);
	virtual void CreateRagdollEntity();

	virtual int PlayerGender();

	// Música
	// [FIXME] Mover a un mejor lugar.
	CSoundPatch *EmitMusic(const char *pName);
	void StopMusic(CSoundPatch *pMusic);
	void VolumeMusic(CSoundPatch *pMusic, float newVolume);
	void FadeoutMusic(CSoundPatch *pMusic, float range = 1.5f);

private:
	// Variables

	CInDirector		Director;
	CBaseEntity		*LastSpawn;

	float			NextPainSound;
	float			NextHealthRegeneration;
	float			BodyHurt;
	float			TasksTimer;
	int				StressLevel;

	bool			m_bBloodWound;
	int				m_iBloodTime;
	float			m_iBlood;
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