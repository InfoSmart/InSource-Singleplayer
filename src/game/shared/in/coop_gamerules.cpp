//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego:
// Coop: 
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "coop_gamerules.h"

#ifndef CLIENT_DLL
	#include "in_player.h"
	#include "env_sound.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CInCoopGameRules );

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CInCoopGameRules::CInCoopGameRules()
{
}

//=========================================================
// Destructor
//=========================================================
CInCoopGameRules::~CInCoopGameRules()
{
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CInCoopGameRules::Think()
{
	BaseClass::Think();
}

//=========================================================
// Guardamos objetos necesarios en caché.
//=========================================================
void CInCoopGameRules::Precache()
{
	
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInCoopGameRules::GetGameDescription()
{
	return "Apocalypse COOP";
}

//=========================================================
// Ciente conectado
//=========================================================
bool CInCoopGameRules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return true;
}

//=========================================================
// Un jugador ha muerto.
//=========================================================
void CInCoopGameRules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	BaseClass::PlayerKilled(pVictim, info);
	//CIN_Player *pInPlayer = GetInPlayer(pVictim);

	
}

#endif