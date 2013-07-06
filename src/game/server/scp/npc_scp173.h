#ifndef NPC_SCP173_H
#define NPC_SCP173_H

#include "ai_basenpc.h"
#include "ai_behavior_actbusy.h"
#include "ai_blended_movement.h"

//=========================================================
//=========================================================


class CNPC_SCP173 : public CAI_BaseNPC
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CNPC_SCP173, CAI_BaseNPC);

	void Spawn();
	void Precache();
	void Think();

	Class_T	Classify();
	Disposition_t IRelationType(CBaseEntity *pTarget);

	void IdleSound();
	void AttackSound();

	float MaxYawSpeed();
	void HandleAnimEvent(animevent_t *pEvent);

	void MeleeAttack(bool highAttack = false);
	int MeleeAttack1Conditions(float flDot, float flDist);

	int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);
	void Event_Killed(const CTakeDamageInfo &info);

	int SelectCombatSchedule();
	int SelectSchedule();

	int TranslateSchedule(int scheduleType);
	Activity NPC_TranslateActivity(Activity baseAct);

	void GatherConditions();

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	NPC_STATE SelectIdealState();
	//DEFINE_CUSTOM_AI;

protected:
	EHANDLE m_ePhysicsEnt;

private:
	float m_fLastHurtTime;
	float m_fNextAlertSound;
	float m_fNextPainSound;
	float m_fNextSuccessDance;
	float m_fExpireThrow;

	float m_fNextRangeAttack1;

	float m_fNextThrow;
	bool m_bPhysicsCanThrow;
};

#endif // NPC_SCP173_H