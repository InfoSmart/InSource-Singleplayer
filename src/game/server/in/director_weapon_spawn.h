#ifndef DIRECTOR_WEAPON_SPAWN_H

#define DIRECTOR_WEAPON_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

#define SF_SPAWN_CROWBAR		4
#define SF_SPAWN_PISTOL			8
#define SF_SPAWN_RIFLE			16
#define SF_SPAWN_SMG1			32
#define SF_SPAWN_SHOTGUN		64
#define SF_SPAWN_357			128
#define SF_SPAWN_ALYXGUN		256
#define SF_SPAWN_CROSSBOW		512
#define SF_SPAWN_FRAG			1024
#define SF_SPAWN_HEALTHKIT		2048
#define SF_SPAWN_BATTERY		4096
#define SF_SPAWN_BLOOD			8192
#define SF_SPAWN_BANDAGE		16384

// Iván: Solo como referencia, no sirve de nada...
static const char *Weapons[] =
{
	"weapon_crowbar",
	"weapon_pistol",
	"weapon_ar2",
	"weapon_smg1",
	"weapon_shotgun",
	"weapon_357",
	"weapon_alyxgun",
	"weapon_crossbow",
	"weapon_frag"
};
static const char *WeaponsAmmo[] =
{
	"",
	"item_ammo_pistol",
	"item_ammo_ar2",
	"item_ammo_smg1",
	"item_box_buckshot",
	"item_ammo_357",
	"",
	"item_ammo_crossbow",
	"weapon_frag"
};

class CDirectorWeaponSpawn : public CDirectorBaseSpawn
{
public:
	DECLARE_CLASS(CDirectorWeaponSpawn, CDirectorBaseSpawn);
	DECLARE_DATADESC();

	void Precache();

	void Make();
	void MakeWeapon(const char *pWeaponClass);
	void MakeAmmo(const char *pWeaponClass);

	const char *SelectWeapon();
	const char *SelectAmmo(const char *pWeaponClass);

private:
		bool OnlyAmmo;
		bool SpawnCrowbar;
		bool SpawnPistol;
		bool SpawnRifle;
		bool SpawnSMG1;
};

class CWeaponSpawn : public CDirectorBaseSpawn
{
public:
	DECLARE_CLASS(CWeaponSpawn, CDirectorBaseSpawn);
	DECLARE_DATADESC();

	CWeaponSpawn();

	void Think();
	void Precache();

	bool DetectTouch();
	void Make();
	void MakeWeapon(const char *pWeaponClass);
	void MakeAmmo(const char *pWeaponClass);
	void MakeItem(const char *pItemClass);

	const char *SelectWeapon();
	const char *SelectAmmo(const char *pWeaponClass);

private:
	CBaseEntity *pLastWeaponSpawned;
	int UniqueKey;
	bool OnlyAmmo;
};

#endif // DIRECTOR_WEAPON_SPAWN_H