#ifndef NPC_ZOMBIE_MAKER_H

#define NPC_ZOMBIE_MAKER_H

#ifdef _WIN32
#pragma once
#endif

#include "env_sound.h"

class CDirectorZombieSpawn : public CBaseEntity
{
public:
	DECLARE_CLASS(CDirectorZombieSpawn, CBaseEntity);

	CDirectorZombieSpawn();

	void Spawn();
	void Precache();

	void Enable();
	void Disable();

	bool CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult);
	void ChildPostSpawn(CAI_BaseNPC *pChild);

	CAI_BaseNPC *MakeNPC(bool Horde = false, bool disclosePlayer = false);
	CAI_BaseNPC *MakeNoCollisionNPC(bool Horde = false, bool disclosePlayer = false);
	CAI_BaseNPC *MakeGrunt();
	bool CanMakeGrunt() { return SpawnGrunt; }

	void DeathNotice(CBaseEntity *pVictim);
	const char *SelectRandomZombie();

	int DrawDebugTextOverlays();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputSpawn(inputdata_t &inputdata);
	void InputSpawnCount(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	DECLARE_DATADESC();

public:
	float LastSpawn;

private:
	COutputEvent OnSpawnNPC;
	COutputEvent OnChildDead;

	bool Disabled;

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

#define SF_SPAWN_CLASSIC		4
#define SF_SPAWN_FAST			8
#define SF_SPAWN_POISON			16
#define SF_SPAWN_ZOMBINE		32
#define SF_SPAWN_GRUNT			64

class CSurvivalZombieSpawn : public CBaseEntity
{
public:
	DECLARE_CLASS(CSurvivalZombieSpawn, CBaseEntity);
	CSurvivalZombieSpawn();

	void Think();
	void Spawn();
	void Precache();

	void Enable();
	void Disable();

	bool CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult);

	CAI_BaseNPC *MakeNPC();
	CAI_BaseNPC *MakeGrunt();
	bool CanMakeGrunt() { return HasSpawnFlags(SF_SPAWN_GRUNT); }

	void DeathNotice(CBaseEntity *pVictim);
	const char *SelectRandomZombie();

	int CountGrunts();
	int DrawDebugTextOverlays();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputSpawn(inputdata_t &inputdata);
	void InputSpawnCount(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	DECLARE_DATADESC();

public:
	float LastSpawn;

private:
	COutputEvent OnSpawnZombie;
	COutputEvent OnZombieDead;

	CEnvSound *pGruntMusic;
	bool Disabled;

	int Childs;
	int ChildsAlive;
	int ChildsKilled;

	float SpawnRadius;
};

static void DispatchActivate( CBaseEntity *pEntity )
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

#endif