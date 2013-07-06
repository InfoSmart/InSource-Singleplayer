#ifndef DIRECTOR_H

#define DIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "scripted.h"

//=========================================================
// Estados del Director
//=========================================================
enum DirectorStatus
{
	RELAXED = 0,	// Relajado: Vamos con calma.
	EXALTED,		// Exaltado: Algunos zombis por aquí y una música aterradora.
	HORDE,			// Horda: ¡Ataquen mis valientes!
	BOSS,			// BOSS: Estado especial indicando la aparición de un Jefe.
	CLIMAX,			// Climax: ¡¡Se nos va!! ¡Ataquen sin compasión carajo!
	WAIT			// En espera: Estado especial para el final.
};

//=========================================================
// Estados de enojo de InDirector
//=========================================================
enum DirectorAngry
{
	HAPPY = 0,
	UNCOMFORTABLE,
	ANGRY,
	FURIOUS
};

class CDirector : public CLogicalEntity
{
public:
	DECLARE_CLASS(CDirector, CLogicalEntity);
	DECLARE_DATADESC();

	CDirector();
	~CDirector();

	bool PlayingHordeMusic() { return ( Horde_Music == NULL ) ? false : true; }
	bool PlayingDangerMusic() { return ( Danger_Music == NULL ) ? false : true; }
	bool PlayingDanger_A_Music() { return ( Danger_A_Music == NULL ) ? false : true; }
	bool PlayingDanger_B_Music() { return ( Danger_B_Music == NULL ) ? false : true; }
	bool PlayingBossMusic() { return ( Boss_Music == NULL ) ? false : true; }
	bool PlayingClimaxMusic() { return ( Climax_Music == NULL ) ? false : true; }
	

	void Spawn();
	void Precache();
	void Think();
	void FinalThink();

	void Enable();
	void Disable();

	DirectorStatus GetStatus() { return Status; };
	void SetStatus(DirectorStatus status);

	DirectorAngry GetAngryLevel() { return AngryLevel; };
	int UpdateAngryLevel();

	bool IsTooFar(CAI_BaseNPC *pNPC);
	bool IsTooClose(CAI_BaseNPC *pNPC);

	void FinalEnd();
	void StopMusic();

	void Relaxed();
	void Horde(bool super = false, bool triggered = false);
	void Climax(bool mini = false);
	void Wait();

	const char *MS();
	const char *GetStatusName(DirectorStatus status);
	const char *GetStatusName();

	/* FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS) */
	void InputForceRelax(inputdata_t &inputdata);
	void InputForceExalted(inputdata_t &inputdata);
	void InputForceHorde(inputdata_t &inputdata);
	void InputForceTriggeredHorde(inputdata_t &inputdata);
	void InputForceBoss(inputdata_t &inputdata);
	void InputForceClimax(inputdata_t &inputdata);
	void InputForceSpawnBoss(inputdata_t &inputdata);

	void InputStartFinal(inputdata_t &inputdata);
	void InputEndFinal(inputdata_t &inputdata);

	void InputDisclosePlayer(inputdata_t &inputdata);
	void InputKillChilds(inputdata_t &inputdata);
	void InputKillNoVisibleChilds(inputdata_t &inputdata);

	void InputSetDisabledFor(inputdata_t &inputdata);
	void InputSetMakerDistance(inputdata_t &inputdata);
	void InputSetHordeQueue(inputdata_t &inputdata);

	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

	void InputEnableMusic(inputdata_t &inputdata);
	void InputDisableMusic(inputdata_t &inputdata);

	/* FUNCIONES RELACIONADAS A LOS ZOMBIS */
	int GetMaxChildsScale();
	int CountChilds();
	bool MayQueueChilds();
	bool MayQueueHordeChilds();
	void CheckChilds();
	void SpawnChilds();
	void ChildKilled(CBaseEntity *pVictim);
	void KillChilds(bool onlyNoVisible = false);

	/* FUNCIONES RELACIONADAS AL IA DEL DIRECTOR */
	void DisclosePlayer();

	/* FUNCIONES RELACIONADAS AL GRUNT */
	bool MayQueueBoss();
	int CountBoss();
	void CheckBoss();
	void SpawnBoss(int pCount = 1);
	void MBossKilled(CBaseEntity *pVictim) { BossKilled++; };

	/* FUNCIONES RELACIONADAS AL SONIDO/MUSICA */
	void EmitDangerMusic(bool A, bool B);
	void FadeDangerMusic();
	void FadeBossMusic();

	/* FUNCIONES RELACIONADAS A OTROS */
	int DrawDebugTextOverlays();

	bool AllowMusic;
	bool MuteOtherSounds; // @TODO: Mover a un mejor lugar.

private:

	COutputEvent OnSpawnChild;
	COutputEvent OnChildDead;
	COutputEvent OnSpawnBoss;
	COutputEvent OnBossDead;
	COutputEvent OnFinalStart;
	COutputEvent OnClimax;

	DirectorStatus		Status;
	DirectorAngry		AngryLevel;

	int		Left4Exalted;
	int		Left4Horde;
	bool	TriggerHorde;
	bool	Disabled;
	int		SpawnBlocked;
	int		DirectorDisabled;

	CSoundPatch *Boss_Music;
	int			BossAlive;
	bool		BossSpawnPending;
	int			BossSpawnPendingCount;
	int			BossKilled;
	
	CSoundPatch *pSound;
	CSoundPatch *Horde_Music;
	CSoundPatch *Danger_Music;
	CSoundPatch *Danger_A_Music;
	CSoundPatch *Danger_B_Music;
	CSoundPatch *Climax_Music;

	string_t SoundClimax;
	string_t SoundMiniClimax;
	string_t SoundHorde;
	string_t SoundDanger;
	string_t SoundDangerA;
	string_t SoundDangerB;
	string_t SoundBoss;
	string_t SoundBossBrothers;
	string_t SoundStartFinal;
	string_t SoundEscape;

	int MaxChildAlive;
	int	LastSpawnChilds;

	int HordeQueue;
	int HordesPassed;
	int LastHordeChilds;
	int LastHordeDisclose;

	int ChildsAlive;
	int ChildsKilled;
	int ChildsTargetPlayer;
	int SpawnQueue;
	int ChildsSpawned;
	int ChildsExaltedSpawned;

	bool ChildChoir;

	bool InFinal;
	int FinalWaves;
	int FinalWavesLeft;
	int FinalBoss;
	float ExitWait;
};

inline CDirector *ToBaseDirector(CBaseEntity *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<CDirector *>(pEntity);
	#else
		return static_cast<CDirector *>(pEntity);
	#endif
}

inline CDirector *GetDirector()
{
	CDirector *pDirector = (CDirector *)gEntList.FindEntityByClassname(NULL, "info_director");

	if( !pDirector )
		return NULL;

	return pDirector;
}

#endif // DIRECTOR_H