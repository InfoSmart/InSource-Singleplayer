#ifndef INDIRECTOR_H

#define INDIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "scripted.h"

class CInDirector : public CLogicalEntity
{
public:
	DECLARE_CLASS(CInDirector, CLogicalEntity);

	CInDirector();
	~CInDirector();

	void Spawn();
	void SpawnScriptedSchedule();
	void Precache();
	void Think();

	void Enable();
	void Disable();

	int GetStatus() { return Status; };
	void SetStatus(int status);

	void Relaxed();
	void Horde(bool super = false);
	void Climax(bool mini = false);

	const char *MS();
	const char *GetStatusName(int status);
	const char *GetStatusName();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputForceRelax(inputdata_t &inputdata);
	void InputForceExalted(inputdata_t &inputdata);
	void InputForceHorde(inputdata_t &inputdata);
	void InputForceGrunt(inputdata_t &inputdata);
	void InputForceClimax(inputdata_t &inputdata);
	void InputForceSpawnGrunt(inputdata_t &inputdata);

	void InputSetDisabledFor(inputdata_t &inputdata);
	void InputSetMakerDistance(inputdata_t &inputdata);
	void InputSetHordeQueue(inputdata_t &inputdata);

	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	/* FUNCIONES RELACIONADAS AL IA DEL DIRECTOR */
	bool MayQueueZombies();
	bool MayQueueHordeZombies();
	bool MayQueueGrunt();

	/* FUNCIONES RELACIONADAS AL DIRECTOR ZOMBIE MAKER */
	int CountZombies();
	void CheckZombies();
	void SpawnZombies();
	void SpawnGrunt();

	/* FUNCIONES RELACIONADAS AL GRUNT */
	int CountGrunts();
	void CheckGrunts();

	/* FUNCIONES RELACIONADAS AL SONIDO/MUSICA */
	void EmitHordeMusic(bool A, bool B);
	void FadeoutHordeMusic();
	void FadeoutGruntMusic();

	/* FUNCIONES RELACIONADAS A OTROS */
	int DrawDebugTextOverlays();

	//=========================================================
	// Estados de InDirector
	//=========================================================

	enum
	{
		RELAXED = 0,	// Relajado: Vamos con calma.
		EXALTED,		// Exaltado: Algunos zombis por aqu� y una m�sica aterradora.
		HORDE,			// Horda: �Ataquen mis valientes!
		GRUNT,			// Grunt: Estado especial indicando la aparici�n de un Grunt.
		CLIMAX			// Climax: ��Se nos va!! �Ataquen sin compasi�n carajo!
	};

	DECLARE_DATADESC();

private:

	CSoundPatch *pSound;

	int		Status;
	int		Left4Exalted;
	int		Left4Horde;
	bool	Disabled;
	int		SpawnBlocked;
	int		DirectorDisabled;

	int			GruntsAlive;
	bool		GruntsMusic;
	CSoundPatch *Sound_GruntMusic;
	bool		GruntSpawnPending;

	bool		PlayingHordeMusic;
	CSoundPatch *Sound_HordeMusic;
	bool		PlayingHorde_A_Music;
	CSoundPatch *Sound_Horde_A_Music;
	bool		PlayingHorde_B_Music;
	CSoundPatch *Sound_Horde_B_Music;

	int	LastSpawnZombies;
	int HordeQueue;

	int ZombiesAlive;
	int ZombiesTargetPlayer;
	int SpawnQueue;
	int ZombiesSpawned;
	int ZombiesExaltedSpawned;

	int ZombiesClassicAlive;
	int ZombinesAlive;
	int ZombiesFastAlive;
	int ZombiesPoisonAlive;
};

inline CInDirector *ToBaseDirector(CBaseEntity *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<CInDirector *>(pEntity);
	#else
		return static_cast<CInDirector *>(pEntity);
	#endif
}

inline CInDirector *GetDirector()
{
	CInDirector *pDirector = (CInDirector *)gEntList.FindEntityByClassname(NULL, "info_director");

	if( !pDirector )
		return NULL;

	return pDirector;
}

#endif // INDIRECTOR_H