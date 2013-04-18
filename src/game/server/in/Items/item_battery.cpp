//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Handling for the suit batteries.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "engine/IEngineSound.h"
#include "in_player.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// CItemBattery
//=========================================================
class CItemBattery : public CItem
{
public:
	DECLARE_CLASS(CItemBattery, CItem);

	void Spawn();
	void Precache();
	bool MyTouch(CBasePlayer *pPlayer);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);

//=========================================================
// Configuración
//=========================================================

#define MODEL "models/items/battery.mdl"

//=========================================================
// Crea el objeto.
//=========================================================
void CItemBattery::Spawn()
{
	Precache();
	SetModel(MODEL);

	BaseClass::Spawn();
}

//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CItemBattery::Precache()
{
	PrecacheModel(MODEL);
	PrecacheScriptSound("ItemBattery.Touch");
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CItemBattery::MyTouch(CBasePlayer *pPlayer)
{
	// En modo Survival esto solo se puede usar desde el inventario.
	if ( g_pGameRules->IsMultiplayer() )
		return false;

	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>(pPlayer);
	return ( pHL2Player && pHL2Player->ApplyBattery() );
}

//=========================================================
// Se activa cuando el usuario usa (+USE) el objeto.
//=========================================================
void CItemBattery::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
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