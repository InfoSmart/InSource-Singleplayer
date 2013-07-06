//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Creador de objetos para el modo Survival.
//
// InfoSmart 2013. Todos los derechos reservados.
//=============================================================================//

#include "cbase.h"
#include "director_base_spawn.h"
#include "director_weapon_spawn.h"

#include "items.h"
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( item_spawn, CItemSpawn );

//=========================================================
// Pensar
//=========================================================
void CItemSpawn::Precache()
{
	if ( HasSpawnFlags(SF_SPAWN_BATTERY) )
		UTIL_PrecacheOther("item_battery");

	if ( HasSpawnFlags(SF_SPAWN_BANDAGE) )
		UTIL_PrecacheOther("item_bandage");

	if ( HasSpawnFlags(SF_SPAWN_SODA) )
		UTIL_PrecacheOther("item_soda");

	if ( HasSpawnFlags(SF_SPAWN_FOOD) )
		UTIL_PrecacheOther("item_food");
}

//=========================================================
//=========================================================
void CItemSpawn::Make()
{
	ConVarRef sv_weapons_respawn("sv_weapons_respawn");

	// Desactivado.
	if ( Disabled || !sv_weapons_respawn.GetBool() )
		return;

	// El objeto que hemos creado anteriormente aún sigue aquí.
	if ( DetectTouch() )
		return;

	// Emm... ¿puso todas las clases en "no crear"? :genius:
	// @TODO: Mejorar código.
	if ( !HasSpawnFlags(SF_SPAWN_BATTERY | SF_SPAWN_BANDAGE | SF_SPAWN_SODA | SF_SPAWN_FOOD))
		return;

	// Seleccionamos una clase de arma para crear.
	const char *pItemClass = SelectItem();
	MakeItem(pItemClass);
}

//=========================================================
// Selecciona una clase de objeto.
//=========================================================
const char *CItemSpawn::SelectItem()
{
	int pRandom = random->RandomInt(1, 5);

	if ( pRandom == 1 && HasSpawnFlags(SF_SPAWN_BATTERY) )
		return "item_battery";

	if ( pRandom == 2 && HasSpawnFlags(SF_SPAWN_BANDAGE) )
		return "item_bandage";

	if ( pRandom == 3 && HasSpawnFlags(SF_SPAWN_SODA) )
		return "item_soda";

	if ( pRandom == 4 && HasSpawnFlags(SF_SPAWN_FOOD) )
		return "item_food";

	return SelectItem();
}