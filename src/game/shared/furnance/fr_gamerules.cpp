//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego:
// Define la configuraci�n de las amistades, reaparici�n, objetos, etc...
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "fr_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definici�n de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CFrGameRules );

//=========================================================
// Configuraci�n
//=========================================================

// Nombre del juego.
#define GAME_DESCRIPTION	"Furnance";

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CFrGameRules::CFrGameRules()
{
	
}

//=========================================================
// Destructor
//=========================================================
CFrGameRules::~CFrGameRules()
{
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CFrGameRules::GetGameDescription()
{
	return GAME_DESCRIPTION;
}

//=========================================================
// Define las amistades iniciales de las clases.
//=========================================================
void CFrGameRules::InitDefaultAIRelationships()
{
	BaseClass::InitDefaultAIRelationships();
}

//=========================================================
// Devuelve en texto la clase de este NPC.
//=========================================================
const char* CFrGameRules::AIClassText(int classType)
{
	switch ( classType )
	{
		default:	return BaseClass::AIClassText(classType);
	}
}

//=========================================================
// Crea al jugador.
//=========================================================
void CFrGameRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	// No es Half-Life 2 pero necesitamos el traje de protecci�n para mostrar todo.
	pPlayer->EquipSuit();

	BaseClass::PlayerSpawn(pPlayer);
}

#endif