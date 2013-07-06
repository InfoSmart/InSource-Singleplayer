//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego:
// Define la configuración de las amistades, reaparición, objetos, etc...
//
// Coop: scpcoop_gamerules
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "scp_gamerules.h"

#ifndef CLIENT_DLL
	#include "director.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CSCPGameRules );

//=========================================================
// Definición de comandos de consola.
//=========================================================

//=========================================================
// Configuración
//=========================================================

// Nombre del juego.
#define GAME_DESCRIPTION	"SCP: Source";

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CSCPGameRules::CSCPGameRules()
{
	
}

//=========================================================
// Destructor
//=========================================================
CSCPGameRules::~CSCPGameRules()
{
	
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CSCPGameRules::GetGameDescription()
{
	return GAME_DESCRIPTION;
}

//=========================================================
// Verifica si los grupos pueden tener colisiones.
//=========================================================
bool CSCPGameRules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

//=========================================================
// Define las amistades iniciales de las clases.
//=========================================================
void CSCPGameRules::InitDefaultAIRelationships()
{
	BaseClass::InitDefaultAIRelationships();

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCP_173,			CLASS_NONE,				D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCP_173,			CLASS_PLAYER,			D_HT, 0);	

		
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

//=========================================================
// Devuelve en texto la clase de este NPC.
//=========================================================
const char* CSCPGameRules::AIClassText(int classType)
{
	switch ( classType )
	{
		default:					return BaseClass::AIClassText(classType);
	}
}

//=========================================================
// Crea al jugador.
//=========================================================
void CSCPGameRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	BaseClass::PlayerSpawn(pPlayer);
}

#endif // CLIENT_DLL