//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Inicia el cliente y establece las reglas del juego para cada modo.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "in_player.h"
#include "gamerules.h"
#include "viewport_panel_names.h"

// Reglas del juego
#include "scp_gamerules.h"			// Modo Historia.
//#include "scpcoop_gamerules.h"		// Modo Coop @TODO

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

	pPlayer->EquipSuit();
}

//=========================================================
//=========================================================
void ClientActive(edict_t *pEdict, bool bLoadGame)
{
	CIN_Player *pPlayer = ToInPlayer(CBaseEntity::Instance(pEdict));
	Assert( pPlayer );

	if ( !pPlayer )
		return;

	// Creamos al jugador.
	pPlayer->InitialSpawn();

	// Si no esta cargando una partida guardada.
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
		return "SCP: Source Alpha";
}

//=========================================================
// Inicia e instala las reglas del juego.
//=========================================================
void InstallGameRules()
{
	// Modo Coop
	if ( gpGlobals->coop )
		CreateGameRulesObject("CSCPCoopGameRules");

	// Modo Historia
	else
		CreateGameRulesObject("CSCPGameRules");
}

//=========================================================
//=========================================================
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	// Modo Coop
	if ( gpGlobals->coop )
	{
		CIN_Player *pPlayer = ToInPlayer(pEdict);

		if ( pPlayer )
		{
			if ( gpGlobals->curtime > pPlayer->GetDeathTime() + 15.0f )
				pPlayer->Spawn();
			else
				pPlayer->SetNextThink(gpGlobals->curtime + 0.1f);
		}
	}

	// Modo Historia: Recargar todo el servidor.
	else
		engine->ServerCommand("reload\n");
}