#ifndef DIRECTOR_SPAWN_H

#define DIRECTOR_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

#include "env_sound.h"

class CDirectorSpawn : public CBaseEntity
{
public:
	DECLARE_CLASS(CDirectorSpawn, CBaseEntity);

	CDirectorSpawn();

	void Spawn();
	void Precache();

	void Enable();
	void Disable();

	bool CanMakeNPC(CAI_BaseNPC *pNPC, Vector *pResult);
	bool PostSpawn(CAI_BaseNPC *pNPC);
	void AddHealth(CAI_BaseNPC *pNPC);
	CAI_BaseNPC *VerifyClass(const char *pClass);

	CAI_BaseNPC *MakeNPC(bool Horde = false, bool disclosePlayer = false, bool checkRadius = true);
	CAI_BaseNPC *MakeNoCollisionNPC(bool Horde = false, bool disclosePlayer = false);
	CAI_BaseNPC *MakeBoss();

	bool Enabled() { return ( Disabled ) ? false : true; }
	bool CanMakeBoss();

	void DeathNotice(CBaseEntity *pVictim);
	const char *SelectRandom();
	const char *SelectRandomBoss();

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
	float SpawnRadius;

	int Childs;
	int ChildsAlive;
	int ChildsKilled;

protected:
	string_t iNpcs[8];
	string_t iBoss[3];

	/*
	const char *NPC1;
	const char *NPC2;
	const char *NPC3;
	const char *NPC4;
	const char *NPC5;
	const char *NPC6;
	const char *NPC7;
	const char *NPC8;

	const char *BOSS1;
	const char *BOSS2;
	const char *BOSS3;
	*/
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