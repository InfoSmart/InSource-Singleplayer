//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego
//
// Define la configuración de amistades, reaparición, objetos, etc...
//
// Con el fin de evitar hacer archivos innecesarios, las reglas de Apocalypse
// fueron puestas aquí, si hará un MOD distinto solo cambielas.
//
//=====================================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "singleplay_gamerules.h"

#include "hl2_gamerules.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CInSource );

//=========================================================
// Definición de variables para la configuración.
//=========================================================

ConVar	sk_plr_dmg_flare_round	("sk_plr_dmg_flare_round","0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_flare_round	("sk_npc_dmg_flare_round","0", FCVAR_REPLICATED);
ConVar	sk_max_flare_round		("sk_max_flare_round","0", FCVAR_REPLICATED);

ConVar	sk_max_slam				("sk_max_slam","0", FCVAR_REPLICATED);
ConVar	sk_max_tripwire			("sk_max_tripwire","0", FCVAR_REPLICATED);

ConVar	sk_plr_dmg_molotov		("sk_plr_dmg_molotov","0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_molotov		("sk_npc_dmg_molotov","0", FCVAR_REPLICATED);
ConVar	sk_max_molotov			("sk_max_molotov","0", FCVAR_REPLICATED);

//=========================================================
// Configuración
//=========================================================

// Nombre de la aplicación.
#define GAME_DESCRIPTION	"Apocalypse";

#ifdef CLIENT_DLL
#else

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInSource::GetGameDescription()
{
	return GAME_DESCRIPTION;
}

//=========================================================
// Jugador creado
//=========================================================
void CInSource::PlayerSpawn(CBasePlayer *pPlayer)
{
	
}

//=========================================================
// Define las amistades iniciales de las clases.
//=========================================================
void CInSource::InitDefaultAIRelationships()
{
	BaseClass::InitDefaultAIRelationships();

	// ------------------------------------------------------------
	//	> CLASS_ANTLION
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_GRUNT,			D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BARNACLE
	//
	//  In this case, the relationship D_HT indicates which characters
	//  the barnacle will try to eat.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,	CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,	CLASS_GRUNT,			D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,	CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,	CLASS_GRUNT,			D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSQUID
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_HEADCRAB,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_HOUNDEYE,			D_FR, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_HACKED_ROLLERMINE,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,	CLASS_GRUNT,			D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_REBEL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_GUNSHIP
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_GRUNT,		D_FR, 0);
	
	// ------------------------------------------------------------
	//	> CLASS_COMBINE_HUNTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,	CLASS_GRUNT,			D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,	CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,	CLASS_GRUNT,			D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,	CLASS_BULLSQUID,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,	CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_HOUNDEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_HEADCRAB,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_HACKED_ROLLERMINE,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,	CLASS_GRUNT,			D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MANHACK
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_HOUNDEYE,		D_HT, -1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_METROPOLICE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,	CLASS_GRUNT,		D_FR, 0);
	
	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,	CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,	CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,	CLASS_GRUNT,			D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_SCANNER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,	CLASS_BULLSQUID,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,	CLASS_HOUNDEYE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,	CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_STALKER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_HOUNDEYE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_GRUNT,		D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,	CLASS_BULLSQUID,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,	CLASS_HOUNDEYE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,	CLASS_GRUNT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PROTOSNIPER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_BULLSQUID,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_HOUNDEYE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_GRUNT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_HACKED_ROLLERMINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,	CLASS_BULLSQUID,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,	CLASS_HOUNDEYE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,	CLASS_GRUNT,		D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_GRUNT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_GRUNT,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_HEADCRAB,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_MANHACK,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_METROPOLICE,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_MILITARY,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_SCANNER,			D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GRUNT,	CLASS_HACKED_ROLLERMINE,D_LI, 0);
		
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_BASE,				D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_HEADCRAB,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BASE,			CLASS_GRUNT,			D_HT, 0);
	*/
}

const char* CInSource::AIClassText(int classType)
{
	switch ( classType )
	{
		case CLASS_GRUNT:			return "CLASS_GRUNT";
		case CLASS_BULLSQUID:		return "CLASS_BULLSQUID";
		case CLASS_HOUNDEYE:		return "CLASS_HOUNDEYE";
		default:					return BaseClass::AIClassText(classType);
	}
}

#endif