
#include "cbase.h"
#include "in_player.h"
#include "in_gamerules.h"
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
	CIn_Player *pPlayer = CIn_Player::CreatePlayer("player", pEdict);
	pPlayer->SetPlayerName(playername);
}

//=========================================================
// Returns the descriptive name of this .dll.  
// E.g., Half-Life, or Team Fortress 2
//=========================================================
const char *GetGameDescription()
{
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
	CreateGameRulesObject("CInSource");
}