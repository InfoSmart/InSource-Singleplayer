//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Inicia el cliente y establece las reglas del juego para cada modo.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "fr_player.h"
#include "fr_gamerules.h"

#include "tier0/vprof.h"

//=========================================================
// Un jugador ha entrado al servidor.
//=========================================================
void ClientPutInServer(edict_t *pEdict, const char *playername)
{
	CIN_Player *pPlayer = CIN_Player::CreatePlayer("player", pEdict);
	pPlayer->SetPlayerName(playername);
}

//=========================================================
// Configura y termina de agregar al jugador.
//=========================================================
void FinishClientPutInServer(CIN_Player *pPlayer)
{
	char sName[128];
	Q_strncpy(sName, pPlayer->GetPlayerName(), sizeof(sName));

	// Removemos cualquier % y lo reemplazamos por un espacio.
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// Imprimimos un mensaje en la consola.
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>");
}

//=========================================================
//=========================================================
void ClientActive(edict_t *pEdict, bool bLoadGame)
{
	CFR_Player *pPlayer = ToFRPlayer(CBaseEntity::Instance(pEdict));
	Assert( pPlayer );

	if ( !pPlayer )
		return;

	// Creamos al jugador.
	pPlayer->InitialSpawn();

	// Si no esta carganado una partida guardada.
	if ( !bLoadGame )
	{
		// Spawn!
		pPlayer->Spawn();

		// Terminamos de agregarlo.
		FinishClientPutInServer(pPlayer);
	}
}

//=========================================================
// Devuelve el nombre del motor gráfico.
//=========================================================
const char *GetGameDescription()
{
	if ( g_pGameRules )
		return g_pGameRules->GetGameDescription();
	else
		return "Furnance Alpha";
}

//=========================================================
// Inicia e instala las reglas del juego.
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject("CFrGameRules");
}

//=========================================================
//=========================================================
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	engine->ServerCommand("reload\n");
}