//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Loot
// Como en DayZ. Una caja con distintos objetos y armas para "lootear"
//
// InfoSmart. Todos los derechos reservados.
//=============================================================================//

#include "cbase.h"
#include "item_loot.h"

#include "in_gamerules.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"

#include "in_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS(item_loot, CLoot);
PRECACHE_REGISTER(item_loot);

BEGIN_DATADESC( CLoot )
	
END_DATADESC();

//=========================================================
// Configuración
//=========================================================

ConVar sv_max_loot			("sv_max_loot",				"10", FCVAR_REPLICATED, "");
ConVar sv_loot_fill_min_time("sv_loot_fill_min_time",	"10", FCVAR_REPLICATED, "");
ConVar sv_loot_fill_max_time("sv_loot_fill_max_time",	"30", FCVAR_REPLICATED, "");

#define MODEL "models/props_junk/cardboard_box003a_gib01.mdl" // Una caja comun sin fisicas.

//=========================================================
// Constructor
//=========================================================
CLoot::CLoot()
{
	// Poner objetos ahora.
	NextFill		= gpGlobals->curtime;
	// Seleccionamos una cantidad máxima al azar.
	MaxLootItems	= floor( (sv_max_loot.GetInt() / random->RandomInt(1, 4)) + 0.5);

	SetNextThink(gpGlobals->curtime);
}

//=========================================================
// Crea el objeto.
//=========================================================
void CLoot::Spawn()
{
	Precache();
	SetModel(MODEL);

	SetSolid(SOLID_BBOX);
	SetBlocksLOS( false );
	
	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CLoot::Precache()
{
	PrecacheModel(MODEL);
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CLoot::Think()
{
	BaseClass::Think();

	// ¡Es hora de colocar nuevos objetos!
	if ( NextFill <= gpGlobals->curtime )
		Fill();

	SetNextThink(gpGlobals->curtime + 1);
}

//=========================================================
// Agrega nuevos objetos al loot.
//=========================================================
void CLoot::Fill()
{
	// Este loot ya esta lleno.
	if ( GetItemCount() >= MaxLootItems )
		return;

	// ¿Cuantos objetos nuevos le pondremos?
	int NewItemsCount = random->RandomInt(1, 3);

	for ( int i = 0; i < NewItemsCount; i++ )
	{
		// Seleccionamos un objeto al azar.
		const char *pItem = SelectRandomItem();
		// Agregamos el objeto al Loot.
		AddItem(pItem);
	}

	// La siguiente vez que agregaremos nuevos objetos.
	NextFill = gpGlobals->curtime + random->RandomInt(sv_loot_fill_min_time.GetInt(), sv_loot_fill_max_time.GetInt());
}

//=========================================================
// Obtiene el número de objetos en el loot.
//=========================================================
int CLoot::GetItemCount()
{
	int pTotal = 0;

	for ( int i = 1; i < LootItems.Count(); i++ )
	{
		// No hay nada en este Slot
		if ( LootItems[i] == 0 || !LootItems[i] )
			continue;

		pTotal++;
	}

	return pTotal;
}

bool CLoot::HasItem(int pEntity)
{
	// Verificamos cada slot del inventario.
	for ( int i = 1; i < LootItems.Count(); i++ )
	{
		// Este slot contiene exactamente la ID del objeto.
		if ( LootItems[i] == pEntity )
			return true;
	}

	return false;
}

bool CLoot::HasItemByName(const char *pName)
{
	int pEntity = g_pInGameRules->Inventory_GetItemID(pName);
	return HasItem(pEntity);
}

//=========================================================
// Agrega un nuevo objeto.
//=========================================================
void CLoot::AddItem(const char *pName)
{
	// Este loot ya esta lleno.
	if ( GetItemCount() >= MaxLootItems )
		return;

	// Obtenemos la ID de este objeto.
	int pEntity = g_pInGameRules->Inventory_GetItemID(pName);

	// ¡Este objeto no esta registrado!
	if ( pEntity == 0 )
		return;

	for ( int i = 1; i < LootItems.Count(); i++ )
	{
		// Este slot no tiene ningún objeto.
		if ( LootItems[i] == 0 || !LootItems[i] )
		{
			LootItems.Set(i, pEntity);
			break;
		}
	}
}

//=========================================================
// Elimina un objeto.
//=========================================================
bool CLoot::RemoveItem(int pEntity, CIN_Player *pPlayer)
{
	// Verificamos cada slot del inventario.
	for ( int i = 1; i < LootItems.Count(); i++ )
	{
		// Este slot contiene exactamente la ID del objeto.
		if ( LootItems[i] == pEntity )
		{
			// La establecemos en 0 para removerlo y actualizamos el loot.
			LootItems.Set(i, 0);

			if ( pPlayer )
				SendLoot(pPlayer);
			else
				Msg("[LOOT} pPlayer invalido \r\n");

			return true;
		}
	}

	return false;
}

//=========================================================
// Selecciona un objeto al azar.
//=========================================================
const char *CLoot::SelectRandomItem()
{
	int pRandom			= random->RandomInt(1, 15);
	const char *pResult = "";

	if ( pRandom == 1 && HasSpawnFlags(SF_SPAWN_PISTOL) )
		pResult = "item_ammo_pistol";

	if ( pRandom == 2 && HasSpawnFlags(SF_SPAWN_AR2) )
		pResult = "item_ammo_ar2";

	if ( pRandom == 3 && HasSpawnFlags(SF_SPAWN_SMG1) )
		pResult = "item_ammo_smg1";

	if ( pRandom == 4 && HasSpawnFlags(SF_SPAWN_SHOTGUN) )
		pResult = "item_box_buckshot";

	if ( pRandom == 5 && HasSpawnFlags(SF_SPAWN_357) )
		pResult = "item_ammo_357";

	//if ( pRandom == 6 && HasSpawnFlags(SF_SPAWN_ALYXGUN) )
		//pResult = "weapon_alyxgun";

	if ( pRandom == 7 && HasSpawnFlags(SF_SPAWN_CROSSBOW) )
		pResult = "item_ammo_crossbow";

	if ( pRandom == 8 && HasSpawnFlags(SF_SPAWN_FRAG) )
		pResult = "weapon_frag";

	if ( pRandom == 9 && HasSpawnFlags(SF_SPAWN_HEALTHKIT) )
	{
		if ( random->RandomInt(1, 5) == 2 )
			pResult = "item_healthvial";
		else
			pResult = "item_healthkit";
	}

	if ( pRandom == 10 && HasSpawnFlags(SF_SPAWN_BATTERY) )
		pResult = "item_battery";

	if ( pRandom == 11 && HasSpawnFlags(SF_SPAWN_BLOOD) )
		pResult = "item_bloodkit";

	if ( pRandom == 12 && HasSpawnFlags(SF_SPAWN_BANDAGE) )
		pResult = "item_bandage";

	if ( pRandom == 13 && HasSpawnFlags(SF_SPAWN_SODA) )
		pResult = "item_soda";

	if ( pRandom == 14 && HasSpawnFlags(SF_SPAWN_FOOD) )
		pResult = "item_food";

	// Hacemos otra pasada a la suerte.
	if ( pResult == "" )
		return SelectRandomItem();

	// Es munición.
	if ( Q_stristr(pResult, "ammo") && pResult != "item_ammo_crossbow" )
	{
		// ¡Suerte! Toca una caja grande de munición.
		if ( random->RandomInt(1, 5) > 3 )
			pResult = CFmtStr("%s_large", pResult);
	}

	return pResult;
}

//=========================================================
// Se activa cuando el usuario usa (+USE) el objeto.
//=========================================================
void CLoot::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CIN_Player *pPlayer = (CIN_Player *)pActivator;

	if ( !pPlayer )
		return;

	// Le enviamos los objetos dentro del Loot.
	SendLoot(pPlayer);

	// Abrimos el panel de Loot.
	pPlayer->ExecCommand("openloot");
}

void CLoot::SendLoot(CIN_Player *pPlayer)
{
	// No es un jugador.
	if ( !pPlayer )
		return;

	CSingleUserRecipientFilter user(pPlayer);
	user.MakeReliable();

	// Loot!!
	for ( int i = 1; i < LootItems.Count(); i++ )
	{
		// Este slot no tiene ningún objeto.
		if ( LootItems[i] == 0 || !LootItems[i] )
			continue;

		// Le enviamos la lista de objetos en este Loot.
		UserMessageBegin(user, "Loot");
			WRITE_SHORT(entindex());	// Se lo enviamos en cada objeto y nos evitamos hacer otro UserMessage
			WRITE_SHORT(i);				// ID en el array.
			WRITE_SHORT(LootItems[i]);	// ID del objeto.
		MessageEnd();
	}

	// Actualizamos el panel de Loot.
	pPlayer->ExecCommand("cl_update_loot 1");
}