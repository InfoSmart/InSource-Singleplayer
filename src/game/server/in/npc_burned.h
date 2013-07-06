//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// NPC BURNED
//
// Zombi quemado capaz de derribarte con 1 golpe.
// Inspiración: WITCH de Left 4 Dead
//
//=====================================================================================//

#ifndef NPC_BURNED_H
#define NPC_BURNED_H

#ifdef WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_behavior_actbusy.h"
#include "ai_blended_movement.h"

//=========================================================
//=========================================================

class CNPC_Burned : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CNPC_Burned, CAI_BaseNPC);
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void NPCInit();
	virtual void Precache();
	virtual void Think();

	Class_T Classify();

	virtual void StartIrk();
	virtual int	OnTakeDamage_Alive(const CTakeDamageInfo &info);

	virtual void HandleAnimEvent(animevent_t *pEvent);

	// Calculo y permisos.
	virtual bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	// Ataque
	virtual int MeleeAttack1Conditions(float flDot, float flDist);

	// Animaciones
	Activity NPC_TranslateActivity(Activity baseAct);

private:
	bool IsIrked;
	bool HasIrked;
	CountdownTimer DetectIrkedTime;
};

#endif // NPC_BURNED_H