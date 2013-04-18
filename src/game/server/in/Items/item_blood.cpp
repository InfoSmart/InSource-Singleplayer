//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Sangre
// Todos necesitamos litros y litros de sangre para vivir.
//
// Iván Bravo Bravo
// InfoSmart. Todos los derechos reservados.
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
//=========================================================
// Bolsa de sangre.
//=========================================================
//=========================================================
class CBloodKit : public CItem
{
public:
	DECLARE_CLASS(CBloodKit, CItem);

	virtual void Spawn();
	void Precache();

	virtual bool MyTouch(CBasePlayer *pPlayer);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(item_bloodkit, CBloodKit);
PRECACHE_REGISTER(item_bloodkit);

//=========================================================
// Configuración
//=========================================================

ConVar sk_bloodkit			("sk_bloodkit",			"0", FCVAR_REPLICATED,	"¿Cuanta sangre daran las bolsas de sangre?");
ConVar sk_bloodkit_insp		("sk_bloodkit_insp",	"0", 0,					"¿La bolsa de sangre se podrá usar en Singleplayer?"); // @TODO: Incorporar

#define MODEL "models/items/healthkit.mdl"

//=========================================================
// Crea el objeto.
//=========================================================
void CBloodKit::Spawn()
{
	Precache();
	SetModel(MODEL);
	m_nSkin = 1;

	BaseClass::Spawn();
}


//=========================================================
// Guarda los objetos necesarios en caché.
//=========================================================
void CBloodKit::Precache()
{
	PrecacheModel(MODEL);
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CBloodKit::MyTouch(CBasePlayer *pPlayer)
{
	// En modo Survival esto solo se puede usar desde el inventario.
	if ( g_pGameRules->IsMultiplayer() )
		return false;

	CIN_Player *pInPlayer = GetInPlayer(pPlayer);

	if ( !pPlayer )
		return false;

	if ( pInPlayer->TakeBlood(sk_bloodkit.GetFloat()) != 0 )
	{
		CSingleUserRecipientFilter user(pInPlayer);
		user.MakeReliable();

		UserMessageBegin(user, "ItemPickup");
			WRITE_STRING(GetClassname());
		MessageEnd();

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
void CBloodKit::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
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

	// Todo salio correcto, removerlo del mapa.
	if ( slot != 0 )
		Remove();
}


//=========================================================
//=========================================================
// Bolsa de sangre vacia.
// Una bolsa que no hace absolutamente nada. (Decoración)
//=========================================================
//=========================================================
class CEmptyBloodKit : public CBloodKit
{
public:
	DECLARE_CLASS(CEmptyBloodKit, CBloodKit);

	void Spawn();
	bool MyTouch(CBasePlayer *pPlayer);
};

LINK_ENTITY_TO_CLASS(item_empty_bloodkit, CBloodKit);
PRECACHE_REGISTER(item_empty_bloodkit);

//=========================================================
// Crea el objeto.
//=========================================================
void CEmptyBloodKit::Spawn()
{
	BaseClass::Spawn();
	m_nSkin = 2; // @TODO: ¿Algo mejor?
}

//=========================================================
// Se activa cuando el usuario toca el objeto.
//=========================================================
bool CEmptyBloodKit::MyTouch(CBasePlayer *pPlayer)
{
	return false;
}