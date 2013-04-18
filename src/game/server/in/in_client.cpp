
#include "cbase.h"
#include "in_player.h"
#include "insp_gamerules.h"
#include "inmp_gamerules.h"
#include "gamerules.h"
#include "viewport_panel_names.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_show_motd("sv_show_motd", "0", 0, "Muestra el mensaje del día.");

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
	char sName[128];
	Q_strncpy(sName, pPlayer->GetPlayerName(), sizeof(sName));

	// Removemos cualquier % y lo reemplazamos por un espacio.
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>");

	pPlayer->ExecCommand("cl_update_inventory 1"); // Actualizamos el inventario.
	
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
	Msg("InSource: BUILD \r\n");

	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "InSource Alpha";
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	// Esta es la magia de tener 2 modos.
	// @TODO: Crear el modo Coop.

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
			if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
				pPlayer->Spawn();
			else
				pPlayer->SetNextThink(gpGlobals->curtime + 0.1f);
		}
	}

	// En un jugador, reiniciar todo el servidor.
	else
		engine->ServerCommand("reload\n");
}