//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "hl2_player.h"
#include "hl2_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "EntityList.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say(edict_t *pEdict, bool teamonly);

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer, char *classname );
extern bool			g_fGameOver;

#ifdef HL2_EPISODIC
extern ConVar gamerules_survival;
#endif

//=========================================================
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//=========================================================
CBaseEntity* FindEntity(edict_t *pEdict, char *classname)
{
	// If no name was given set bits based on the picked
	if ( FStrEq(classname, "") ) 
		return (FindPickerEntityClass(static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname ));

	return NULL;
}

//=========================================================
// Purpose: Precache game-specific models & sounds
//=========================================================
void ClientGamePrecache()
{
	CBaseEntity::PrecacheModel("models/gibs/agibs.mdl");
	CBaseEntity::PrecacheModel("models/weapons/v_hands.mdl");

	CBaseEntity::PrecacheScriptSound("HUDQuickInfo.LowAmmo");
	CBaseEntity::PrecacheScriptSound("HUDQuickInfo.LowHealth");

	CBaseEntity::PrecacheScriptSound("FX_AntlionImpact.ShellImpact");
	CBaseEntity::PrecacheScriptSound("Missile.ShotDown");
	CBaseEntity::PrecacheScriptSound("Bullets.DefaultNearmiss");
	CBaseEntity::PrecacheScriptSound("Bullets.GunshipNearmiss");
	CBaseEntity::PrecacheScriptSound("Bullets.StriderNearmiss");
	
	CBaseEntity::PrecacheScriptSound("Geiger.BeepHigh");
	CBaseEntity::PrecacheScriptSound("Geiger.BeepLow");
}

//=========================================================
// called by ClientKill and DeadThink
//=========================================================
/*
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		// make a copy of the dead body for appearances sake
		if ( fCopyCorpse )
			((CHL2_Player *)pEdict)->CreateCorpse();

		// respawn player
		pEdict->Spawn();
	}

	// restart the entire server
	else
		engine->ServerCommand("reload\n");
}
*/

//=========================================================
//=========================================================
void GameStartFrame()
{
	VPROF("GameStartFrame()");

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = (teamplay.GetInt() != 0);
}