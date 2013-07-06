#ifndef IN_UTILS_H

#define IN_UTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

// FUNCIONES RELACIONADAS A LA MUSICA
CSoundPatch *CreateMusic(const char *pName, CBasePlayer *pPlayer = NULL);
CSoundPatch *EmitMusic(const char *pName, CBasePlayer *pPlayer = NULL);
void StopMusic(CSoundPatch *pMusic);
void VolumeMusic(CSoundPatch *pMusic, float newVolume);
void FadeoutMusic(CSoundPatch *pMusic, float range = 1.5f, bool destroy = false);

// FUNCIONES RELACIONADAS A LOS JUGADORES
CIN_Player	*UTIL_InPlayerByIndex(int playerIndex);
CIN_Player *UTIL_GetLocalInPlayer();
CIN_Player *UTIL_GetRandomInPlayer();
CIN_Player *UTIL_GetNearestInPlayer(const Vector &pos, float &nearestDistance);

int UTIL_GetPlayersHealth();
int UTIL_GetPlayersBattery();

bool UTIL_IsPlayersVisible(Vector origin);
bool UTIL_IsPlayersVisible(CBaseEntity *pEntity);

bool UTIL_InPlayersViewCone(Vector origin);
bool UTIL_InPlayersViewCone(CBaseEntity *pEntity);

bool UTIL_IsPlayersVisibleCone(Vector origin);
bool UTIL_IsPlayersVisibleCone(CBaseEntity *pEntity);

#endif