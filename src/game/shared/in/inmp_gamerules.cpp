//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego:
// Base para Multiplayer
//
// Define la configuración de las amistades, reaparición, objetos, etc...
//
// Con el fin de evitar la creación de archivos innecesarios, las reglas de Apocalypse
// fueron puestas aquí, si hará un MOD distinto solo cambielas.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "inmp_gamerules.h"
#include "basehlcombatweapon.h"

#ifndef CLIENT_DLL

	//#include "in_python.h" // FIXME!!!
	#include "game.h"
	#include "in_player.h"
	#include "env_sound.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Python
//extern void PythonInitHandles();
//extern void PythonShutdownHandles();

//=========================================================
// Guardado y definición de datos
//=========================================================

//REGISTER_GAMERULES_CLASS( CInMPGameRules );

//=========================================================
// Definición de variables para la configuración.
//=========================================================

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// Configuración
//=========================================================

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CInMPGameRules::CInMPGameRules()
{
	// Ejecutamos el archivo de configuración para servidores dedicados.
	if ( engine->IsDedicatedServer() )
	{
		const char *cfgFile = servercfgfile.GetString();

		if ( cfgFile && cfgFile[0] )
		{
			char szCommand[256];

			Msg("Ejecutando el archivo de configuracion del servidor.\n");

			Q_snprintf(szCommand, sizeof(szCommand), "exec %s\n", cfgFile);
			engine->ServerCommand(szCommand);
		}
	}
}

//=========================================================
// Destructor
//=========================================================
CInMPGameRules::~CInMPGameRules()
{
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CInMPGameRules::Think()
{
	// @TODO: ¿Algo más por hacer? Si no eliminar.
	CGameRules::Think();
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInMPGameRules::GetGameDescription()
{
	return "Apocalypse Multiplayer";
}

//=========================================================
// Crea al jugador.
//=========================================================
void CInMPGameRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	// Le damos el traje de protección.
	pPlayer->EquipSuit();

	// Spawn!
	BaseClass::PlayerSpawn(pPlayer);
}

//=========================================================
// Cliente desconectado.
//=========================================================
void CInMPGameRules::ClientDisconnected(edict_t *pClient)
{
	// Verificamos que aún exista.
	if ( !pClient )
		return;

	CIN_Player *pPlayer = (CIN_Player *)CBaseEntity::Instance(pClient);

	if ( !pPlayer )
		return;

	FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

	// Le quitamos todo.
	pPlayer->RemoveAllItems(true);
	pPlayer->DestroyViewModels();
	pPlayer->SetConnected(PlayerDisconnected);
}

//=========================================================
// Obtiene el recurso del jugador si el asesino es uno o NULL
//=========================================================
CBasePlayer *CInMPGameRules::GetDeathScorer(CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	if ( !pKiller )
		return NULL;

	if ( pKiller->Classify() == CLASS_PLAYER )
		return (CBasePlayer *)pKiller;

	return NULL;
}

//=========================================================
// Crea un evento de muerte.
//=========================================================
void CInMPGameRules::DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	const char *Killer	= "world";
	int KillerID		= 0;

	// Información de la muerte.
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller	= info.GetAttacker();
	CBasePlayer *pScorer	= GetDeathScorer(pKiller, pInflictor);

	if ( info.GetDamageCustom() )
	{
		Killer = GetDamageCustomString(info);

		if ( pScorer )
			KillerID = pScorer->GetUserID();
	}
	else
	{
		// Lo mato otro jugador.
		if ( pScorer )
		{
			// Obtenemos la ID del jugador.
			KillerID = pScorer->GetUserID();

			//
			if ( pInflictor )
			{
				// El inflictor del daño es el mismo jugador.
				if ( pInflictor == pScorer )
				{
					// Obtenemos el nombre del arma
					if ( pScorer->GetActiveWeapon() )
						Killer = pScorer->GetActiveWeapon()->GetClassname();
				}
				// El inflictor fue un objeto (¿Granada?)
				else
					Killer = STRING(pInflictor->m_iClassname);
			}
		}
		else
			Killer = STRING(pInflictor->m_iClassname);

		if ( strncmp(Killer, "weapon_", 7) == 0 )
			Killer += 7;

		if ( strncmp(Killer, "npc_", 4) == 0 )
			Killer += 4;

		if ( strncmp(Killer, "func_", 5) == 0 )
			Killer += 5;

		// Lo traducimos.
		Killer = NPCName(Killer);
	}

	IGameEvent *Event = gameeventmanager->CreateEvent("player_death");

	if ( Event )
	{
		Event->SetInt("userid",		pVictim->GetUserID());
		Event->SetInt("attacker",	KillerID);
		Event->SetInt("customkill", info.GetDamageCustom());
		Event->SetInt("priority",	7);
		Event->SetString("weapon",	Killer);

		gameeventmanager->FireEvent(Event);
	}
}

//=========================================================
// Un jugador ha muerto.
//=========================================================
void CInMPGameRules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	//CIN_Player *pInPlayer = GetInPlayer(pVictim);
	DeathNotice(pVictim, info);

	// Find the killer & the scorer
	//CBaseEntity *pInflictor = info.GetInflictor();
	//CBaseEntity *pKiller	= info.GetAttacker();

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
	BaseClass::PlayerKilled(pVictim, info);
}

//=========================================================
// Obtiene un nombre más elegante para un NPC.
//=========================================================
const char *CInMPGameRules::NPCName(const char *pClass)
{
	/*
	if ( pClass == "zombie" )
		return "Zombi";

	if ( pClass == "fastzombie" )
		return "Zombi rápido";

	if ( pClass == "poisonzombie" )
		return "Zombi venenoso";

	if ( pClass == "grunt" )
		return "Super soldado Grunt";

	if ( pClass == "combine_s" )
		return "Soldado Combine";
		*/

	return pClass;
}

#endif