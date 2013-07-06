//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Lata de comida.
// Todos tenemos hambre.
//
// InfoSmart 2013. Todos los derechos reservados.
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "in_player.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// CSoda
//=========================================================
class CFood : public CItem
{
public:
	DECLARE_CLASS(CFood, CItem);

	void Spawn();
	void Precache();

	bool MyTouch(CBasePlayer *pPlayer);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(item_food, CFood);
PRECACHE_REGISTER(item_food);

//=========================================================
// Configuración
//=========================================================

ConVar sk_food("sk_food", "0", FCVAR_REPLICATED,	"¿Cuanta hambre quitaran las latas de comida?");

#define MODEL "models/props_junk/garbage_metalcan001a.mdl" // @TODO: Crear/encontrar uno mejor.

//=========================================================
// Crea el objeto.
//=========================================================
void CFood::Spawn()
{
	Precache();
	SetModel(MODEL);

	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CFood::Precache()
{
	PrecacheModel(MODEL);
	PrecacheScriptSound("Player.Eat");
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CFood::MyTouch(CBasePlayer *pPlayer)
{
	// En modo Survival esto solo se puede usar desde el inventario.
	if ( g_pGameRules->IsMultiplayer() )
		return false;

	CIN_Player *pInPlayer = GetInPlayer(pPlayer);

	if ( !pPlayer )
		return false;

	if ( pInPlayer->TakeHunger(sk_food.GetFloat()) != 0 )
	{
		CSingleUserRecipientFilter user(pInPlayer);
		user.MakeReliable();

		UserMessageBegin(user, "ItemPickup");
			WRITE_STRING(GetClassname());
		MessageEnd();

		CPASAttenuationFilter filter(pInPlayer, "Player.Eat");
		EmitSound(filter, entindex(), "Player.Eat");

		if ( g_pGameRules->ItemShouldRespawn(this) )
			Respawn();
		else
			Remove();

		return true;
	}

	return false;
}

//=========================================================
// Se activa cuando el usuario usa (+USE) el objeto.
//=========================================================
void CFood::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// En modo Historia esto hace lo que tiene que hacer.
	if ( !g_pGameRules->IsMultiplayer() )
	{
		BaseClass::Use(pActivator, pCaller, useType, value);
		return;
	}

   CIN_Player *pPlayer = (CIN_Player *)pActivator;

	if ( !pPlayer )
		return;

	// Lo agregamos al inventario.
	int slot = pPlayer->Inventory_AddItem(GetClassname());

	if ( slot != 0 )
		Remove();
}


//=========================================================
//=========================================================
// Lata de comida vacia.
// Una lata que no hace absolutamente nada. (Decoración)
//=========================================================
//=========================================================
class CEmptyFood : public CFood
{
public:
	DECLARE_CLASS(CEmptyFood, CFood);
	bool MyTouch(CBasePlayer *pPlayer);
};

LINK_ENTITY_TO_CLASS(item_empty_food, CEmptyFood);
PRECACHE_REGISTER(item_empty_food);

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CEmptyFood::MyTouch(CBasePlayer *pPlayer)
{
	return false;
}