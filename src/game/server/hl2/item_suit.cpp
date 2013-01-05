//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== item_suit.cpp ========================================================

  handling for the player's suit.
*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
public:
	DECLARE_CLASS( CItemSuit, CItem );

	void Spawn( void )
	{ 
		Precache();
		SetModel("models/items/hevsuit.mdl");
		BaseClass::Spawn();
		
		CollisionProp()->UseTriggerBounds(false, 0);
	}

	void Precache( void )
	{
		PrecacheModel ("models/items/hevsuit.mdl");
	}

	bool MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->IsSuitEquipped())
			return true;

		if (m_spawnflags & SF_SUIT_SHORTLOGON)
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A0");
		else
		{
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_AAx");
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A3");
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A4");
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A6");
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A7");
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A10");
		}

		pPlayer->EquipSuit();				
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);