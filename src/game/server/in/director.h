//=====================================================================================//
//
// Inteligencia artificial encargada de la creación de enemigos al rededor del jugador.
//
// Inspiración: I.A. Director de Left 4 Dead 2
//
//=====================================================================================//

#ifndef DIRECTOR_H

#define DIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "env_music.h"

//=========================================================
// Configuración
//=========================================================

#define CHILD_NAME		"director_child"
#define BOSS_NAME		"director_boss"

#define BOSS_MINTIME_TO_ATTACK 30

//=========================================================
// Estados del Director
//=========================================================
enum DirectorStatus
{
	RELAXED = 1,		// Relajado
	EXALTED,		// Exaltado
	PANIC,			// ¡Panico!
	BOSS,			// Jefe
	CLIMAX,			// Climax
	LAST			// Debe ser el último
};

class EnvMusic;

//=========================================================
// CDirector
//=========================================================
class CDirector : public CLogicalEntity
{
public:
	DECLARE_CLASS(CDirector, CLogicalEntity);
	DECLARE_DATADESC();

	// Constructores
	CDirector();
	~CDirector();

	// Devoluciones
	int GetQueue() { return SpawnQueue; }

	// Inicialización
	virtual void Init();
	virtual void InitMusic();

	virtual void Spawn();
	virtual void Precache();
	virtual void Think();

	virtual void DeathNotice(CBaseEntity *pVictim);

	// Reinicios y utilidades.
	virtual void RestartExalted();
	virtual void RestartPanic();

	virtual bool IsTooFar(CAI_BaseNPC *pNPC);
	virtual bool IsTooClose(CAI_BaseNPC *pNPC);

	// Provocación de estado.
	virtual void Relaxed();
	virtual void Panic(bool super = false, bool infinite = false);
	virtual void Climax(bool mini = false);

	// Estado
	virtual const char *Ms();
	virtual void SetStatus(DirectorStatus status);
	virtual const char *GetStatus(DirectorStatus status);
	virtual const char *GetStatus();

	// Hijos
	virtual float GetMaxChilds();
	virtual int CountChilds();

	virtual bool QueueChilds();
	virtual bool QueuePanic();
	virtual void HandleChilds();

	// Jefes
	virtual int CountBoss();

	virtual bool QueueBoss();
	virtual void HandleBoss();

	// Música
	virtual void HandleMusic();

	virtual void EmitDangerMusic(bool A = false, bool B = false);
	virtual void StopDangerMusic();

	// Depuración
	virtual int DrawDebugTextOverlays();

public:

	// Outputs
	COutputEvent OnClimax;

	// Estado
	DirectorStatus Status;
	bool Spawning;

	// Inicialización
	int Disabled4Seconds;
	bool Disabled;
	int LastDisclose;

	// Relajado y Exaltado.
	CountdownTimer Left4Exalted;

	// Horda/Panico
	CountdownTimer Left4Panic;
	int PanicQueue;
	int PanicChilds;
	bool InfinitePanic;
	int PanicCount;

	// Hijos
	int SpawnQueue;
	int ChildsSpawned;
	int ChildsAlive;
	int ChildsTooClose;
	int ChildsKilled;

	// Jefes
	bool BossPendient;
	int BossSpawned;
	int BossAlive;
	int BossKilled;

	// Sonidos
	string_t Sounds[10];

	EnvMusic *DangerMusic;
	EnvMusic *DangerMusic_A;
	EnvMusic *DangerMusic_B;

	EnvMusic *HordeMusic;

	EnvMusic *ClimaxMusic;
	EnvMusic *BossMusic;
};

inline CDirector *Director()
{
	CDirector *pDirector = (CDirector *)gEntList.FindEntityByClassname(NULL, "info_director");

	if ( !pDirector )
		return NULL;

	return pDirector;
}

#endif // DIRECTOR_H