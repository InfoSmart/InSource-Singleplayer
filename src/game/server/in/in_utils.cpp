//=====================================================================================//
//
// Sistema encargado de constrolar al personaje.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"

#include "in_player.h"
#include "in_utils.h"

#include "tier0/memdbgon.h"

// Necesario para la música de fondo.
extern ISoundEmitterSystemBase *soundemitterbase;

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA MUSICA
//=========================================================
//=========================================================

CSoundPatch *CreateMusic(const char *pName, CBasePlayer *pPlayer)
{
	CSoundParameters params;

	if ( !soundemitterbase->GetParametersForSound(pName, params, GENDER_NONE) )
		return NULL;

	CSoundPatch *pMusic = NULL;

	if ( pPlayer )
	{
		CSingleUserRecipientFilter filter(pPlayer);
		filter.MakeReliable();

		pMusic = ENVELOPE_CONTROLLER.SoundCreate(filter, pPlayer->entindex(), params.channel, params.soundname, params.soundlevel);
	}
	else
	{
		CReliableBroadcastRecipientFilter filter;
		pMusic = ENVELOPE_CONTROLLER.SoundCreate(filter, -1, params.channel, params.soundname, params.soundlevel);
	}

	if ( !pMusic )
		return NULL;

	return pMusic;
}

//=========================================================
// Inicia una música de fondo
//=========================================================
CSoundPatch *EmitMusic(const char *pName, CBasePlayer *pPlayer)
{
	CSoundPatch *pMusic = CreateMusic(pName, pPlayer);

	if ( !pMusic )
		return NULL;

	CSoundParameters params;

	if ( !soundemitterbase->GetParametersForSound(pName, params, GENDER_NONE) )
		return NULL;

	ENVELOPE_CONTROLLER.Play(pMusic, params.volume, params.pitch);
	return pMusic;
}

//=========================================================
// Parar una música de fondo
//=========================================================
void StopMusic(CSoundPatch *pMusic)
{
	if ( !pMusic )
		return;

	VolumeMusic(pMusic, 0);
}

//=========================================================
// Cambia el volumen de una música de fondo
//=========================================================
void VolumeMusic(CSoundPatch *pMusic, float newVolume)
{
	if ( !pMusic )
		return;

	if ( newVolume > 0 )
		ENVELOPE_CONTROLLER.SoundChangeVolume(pMusic, newVolume, 0);
	else
		ENVELOPE_CONTROLLER.SoundDestroy(pMusic);
}

//=========================================================
// Realiza el efecto de "desaparecer poco a poco" en una
// música de fondo. Aceptando un rango de desaparición.
//=========================================================
void FadeoutMusic(CSoundPatch *pMusic, float range, bool destroy)
{
	if ( !pMusic )
		return;

	ENVELOPE_CONTROLLER.SoundFadeOut(pMusic, range, destroy);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LOS JUGADORES
//=========================================================
//=========================================================

//=========================================================
// Devuelve al jugador con la Index especificada.
//=========================================================
CIN_Player	*UTIL_InPlayerByIndex(int playerIndex)
{
	CIN_Player *pPlayer = NULL;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);

		if ( pPlayerEdict && !pPlayerEdict->IsFree() )
			pPlayer = (CIN_Player*)GetContainingEntity(pPlayerEdict);
	}
	
	return pPlayer;
}

//=========================================================
// Devuelve el jugador local.
// Si es llamado desde Multiplayer se devolverá el primer
// jugador.
//=========================================================
CIN_Player *UTIL_GetLocalInPlayer()
{
	if ( gpGlobals->maxClients > 1 )
	{
		//DevWarning( "UTIL_GetLocalInPlayer() called in multiplayer game.\n" );
	}

	return UTIL_InPlayerByIndex(1);
}

//=========================================================
// Devuelve un jugador al azar.
//=========================================================
CIN_Player *UTIL_GetRandomInPlayer()
{
	int pPlayers[34]		= {};
	int pCount				= 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		pPlayers[pCount] = i;
		++pCount;
	}

	int pRandom = random->RandomInt(0, ARRAYSIZE(pPlayers) - 1);
	CIN_Player *pPlayer = UTIL_InPlayerByIndex(pRandom);

	if ( !pPlayer )
		pPlayer = UTIL_InPlayerByIndex(1);

	if ( !pPlayer )
		return NULL;

	return pPlayer;
}

//=========================================================
// Devuelve al jugador más cercano.
//=========================================================
CIN_Player *UTIL_GetNearestInPlayer(const Vector &pos, float &nearestDistance)
{
	CIN_Player *pNearest	= NULL;
	nearestDistance			= 0.0f;
	float distance			= 0.0f;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;
			
		// Distancia del jugador al objeto.
		distance = pPlayer->GetAbsOrigin().DistTo(pos);

		if ( nearestDistance == 0.0f || distance < nearestDistance )
		{
			nearestDistance = distance;
			pNearest		= pPlayer;
		}
	}

	return pNearest;
}

//=========================================================
// Devuelve el porcentaje de salud entre todos los jugadores.
//=========================================================
int UTIL_GetPlayersHealth()
{
	int pHealth		= 0;
	int pPlayers	= 0;

	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Un jugador más
		++pPlayers;
		pHealth = pHealth + pPlayer->GetHealth();
	}

	// Dividimos la salud total entre el número de jugadores.
	if ( pHealth > 0 )
		pHealth = pHealth / pPlayers;

	return pHealth;
}

//=========================================================
// Devuelve el porcentaje de bateria entre todos los jugadores.
//=========================================================
int UTIL_GetPlayersBattery()
{
	int pBattery	= 0;
	int pPlayers	= 0;

	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Un jugador más
		++pPlayers;
		pBattery = pBattery + pPlayer->GetBattery();
	}

	// Dividimos la bateria total entre el número de jugadores.
	if ( pBattery > 0 )
		pBattery = pBattery / pPlayers;

	return pBattery;
}

//=========================================================
// Verifica si alguno de los jugadores esta viendo este punto.
//=========================================================
bool UTIL_IsPlayersVisible(Vector origin)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Este jugador lo esta viendo o lo tiene visible.
		if ( pPlayer->FVisible(origin) )
			return true;
	}

	return false;
}

//=========================================================
// Verifica si alguno de los jugadores esta viendo una entidad
//=========================================================
bool UTIL_IsPlayersVisible(CBaseEntity *pEntity)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Este jugador lo esta viendo o lo tiene visible.
		if ( pPlayer->FVisible(pEntity) )
			return true;
	}

	return false;
}

//=========================================================
// Verifica si alguno de los jugadores tiene el punto
// en su cono de visualización.
//=========================================================
bool UTIL_InPlayersViewCone(Vector origin)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Este jugador lo tienen en su cono.
		if ( pPlayer->FInViewCone(origin) )
			return true;
	}

	return false;
}

//=========================================================
// Verifica si alguno de los jugadores tiene la entidad
// en su cono de visualización.
//=========================================================
bool UTIL_InPlayersViewCone(CBaseEntity *pEntity)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Este jugador lo tienen en su cono.
		if ( pPlayer->FInViewCone(pEntity) )
			return true;
	}

	return false;
}

//=========================================================
// Verifica si alguno de los jugadores tiene el punto
// en su cono de visualización o lo esta viendo.
//=========================================================
bool UTIL_IsPlayersVisibleCone(Vector origin)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Lo esta viendo o esta en su cono de visualización.
		if ( pPlayer->FVisible(origin) || pPlayer->FInViewCone(origin) )
			return true;
	}

	return false;
}

//=========================================================
// Verifica si alguno de los jugadores tiene el punto
// en su cono de visualización o lo esta viendo.
//=========================================================
bool UTIL_IsPlayersVisibleCone(CBaseEntity *pEntity)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Lo esta viendo o esta en su cono de visualización.
		if ( pPlayer->FVisible(pEntity) || pPlayer->FInViewCone(pEntity) )
			return true;
	}

	return false;
}