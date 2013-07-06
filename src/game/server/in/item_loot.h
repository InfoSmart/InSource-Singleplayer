#ifndef ITEM_LOOT_H

#define ITEM_LOOT_H
#pragma once

#include "in_player.h"

//=========================================================
// Flags
//=========================================================

#define SF_SPAWN_PISTOL			4
#define SF_SPAWN_AR2			8
#define SF_SPAWN_SMG1			16
#define SF_SPAWN_SHOTGUN		32
#define SF_SPAWN_357			64
#define SF_SPAWN_ALYXGUN		128
#define SF_SPAWN_CROSSBOW		256
#define SF_SPAWN_FRAG			512
#define SF_SPAWN_HEALTHKIT		1024
#define SF_SPAWN_BATTERY		2048
#define SF_SPAWN_BLOOD			4096
#define SF_SPAWN_BANDAGE		8192
#define SF_SPAWN_SODA			16384
#define SF_SPAWN_FOOD			32768

//=========================================================
// CLoot
//=========================================================
class CLoot : public CBaseAnimating
{
public:
	DECLARE_CLASS(CLoot, CBaseAnimating);
	DECLARE_DATADESC();

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

	CLoot();

	virtual void Spawn();
	void Precache();

	void Think();
	void Fill();

	int GetItemCount();

	bool HasItem(int pEntity);
	bool HasItemByName(const char *pName);
	void AddItem(const char *pName);
	bool RemoveItem(int pEntity, CIN_Player *pPlayer);

	const char *SelectRandomItem();

	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SendLoot(CIN_Player *pPlayer);

	CNetworkArray( int, LootItems, 100 );

private:
	float NextFill;
	int MaxLootItems;
	bool OnlyAmmo;
};

#endif //ITEM_LOOT_H