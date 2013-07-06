//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Soda.
// Por obvias razones, un objeto para aliviar la sed.
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
class CSoda : public CItem
{
public:
	DECLARE_CLASS(CSoda, CItem);

	void Spawn();
	void Precache();

	bool MyTouch(CBasePlayer *pPlayer);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(item_soda, CSoda);
PRECACHE_REGISTER(item_soda);

//=========================================================
// Configuración
//=========================================================

ConVar sk_soda("sk_soda", "0", FCVAR_REPLICATED,	"¿Cuanta sed quitaran las latas de soda?");

#define MODEL "models/props_junk/popcan01a.mdl" // @TODO: Crear/encontrar uno mejor.

//=========================================================
// Crea el objeto.
//=========================================================
void CSoda::Spawn()
{
	Precache();
	SetModel(MODEL);

	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CSoda::Precache()
{
	PrecacheModel(MODEL);
	PrecacheScriptSound("Player.Drink");
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CSoda::MyTouch(CBasePlayer *pPlayer)
{
	// En modo Survival esto solo se puede usar desde el inventario.
	if ( g_pGameRules->IsMultiplayer() )
		return false;

	CIN_Player *pInPlayer = GetInPlayer(pPlayer);

	if ( !pPlayer )
		return false;

	if ( pInPlayer->TakeThirst(sk_soda.GetFloat()) != 0 )
	{
		CSingleUserRecipientFilter user(pInPlayer);
		user.MakeReliable();

		UserMessageBegin(user, "ItemPickup");
			WRITE_STRING(GetClassname());
		MessageEnd();

		CPASAttenuationFilter filter(pInPlayer, "Player.Drink");
		EmitSound(filter, entindex(), "Player.Drink");

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
void CSoda::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
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
// Lata de soda vacia.
// Una lata que no hace absolutamente nada. (Decoración)
//=========================================================
//=========================================================
class CEmptySoda : public CSoda
{
public:
	DECLARE_CLASS(CEmptySoda, CSoda);
	bool MyTouch(CBasePlayer *pPlayer);
};

LINK_ENTITY_TO_CLASS(item_empty_soda, CEmptySoda);
PRECACHE_REGISTER(item_empty_soda);

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CEmptySoda::MyTouch(CBasePlayer *pPlayer)
{
	return false;
}