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
#include "in_gamerules.h"			// Base Singleplayer y modo Historia.
#include "inmp_gamerules.h"			// Base Multiplayer
#include "insurvival_gamerules.h"	// Modo Survival
#include "coop_gamerules.h"			// Modo Coop

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_show_motd("sv_show_motd", "0", 0, "Muestra el mensaje del día.");

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

	// Actualizamos su inventario.
	pPlayer->ExecCommand("cl_update_inventory 1");
	
	// Mostrar el mensaje del día.
	if ( sv_show_motd.GetBool() )
	{
		const ConVar *hostname	= cvar->FindVar("hostname");
		const char *title		= (hostname) ? hostname->GetString() : "MENSAJE DEL DÍA";
		
		KeyValues *data = new KeyValues("data");
		data->SetString("title", title);		// info panel title
		data->SetString("type", "1");			// show userdata from stringtable entry
		data->SetString("msg",	"motd");		// use this stringtable entry

		pPlayer->ShowViewPortPanel(PANEL_INFO, true, data);
		data->deleteThis();
	}
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
		return "InSource Alpha";
}

//=========================================================
// Inicia e instala las reglas del juego.
//=========================================================
void InstallGameRules()
{
	// Modo Survival
	if ( gpGlobals->deathmatch )
		CreateGameRulesObject("CInSurvivalGameRules");

	// Modo Coop
	else if ( gpGlobals->coop )
		CreateGameRulesObject("CInCoopGameRules");

	// Modo Historia
	else
		CreateGameRulesObject("CInGameRules");
}

//=========================================================
//=========================================================
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	// Modo Survival
	// Después de morir, reaparece al jugador después de la animación de muerte.
	if ( gpGlobals->deathmatch )
	{
		CIN_Player *pPlayer = ToInPlayer(pEdict);

		if ( pPlayer )
		{
			if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
				pPlayer->Spawn();
			else
				pPlayer->SetNextThink(gpGlobals->curtime + 0.1f);
		}
	}

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