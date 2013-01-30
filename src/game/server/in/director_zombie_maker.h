#ifndef NPC_ZOMBIE_MAKER_H

#define NPC_ZOMBIE_MAKER_H

#ifdef _WIN32
#pragma once
#endif

#include "monstermaker.h"

class CNPCZombieMaker : public CBaseEntity
{
public:
	DECLARE_CLASS(CNPCZombieMaker, CBaseEntity);

	CNPCZombieMaker();

	void Spawn();
	void Precache();

	void Enable();
	void Disable();

	bool CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult);
	void ChildPostSpawn(CAI_BaseNPC *pChild);
	CAI_BaseNPC *MakeNPC(bool super = false);
	CAI_BaseNPC *MakeGrunt();
	bool CanMakeGrunt() { return m_SpawnGrunt; }

	void DeathNotice(CBaseEntity *pVictim);
	const char *SelectRandomZombie();

	int DrawDebugTextOverlays();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputSpawn(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	DECLARE_DATADESC();

private:
	COutputEvent m_OnSpawnNPC;
	COutputEvent m_OnNPCDead;

	bool m_Enabled;

	bool m_SpawnClassicZombie;
	bool m_SpawnFastZombie;
	bool m_SpawnPoisonZombie;
	bool m_SpawnZombine;
	bool m_SpawnGrunt;

	int m_Childs;
	int m_ChildsAlive;
	int m_ChildsKilled;
	float m_Radius;
};

#endif