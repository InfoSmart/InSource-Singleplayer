//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Reglas del juego:
// Survival: Empiezas con una sola palanca, visita edificios para encontrar armas, medicamentos y
// recursos para sobrevivir, evita a los demás sobrevivientes y unete a tus amigos para
// matar zombis o a los demás. El intento de hacer un DayZ: Source
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "insurvival_gamerules.h"

#ifndef CLIENT_DLL
	#include "in_player.h"
	#include "env_sound.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

REGISTER_GAMERULES_CLASS( CInSurvivalGameRules );

#ifndef CLIENT_DLL

//=========================================================
// Constructor
//=========================================================
CInSurvivalGameRules::CInSurvivalGameRules()
{
	// Reproducimos una música de ambiente en periodos de 50 a 150 segundos.
	NextAmbientMusic	= gpGlobals->curtime + (random->RandomInt(20, 80));
	NextCleanFlies		= gpGlobals->curtime + 200;
}

//=========================================================
// Destructor
//=========================================================
CInSurvivalGameRules::~CInSurvivalGameRules()
{
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CInSurvivalGameRules::Think()
{
	BaseClass::Think();

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
//=========================================================
void CInSurvivalGameRules::FliesThink()
{
	// Aún no toca eliminar los sonidos.
	if ( gpGlobals->curtime < NextCleanFlies )
		return;

	CEnvSound *pSound = NULL;

	do
	{
		pSound = (CEnvSound *)gEntList.FindEntityByName(pSound, "survivor_dead_flies");

		if ( !pSound )
			continue;

		pSound->Stop();

	} while(pSound);

	NextCleanFlies = gpGlobals->curtime + 200;
}

//=========================================================
// Guardamos objetos necesarios en caché.
//=========================================================
void CInSurvivalGameRules::Precache()
{
	CBaseEntity::PrecacheScriptSound("Ambient.SurvivorTerror");
	CBaseEntity::PrecacheScriptSound("Ambient.DeathFlies2");
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInSurvivalGameRules::GetGameDescription()
{
	return "Apocalypse Supervivencia";
}

//=========================================================
// Ciente conectado
//=========================================================
bool CInSurvivalGameRules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return true;
}

//=========================================================
// Un jugador ha muerto.
//=========================================================
void CInSurvivalGameRules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	BaseClass::PlayerKilled(pVictim, info);
	CIN_Player *pInPlayer = GetInPlayer(pVictim);

	// @TODO: ¿En vez de soltar todas las cosas, permitir ver el inventario externo?
	pInPlayer->Inventory_DropAll();

	// Creamos efecto de moscas.
	CEnvSound *pFlies = (CEnvSound *)CreateEntityByName("env_sound");
	pFlies->SetSoundName(MAKE_STRING("Ambient.DeathFlies2"));
	pFlies->SetRadius(900);
	pFlies->SetName(MAKE_STRING("survivor_dead_flies"));
	pFlies->SetAbsOrigin(pVictim->GetAbsOrigin());
	DispatchSpawn(pFlies);
	pFlies->Activate();
	pFlies->PlayManual(1, 100);
}

#endif