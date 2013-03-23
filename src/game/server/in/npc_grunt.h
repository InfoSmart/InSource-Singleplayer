//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BASE
//
// Usa este archivo para hacer una base al crear un nuevo NPC.
//
//=====================================================================================//

#ifndef NPC_GRUNT_H
#define NPC_GRUNT_H

#include "ai_basenpc.h"
#include "ai_behavior_actbusy.h"
#include "ai_blended_movement.h"

//=========================================================
//=========================================================

typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseGrunt;

class CNPC_Grunt : public CAI_BaseGrunt
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CNPC_Grunt, CAI_BaseGrunt);

	void Spawn();
	void Precache();
	Class_T	Classify();

	void IdleSound();
	void PainSound(const CTakeDamageInfo &info);
	void AlertSound();
	void DeathSound(const CTakeDamageInfo &info);
	void AttackSound(bool highAttack = false);

	float MaxYawSpeed();
	void HandleAnimEvent(animevent_t *pEvent);

	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	void MeleeAttack(bool highAttack = false);
	//void RangeAttack1();

	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);
	//int RangeAttack1Conditions(float flDot, float flDist);

	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);
	void Event_Killed(const CTakeDamageInfo &info);

	int SelectCombatSchedule();
	int SelectSchedule();
	int TranslateSchedule(int scheduleType);
	void GatherConditions();

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	bool FindNearestPhysicsObject();
	float DistToPhysicsEnt();
	bool IsPhysicsObject(CBaseEntity *pEntity);

	NPC_STATE SelectIdealState();
	DEFINE_CUSTOM_AI;

	static int ACT_SWATLEFTMID;

protected:
	EHANDLE PhysicsEnt;

private:
	float LastHurtTime;
	float NextAlertSound;
	float NextPainSound;

	float NextRangeAttack1;

	float NextThrow;
	bool PhysicsCanThrow;
};

#endif // NPC_GRUNT_H