//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Infectado
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#ifndef NPC_INFECTED_H
#define NPC_INFECTED_H

#include "ai_basenpc.h"
#include "ai_behavior_actbusy.h"
#include "ai_blended_movement.h"

//=========================================================
//=========================================================

typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseInfected;

class CInfected : public CAI_BaseInfected
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CInfected, CAI_BaseInfected);

	void Spawn();
	void Precache();
	void Think();

	Class_T	Classify();

	void IdleSound();
	void PainSound(const CTakeDamageInfo &info);
	void AlertSound();
	void DeathSound(const CTakeDamageInfo &info);

	void HandleAnimEvent(animevent_t *pEvent);
	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	void MeleeAttack(bool highAttack = false);

	int MeleeAttack1Conditions(float flDot, float flDist);
	int MeleeAttack2Conditions(float flDot, float flDist);
};

#endif