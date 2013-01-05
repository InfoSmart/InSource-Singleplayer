//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
//=============================================================================//

#ifndef NPC_BULLSQUID_H
#define NPC_BULLSQUID_H

#include "AI_BaseNPC.h"

class CNPC_Bullsquid : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_Bullsquid, CAI_BaseNPC);
	DECLARE_DATADESC();

public:
	void Spawn();
	void Precache();
	Class_T	Classify();
	
	void IdleSound();
	void PainSound(const CTakeDamageInfo &info);
	void AlertSound();
	void DeathSound(const CTakeDamageInfo &info);
	void AttackSound();
	void GrowlSound();

	float MaxYawSpeed();
	void HandleAnimEvent(animevent_t *pEvent);

	int RangeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);

	bool FValidateHintType(CAI_Hint *pHint);
	void RemoveIgnoredConditions();
	Disposition_t IRelationType(CBaseEntity *pTarget);
	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);

	int GetSoundInterests();
	void RunAI();
	virtual void OnListened();

	int SelectSchedule();
	bool FInViewCone ( Vector pOrigin );

	void StartTask ( const Task_t *pTask );
	void RunTask ( const Task_t *pTask );

	NPC_STATE SelectIdealState();

	DEFINE_CUSTOM_AI;

private:
	
	bool  m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 
	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
	int   m_nSquidSpitSprite;
	float m_flHungryTime;// set this is a future time to stop the monster from eating for a while. 

	float m_nextSquidSoundTime;
	Vector	m_vecSaveSpitVelocity;	// Saved when we start to attack and used if we failed to get a clear shot once we release
};

#endif // NPC_BULLSQUID_H