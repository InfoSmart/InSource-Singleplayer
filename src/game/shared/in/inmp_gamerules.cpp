//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego (Modo: Survivor)
//
// Define la configuración de amistades, reaparición, objetos, etc...
//
// Con el fin de evitar hacer archivos innecesarios, las reglas de Apocalypse
// fueron puestas aquí, si hará un MOD distinto solo cambielas.
//
//=====================================================================================//

#include "cbase.h"
#include "inmp_gamerules.h"
#include "basehlcombatweapon.h"

#ifndef CLIENT_DLL

	#include "game.h"
	#include "in_player.h"
	#include "env_sound.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
	@TODO:

	- Moscas en un cuerpo muerto.
	- 
*/

static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

//=========================================================
// Guardado y definición de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CInSourceMPRules );

//=========================================================
// Definición de variables para la configuración.
//=========================================================

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// Configuración
//=========================================================

const CViewVectors* CInSourceMPRules::GetViewVectors() const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CInSourceMPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CInSourceMPRules::CInSourceMPRules()
{
	// Ejecutamos el archivo de configuración para dedicados.
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

	// Reproducimos una música de ambiente en periodos de 50 a 150 segundos.
	NextAmbientMusic = gpGlobals->curtime + (random->RandomInt(20, 80));

	//NextFliesSound = gpGlobals->curtime;
	NextCleanFlies = gpGlobals->curtime + 200;
}	

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CInSourceMPRules::Think()
{
	CGameRules::Think();

	// Hora de reproducir la música.
	if ( NextAmbientMusic <= gpGlobals->curtime )
	{
		UTIL_EmitAmbientSound(0, Vector(0, 0, 0), "Ambient.SurvivorTerror", 0.8f, SNDLVL_NONE, 0, 100);
		NextAmbientMusic = gpGlobals->curtime + (random->RandomInt(20, 80));
	}

	FliesThink();
}

//=========================================================
// Pensar: Reproduce constantemente el sonido de moscas en
// los cadaveres.
// @TODO: Esto es feo... ¿Algo mejor?
// @TODO: Investigar más acerca de los cadaveres.
//=========================================================
void CInSourceMPRules::FliesThink()
{
	// Toca eliminar.
	if ( gpGlobals->curtime < NextCleanFlies )
		return;

	CEnvSound *pSound = NULL;

	do
	{
		pSound = (CEnvSound *)gEntList.FindEntityByName(pSound, "survivor_dead_flies");

		if ( !pSound )
			continue;

		// Toca reproducir.
		/*if ( gpGlobals->curtime >= NextFliesSound )
		{
			pSound->Stop(true);
			pSound->PlayManual(1, 100);
		}*/

		
		//{
			pSound->Stop();
			//UTIL_Remove(pSound);
		//}

	} while(pSound);

	//NextFliesSound = gpGlobals->curtime + 3;

	//if ( gpGlobals->curtime >= NextCleanFlies )
		NextCleanFlies = gpGlobals->curtime + 200;
}

//=========================================================
// Guardamos objetos necesarios en caché.
//=========================================================
void CInSourceMPRules::Precache()
{
	CBaseEntity::PrecacheScriptSound("Ambient.SurvivorTerror");
	//CBaseEntity::PrecacheScriptSound("Ambient.DeathFlies2");
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInSourceMPRules::GetGameDescription()
{
	return "Apocalypse Supervivencia";
}

//=========================================================
// Jugador creado
//=========================================================
void CInSourceMPRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	// Le damos el traje de protección.
	pPlayer->EquipSuit();

	CBaseEntity *pWeaponEntity = (CBaseEntity *)CreateEntityByName("weapon_crowbar");
	pWeaponEntity->Touch(pPlayer);

	BaseClass::PlayerSpawn(pPlayer);
}

//=========================================================
// Ciente conectansoe.
//=========================================================
bool CInSourceMPRules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return true;
}

//=========================================================
// Cliente desconectado.
//=========================================================
void CInSourceMPRules::ClientDisconnected(edict_t *pClient)
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
CBasePlayer *CInSourceMPRules::GetDeathScorer(CBaseEntity *pKiller, CBaseEntity *pInflictor)
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
void CInSourceMPRules::DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info)
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
void CInSourceMPRules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	CIN_Player *pInPlayer = GetInPlayer(pVictim);

	DeathNotice(pVictim, info);
	pInPlayer->Inventory_DropAll(); // @TODO: ¿En vez de soltar todas las cosas, permitir ver el inventario externo?

	// Creamos efecto de moscas.
	CEnvSound *pFlies = (CEnvSound *)CreateEntityByName("env_sound");
	pFlies->SetSoundName(MAKE_STRING("Ambient.DeathFlies2"));
	pFlies->SetRadius(900);
	pFlies->SetName(MAKE_STRING("survivor_dead_flies"));
	pFlies->SetAbsOrigin(pVictim->GetAbsOrigin());
	DispatchSpawn(pFlies);
	pFlies->Activate();
	pFlies->PlayManual(1, 100);

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller	= info.GetAttacker();

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
	BaseClass::PlayerKilled(pVictim, info);
}

//=========================================================
// Obtiene un nombre más elegante para un NPC.
//=========================================================
const char *CInSourceMPRules::NPCName(const char *pClass)
{
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

	return pClass;
}

#endif