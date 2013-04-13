
#include "cbase.h"
#include "in_player.h"
#include "insp_gamerules.h"
#include "inmp_gamerules.h"
#include "gamerules.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// called each time a player is spawned into the game
//=========================================================
void ClientPutInServer(edict_t *pEdict, const char *playername)
{
	// Allocate a CBasePlayer for pev, and call spawn
	CIN_Player *pPlayer = CIN_Player::CreatePlayer("player", pEdict);
	pPlayer->SetPlayerName(playername);
}

//=========================================================
//=========================================================
void FinishClientPutInServer(CIN_Player *pPlayer)
{
	//pPlayer->InitialSpawn();
	//pPlayer->Spawn();

	char sName[128];
	Q_strncpy(sName, pPlayer->GetPlayerName(), sizeof(sName));

	// Removemos cualquier % y lo reemplazamos por un espacio.
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>");

	// @TODO: ¿Mensaje del día?
}

//=========================================================
//=========================================================
void ClientActive(edict_t *pEdict, bool bLoadGame)
{
	CIN_Player *pPlayer = ToInPlayer(CBaseEntity::Instance(pEdict));
	Assert(pPlayer);

	if ( !pPlayer )
		return;

	pPlayer->InitialSpawn();

	if ( !bLoadGame )
		pPlayer->Spawn();

	FinishClientPutInServer(pPlayer);
}

//=========================================================
// Returns the descriptive name of this .dll.  
// E.g., Half-Life, or Team Fortress 2
//=========================================================
const char *GetGameDescription()
{
	// Con el fin de detectar cuando los binarios se quedan en caché.
	// @TODO: Remover en Release
	Msg("InSource: v1.51 \r\n");

	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "InSource";
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	// Esta es la magia de tener 2 modos.

	if ( gpGlobals->coop || gpGlobals->deathmatch )
		CreateGameRulesObject("CInSourceMPRules");
	else
		CreateGameRulesObject("CInSourceSPRules");
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	// Multijugador
	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		CIN_Player *pPlayer = ToInPlayer(pEdict);

		if ( pPlayer )
		{
			//if ( fCopyCorpse )
				//pPlayer->CreateCorpse();

			if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
			{
				pPlayer->Spawn();
			}
			else
				pPlayer->SetNextThink(gpGlobals->curtime + 0.1f);
		}
	}

	// En un jugador, reiniciar todo el servidor.
	else
		engine->ServerCommand("reload\n");
}