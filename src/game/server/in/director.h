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

	void Precache();
	void Think();

	void SetStatus(int status);
	int Status();

	void Relaxed();
	void Horde(bool super = false);
	void Climax();

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
	void InputSetMakerProximity(inputdata_t &inputdata);
	void InputSetHordeQueue(inputdata_t &inputdata);

	/* FUNCIONES RELACIONADAS AL IA DEL DIRECTOR */
	bool MayQueueZombies();
	bool MayQueueHordeZombies();
	bool MayQueueGrunt();
	
	/* FUNCIONES RELACIONADAS AL DIRECTOR ZOMBIE MAKER */
	int CountZombies();
	void CheckZombieMaker();
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

	DECLARE_DATADESC();

private:

	CSoundPatch *pSound;
	
	int		m_status;
	int		m_left4Relax;
	int		m_left4Horde;
	int		t_blockedSpawn;
	int		t_disabledDirector;

	int			m_gruntsAlive;
	bool		m_gruntsMusic;
	CSoundPatch *s_gruntMusic;
	bool		m_gruntSpawnPending;

	bool		m_playingHordeMusic;
	CSoundPatch *s_HordeMusic;
	bool		m_playingHordeAMusic;
	CSoundPatch *s_HordeAMusic;
	bool		m_playingHordeBMusic;
	CSoundPatch *s_HordeBMusic;

	int	t_lastSpawnZombis;
	int m_zombiesHordeQueue;

	int m_zombiesAlive;
	int m_zombiesTargetPlayer;
	int m_zombieSpawnQueue;
	int m_zombiesSpawned;
	int m_zombiesExaltedSpawned;

	int m_zombiesClassicAlive;
	int m_zombinesAlive;
	int m_zombiesFastAlive;
	int m_zombiesPoisonAlive;
};

inline CInDirector *ToBaseDirector(CBaseEntity *pEntity)
{
	if (!pEntity)
		return NULL;

#if _DEBUG
	return dynamic_cast<CInDirector *>(pEntity);
#else
	return static_cast<CInDirector *>(pEntity);
#endif

}

#endif // INDIRECTOR_H