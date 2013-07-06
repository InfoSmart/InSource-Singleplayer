//=====================================================================================//
//
// Inteligencia encargada de la creaci�n de NPC's y hordas.
//
// Inspiraci�n: I.A. Director de Left 4 Dead 2
//
//=====================================================================================//

#ifndef DIRECTOR_MANAGER_H

#define DIRECTOR_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"

enum
{
	SPAWN_NONE = 0,
	SPAWN_NO_COLLISION,
	SPAWN_POWERFUL,
	SPAWN_NO_COLLISION_AND_POWERFUL
};

//=========================================================
// Director_Child
// Administra los posibles NPC's que crear� el Director.
//=========================================================
class Director_Child
{
public:
	Director_Child(const char *pName, int pPorcent = 50) 
	{
		pChildName		= pName;
		psChildName		= MAKE_STRING(pName);
		pChildPorcent	= pPorcent;
	};

	const char *pChildName;
	string_t psChildName;
	int pChildPorcent;
};

//=========================================================
// CDirector_Manager
//=========================================================
class CDirector_Manager
{
public:
	CDirector_Manager();
	~CDirector_Manager();

	// Inicializaci�n
	virtual void Init();

	// Utilidades
	virtual void Disclose();

	// Ubicaci�n
	CAI_Network *GetNetwork();
	virtual void UpdateNodes();
	virtual CAI_Node *GetRandomNode();

	// Verificaci�n
	virtual bool CanMake(const Vector &vecPosition);
	virtual bool PostSpawn(CAI_BaseNPC *pNPC);

	// Creaci�n
	virtual const char *GetChildClass();
	virtual const char *GetBossClass();


	virtual void SpawnChilds();
	virtual void SpawnBoss();

	virtual void AddChild();
	virtual bool AddChild(const Vector &vecPosition, int flag = SPAWN_NONE);
	virtual void AddHorde(int pMount);

	virtual bool AddBoss(const Vector &vecPosition);

private:
	// Cronometros
	CountdownTimer CandidateUpdateTimer;

	// Nodos
	CUtlVector<int> SpawnNodes;
};

CDirector_Manager *DirectorManager();

static void DispatchActivate(CBaseEntity *pEntity)
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

#endif // DIRECTOR_MANAGER_H