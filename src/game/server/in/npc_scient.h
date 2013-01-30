//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BASE
//
// Usa este archivo para hacer una base al crear un nuevo NPC.
//
//=====================================================================================//

#ifndef NPC_SCIENT_H
#define NPC_SCIENT_H

#include "AI_BaseNPC.h"

class CNPC_Scient : public CAI_BaseNPC
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CNPC_Scient, CAI_BaseNPC);

	void Spawn();
	void Precache();
	Class_T	Classify();

	void IdleSound();
	void PainSound(const CTakeDamageInfo &info);
	void AlertSound();
	void DeathSound(const CTakeDamageInfo &info);
	void AttackSound();

	float MaxYawSpeed();
	void HandleAnimEvent(animevent_t *pEvent);

	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);

	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);
	int SelectSchedule();

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	NPC_STATE SelectIdealState();
	DEFINE_CUSTOM_AI;

private:
	float m_flLastHurtTime;
	float m_nextAlertSoundTime;
};

#endif // NPC_BASE_H