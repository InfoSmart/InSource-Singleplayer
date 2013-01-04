//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BASE
//
// Usa este archivo para hacer una base al crear un nuevo NPC.
//
//=====================================================================================//

#ifndef NPC_GRUNT_H
#define NPC_GRUNT_H

#include "AI_BaseNPC.h"

class CNPC_Grunt : public CAI_BaseNPC
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CNPC_Grunt, CAI_BaseNPC);

	void Spawn		();
	void Precache	();
	Class_T	Classify();

	void IdleSound		();
	void PainSound		(const CTakeDamageInfo &info);
	void AlertSound		();
	void DeathSound		(const CTakeDamageInfo &info);
	void HighAttackSound();
	void LowAttackSound	();

	void StartBackgroundMusic	();
	void VolumeBackgroundMusic	(float volume);

	float MaxYawSpeed		();
	void HandleAnimEvent	(animevent_t *pEvent);

	void MeleeAttack1();
	void MeleeAttack2();

	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);

	int OnTakeDamage_Alive	(const CTakeDamageInfo &inputInfo);
	void Event_Killed		(const CTakeDamageInfo &info);

	int SelectCombatSchedule();
	int SelectSchedule		();
	void GatherConditions	();

	void StartTask	(const Task_t *pTask);
	void RunTask	(const Task_t *pTask);

	bool FindNearestPhysicsObject	();
	float DistToPhysicsEnt			();
	bool IsPhysicsObject			(CBaseEntity *pEntity);

	NPC_STATE SelectIdealState();
	DEFINE_CUSTOM_AI;

protected:
	void MusicThink();

	EHANDLE m_hPhysicsEnt;

private:
	float m_flLastHurtTime;
	float m_nextAlertSoundTime;
	float m_volumeFadeOutBackgound;
	float m_flNextThrow;
	bool m_hPhysicsCanThrow;
	bool m_backgroundMusicStarted;
};

#endif // NPC_GRUNT_H