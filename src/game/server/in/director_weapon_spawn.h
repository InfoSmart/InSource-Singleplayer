#ifndef DIRECTOR_WEAPON_SPAWN_H

#define DIRECTOR_WEAPON_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

#define SF_SPAWN_CROWBAR		4
#define SF_SPAWN_PISTOL			8
#define SF_SPAWN_AR2			16
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
#define SF_SPAWN_SODA			32768
#define SF_SPAWN_FOOD			65536

class CDirectorWeaponSpawn : public CDirectorBaseSpawn
{
public:
	DECLARE_CLASS(CDirectorWeaponSpawn, CDirectorBaseSpawn);
	DECLARE_DATADESC();

	void Precache();
	void AddWeapon(const char *pWeapon, bool pPrecache = false);

	void Make();
	void MakeWeapon(const char *pWeaponClass);
	void MakeAmmo(const char *pWeaponClass);

	const char *SelectWeapon();
	const char *SelectAmmo(const char *pWeaponClass);

private:
	bool OnlyAmmo;

	int	pWeaponsInList;
	const char *pWeaponsList[50];
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
	bool OnlyAmmo;
};

class CItemSpawn : public CWeaponSpawn
{
public:
	DECLARE_CLASS(CItemSpawn, CWeaponSpawn);
	//DECLARE_DATADESC();

	void Precache();
	void Make();

	const char *SelectItem();
};

#endif // DIRECTOR_WEAPON_SPAWN_H