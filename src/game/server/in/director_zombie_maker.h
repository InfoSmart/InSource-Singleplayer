#ifndef NPC_ZOMBIE_MAKER_H

#define NPC_ZOMBIE_MAKER_H

#ifdef _WIN32
#pragma once
#endif

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

	CAI_BaseNPC *MakeNPC(bool Horde = false, bool disclosePlayer = false);
	CAI_BaseNPC *MakeGrunt();
	bool CanMakeGrunt() { return SpawnGrunt; }

	void DeathNotice(CBaseEntity *pVictim);
	const char *SelectRandomZombie();

	int DrawDebugTextOverlays();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputSpawn(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	DECLARE_DATADESC();

public:
	float LastSpawn;

private:
	COutputEvent OnSpawnNPC;
	COutputEvent OnNPCDead;

	bool Enabled;

	bool SpawnClassicZombie;
	bool SpawnFastZombie;
	bool SpawnPoisonZombie;
	bool SpawnZombine;
	bool SpawnGrunt;

	int Childs;
	int ChildsAlive;
	int ChildsKilled;

	float SpawnRadius;
};

#endif