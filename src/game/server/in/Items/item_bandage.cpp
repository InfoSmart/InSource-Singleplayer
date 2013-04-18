//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Benda.
// Objeto para dejar de sangrar.
//
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "in_player.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// CBandage
//=========================================================
class CBandage : public CItem
{
public:
	DECLARE_CLASS(CBandage, CItem);

	void Spawn();
	void Precache();

	bool MyTouch(CBasePlayer *pPlayer);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(item_bandage, CBandage);
PRECACHE_REGISTER(item_bandage);

//=========================================================
// Configuración
//=========================================================

#define MODEL "models/props_junk/cardboardbox01b.mdl" // @TODO: Crear/encontrar uno mejor.

//=========================================================
// Crea el objeto.
//=========================================================
void CBandage::Spawn()
{
	Precache();
	SetModel(MODEL);

	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CBandage::Precache()
{
	PrecacheModel(MODEL);
	PrecacheScriptSound("Player.Bandage");
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CBandage::MyTouch(CBasePlayer *pPlayer)
{
	// En modo Survival esto solo se puede usar desde el inventario.
	if ( g_pGameRules->IsMultiplayer() )
		return false;

	CIN_Player *pInPlayer = GetInPlayer(pPlayer);

	if ( !pPlayer )
		return false;

	if ( pInPlayer->ScarredBloodWound() )
	{
		CSingleUserRecipientFilter user(pInPlayer);
		user.MakeReliable();

		UserMessageBegin(user, "ItemPickup");
		WRITE_STRING(GetClassname());
		MessageEnd();

		CPASAttenuationFilter filter(pInPlayer, "Player.Bandage");
		EmitSound(filter, entindex(), "Player.Bandage");

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
void CBandage::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
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