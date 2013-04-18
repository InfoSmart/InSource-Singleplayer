//=====================================================================================//
//
// Creador de armas
//
// Entidad que trabajara en conjunto con InDirector para la creaci�n de armas seg�n
// el nivel y estres del jugador.
//
// Inspiraci�n: Left 4 Dead 2
//
//=====================================================================================//

#include "cbase.h"
#include "director_base_spawn.h"
#include "director_weapon_spawn.h"

#include "director.h"
#include "items.h"
#include "basehlcombatweapon.h"

#include "tier0/memdbgon.h"

#define DETECT_SPAWN_RADIUS 20

//=========================================================
// Definici�n de variables de configuraci�n.
//=========================================================

ConVar sv_weapons_respawn			("sv_weapons_respawn",				"1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Reaparecer armas.");
ConVar sv_weapons_respawn_min_time	("sv_weapons_respawn_min_time",		"20", FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo minimo en segundos que tardar� en reaparecer las armas.");
ConVar sv_weapons_respawn_max_time	("sv_weapons_respawn_max_time",		"50", FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo m�ximo en segundos que tardar� en reaparecer las armas.");

//=========================================================
//=========================================================
// DIRECTOR_WEAPON_SPAWN
//=========================================================
//=========================================================

//=========================================================
// Guardado y definici�n de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_weapon_spawn, CDirectorWeaponSpawn );

BEGIN_DATADESC( CDirectorWeaponSpawn )

	DEFINE_KEYFIELD( OnlyAmmo, FIELD_BOOLEAN, "OnlyAmmo" )
	
END_DATADESC();

//=========================================================
// Guardar los objetos necesarios en cach�.
//=========================================================
void CDirectorWeaponSpawn::Precache()
{
	if ( m_spawnflags & SF_SPAWN_CROWBAR )
		UTIL_PrecacheOther("weapon_crowbar");

	if ( m_spawnflags & SF_SPAWN_PISTOL )
	{
		UTIL_PrecacheOther("weapon_pistol");
		UTIL_PrecacheOther("item_ammo_pistol");
		UTIL_PrecacheOther("item_ammo_pistol_large");
	}

	if ( m_spawnflags & SF_SPAWN_RIFLE )
	{
		UTIL_PrecacheOther("weapon_ar2");
		UTIL_PrecacheOther("item_ammo_ar2");
		UTIL_PrecacheOther("item_ammo_ar2_large");
	}

	if ( m_spawnflags & SF_SPAWN_SMG1 )
	{
		UTIL_PrecacheOther("weapon_smg1");
		UTIL_PrecacheOther("item_ammo_smg1");
		UTIL_PrecacheOther("item_ammo_smg1_large");
	}

	if ( m_spawnflags & SF_SPAWN_SHOTGUN )
	{
		UTIL_PrecacheOther("weapon_shotgun");
		UTIL_PrecacheOther("item_box_buckshot");
	}

	if ( m_spawnflags & SF_SPAWN_357 )
	{
		UTIL_PrecacheOther("weapon_357");
		UTIL_PrecacheOther("item_ammo_357");
		UTIL_PrecacheOther("item_ammo_357_large");
	}

	if ( m_spawnflags & SF_SPAWN_ALYXGUN )
		UTIL_PrecacheOther("weapon_alyxgun");

	if ( m_spawnflags & SF_SPAWN_CROSSBOW )
	{
		UTIL_PrecacheOther("weapon_crossbow");
		UTIL_PrecacheOther("item_ammo_crossbow");
	}

	if ( m_spawnflags & SF_SPAWN_FRAG )
		UTIL_PrecacheOther("weapon_frag");

	BaseClass::Precache();
}

//=========================================================
//=========================================================
void CDirectorWeaponSpawn::Make()
{
	// Esta entidad por ahora no funciona en Multiplayer.
	if ( GameRules()->IsMultiplayer() )
		return;

	// Desactivado.
	if ( Disabled )
		return;

	// Emm... �puso todas las clases en "no crear"? :genius:
	// @TODO: Mejorar c�digo.
	if ( !HasSpawnFlags(SF_SPAWN_CROWBAR | SF_SPAWN_PISTOL | SF_SPAWN_RIFLE | SF_SPAWN_SMG1 | SF_SPAWN_SHOTGUN | SF_SPAWN_357 | SF_SPAWN_ALYXGUN | SF_SPAWN_CROSSBOW | SF_SPAWN_FRAG))
		return;

	// Seleccionamos una clase de arma para crear.
	const char *pWeaponClass	= SelectWeapon();
	CBasePlayer *pPlayer		= UTIL_GetLocalPlayer();

	// Al parecer el jugador ya tiene esta arma, dejarle munici�n.
	if ( pPlayer->Weapon_OwnsThisType(pWeaponClass) || OnlyAmmo )
		MakeAmmo(pWeaponClass);

	// Creamos el arma.
	else
	{
		// �Granadas!
		if ( pWeaponClass == "weapon_frag" )
		{
			int i;

			// Creamos de 2 a 4 granadas.
			// @TODO: La cantidad depende del nivel de estres.
			for ( i = 0; i <= random->RandomInt(1, 3); i = i + 1 )
				MakeWeapon("weapon_frag");

			return;
		}

		MakeWeapon(pWeaponClass);
	}
}

//=========================================================
//=========================================================
void CDirectorWeaponSpawn::MakeWeapon(const char *pWeaponClass)
{
	// Desactivado.
	if ( Disabled )
		return;

	CBaseHLCombatWeapon *pWeapon = (CBaseHLCombatWeapon *)CreateEntityByName(pWeaponClass);

	if ( !pWeapon )
	{
		DevWarning("[DIRECTOR WEAPON SPAWN] Ha ocurrido un problema al crear el arma.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pWeapon, &origin) )
		return;

	QAngle angles	= GetAbsAngles();

	// Lugar de creaci�n.
	pWeapon->SetAbsOrigin(origin);

	// Nombre de la entidad.
	// Iv�n: �NO CAMBIAR SIN AVISAR! Es utilizado por otras entidades para referirse a las armas creadas por el director.
	pWeapon->SetName(MAKE_STRING("director_weapon"));
	pWeapon->SetAbsAngles(angles);

	// Creamos el arma, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pWeapon);
	pWeapon->SetOwnerEntity(this);
	pWeapon->Activate();
}

//=========================================================
//=========================================================
void CDirectorWeaponSpawn::MakeAmmo(const char *pWeaponClass)
{
	// Desactivado.
	if ( Disabled )
		return;

	// Obtenemos el tipo de munici�n para esta arma.
	const char *pWeaponAmmo = SelectAmmo(pWeaponClass);

	// Esta arma no tiene munici�n.
	if ( pWeaponAmmo == "" )
		return;

	CItem *pAmmo = (CItem *)CreateEntityByName(pWeaponAmmo);

	if ( !pAmmo )
	{
		DevWarning("[DIRECTOR WEAPON MAKER] Ha ocurrido un problema al crear la munici�n.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pAmmo, &origin) )
		return;

	QAngle angles = GetAbsAngles();

	// Lugar de creaci�n.
	pAmmo->SetAbsOrigin(origin);

	// Nombre de la Munici�n.
	// Iv�n: �NO CAMBIAR SIN AVISAR! Es utilizado por otras entidades para referirse a la munici�n creada por el director.
	pAmmo->SetName(MAKE_STRING("director_weapon_ammo"));
	pAmmo->SetAbsAngles(angles);

	// Creamos la arma, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pAmmo);
	pAmmo->SetOwnerEntity(this);
	pAmmo->Activate();
}

//=========================================================
// Selecciona una clase de arma.
// @TODO: Seleccionar dependiendo del nivel de estres del
// director.
//=========================================================
const char *CDirectorWeaponSpawn::SelectWeapon()
{
	int pRandom = random->RandomInt(1, 10);

	if ( pRandom == 1 )
		return ( m_spawnflags & SF_SPAWN_CROWBAR ) ? "weapon_crowbar" : SelectWeapon();

	if ( pRandom == 2 )
		return ( m_spawnflags & SF_SPAWN_PISTOL ) ? "weapon_pistol" : SelectWeapon();

	if ( pRandom == 3 )
		return ( m_spawnflags & SF_SPAWN_RIFLE ) ? "weapon_ar2" : SelectWeapon();

	if ( pRandom == 4 )
		return ( m_spawnflags & SF_SPAWN_SMG1 ) ? "weapon_smg1" : SelectWeapon();

	if ( pRandom == 5 )
		return ( m_spawnflags & SF_SPAWN_SHOTGUN ) ? "weapon_shotgun" : SelectWeapon();

	if ( pRandom == 6 )
		return ( m_spawnflags & SF_SPAWN_357 ) ? "weapon_357" : SelectWeapon();

	if ( pRandom == 7 )
		return ( m_spawnflags & SF_SPAWN_ALYXGUN ) ? "weapon_alyxgun" : SelectWeapon();

	if ( pRandom == 8 )
		return ( m_spawnflags & SF_SPAWN_CROSSBOW ) ? "weapon_crossbow" : SelectWeapon();

	if ( pRandom == 9 )
		return ( m_spawnflags & SF_SPAWN_FRAG ) ? "weapon_frag" : SelectWeapon();


	return SelectWeapon();
}

//=========================================================
// Selecciona una clase de munici�n.
//=========================================================
const char *CDirectorWeaponSpawn::SelectAmmo(const char *pWeaponClass)
{
	const char *pResult = "";

	if ( pWeaponClass == "weapon_pistol" )
		pResult = "item_ammo_pistol";

	if ( pWeaponClass == "weapon_ar2" )
		pResult = "item_ammo_ar2";

	if ( pWeaponClass == "weapon_smg1" )
		pResult = "item_ammo_smg1";

	if ( pWeaponClass == "weapon_shotgun" )
		pResult = "item_box_buckshot";

	if ( pWeaponClass == "weapon_357" )
		pResult = "item_ammo_357";

	if ( pWeaponClass == "weapon_crossbow" )
		pResult = "item_ammo_crossbow";

	if ( pWeaponClass == "weapon_frag" )
		pResult = "weapon_frag";

	if ( pResult != "" && pResult != "item_box_buckshot" && pResult != "item_ammo_crossbow" && pResult != "weapon_frag" )
	{
		if ( random->RandomInt(1, 5) > 3 )
			pResult = CFmtStr("%s_large", pResult);
	}

	return pResult;
}

//=========================================================
//=========================================================
// WEAPON_SPAWN
//=========================================================
//=========================================================

//=========================================================
// Guardado y definici�n de datos
//=========================================================

LINK_ENTITY_TO_CLASS( weapon_spawn, CWeaponSpawn );

BEGIN_DATADESC( CWeaponSpawn )

	DEFINE_KEYFIELD( OnlyAmmo, FIELD_BOOLEAN, "OnlyAmmo" )
	
END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CWeaponSpawn::CWeaponSpawn()
{
	//NextSpawn = gpGlobals->curtime;
	//UniqueKey		= random->RandomInt(1000, 9000); // Establecemos una ID �nica.
	pLastWeaponSpawned	= NULL;

	// Pensamos �ahora!
	SetNextThink(gpGlobals->curtime);
}

//=========================================================
// Pensar
//=========================================================
void CWeaponSpawn::Think()
{
	Make();
	SetNextThink( gpGlobals->curtime + random->RandomInt(sv_weapons_respawn_min_time.GetInt(), sv_weapons_respawn_max_time.GetInt()) );
}

//=========================================================
// Guardar los objetos necesarios en cach�.
//=========================================================
void CWeaponSpawn::Precache()
{
	if ( HasSpawnFlags(SF_SPAWN_CROWBAR) )
		UTIL_PrecacheOther("weapon_crowbar");

	if ( HasSpawnFlags(SF_SPAWN_PISTOL) )
	{
		UTIL_PrecacheOther("weapon_pistol");
		UTIL_PrecacheOther("item_ammo_pistol");
		UTIL_PrecacheOther("item_ammo_pistol_large");
	}

	if ( HasSpawnFlags(SF_SPAWN_RIFLE) )
	{
		UTIL_PrecacheOther("weapon_ar2");
		UTIL_PrecacheOther("item_ammo_ar2");
		UTIL_PrecacheOther("item_ammo_ar2_large");
	}

	if ( HasSpawnFlags(SF_SPAWN_SMG1) )
	{
		UTIL_PrecacheOther("weapon_smg1");
		UTIL_PrecacheOther("item_ammo_smg1");
		UTIL_PrecacheOther("item_ammo_smg1_large");
	}

	if ( HasSpawnFlags(SF_SPAWN_SHOTGUN) )
	{
		UTIL_PrecacheOther("weapon_shotgun");
		UTIL_PrecacheOther("item_box_buckshot");
	}

	if ( HasSpawnFlags(SF_SPAWN_357) )
	{
		UTIL_PrecacheOther("weapon_357");
		UTIL_PrecacheOther("item_ammo_357");
		UTIL_PrecacheOther("item_ammo_357_large");
	}

	if ( HasSpawnFlags(SF_SPAWN_ALYXGUN) )
		UTIL_PrecacheOther("weapon_alyxgun");

	if ( HasSpawnFlags(SF_SPAWN_CROSSBOW) )
	{
		UTIL_PrecacheOther("weapon_crossbow");
		UTIL_PrecacheOther("item_ammo_crossbow");
	}

	if ( HasSpawnFlags(SF_SPAWN_FRAG) )
		UTIL_PrecacheOther("weapon_frag");

	if ( HasSpawnFlags(SF_SPAWN_HEALTHKIT) )
	{
		UTIL_PrecacheOther("item_healthkit");
		UTIL_PrecacheOther("item_healthvial");
	}

	if ( HasSpawnFlags(SF_SPAWN_BATTERY) )
		UTIL_PrecacheOther("item_battery");

	if ( HasSpawnFlags(SF_SPAWN_BLOOD) )
		UTIL_PrecacheOther("item_bloodkit");

	if ( HasSpawnFlags(SF_SPAWN_BANDAGE) )
		UTIL_PrecacheOther("item_bandage");

	BaseClass::Precache();
}

//=========================================================
// Detecta si un arma/munici�n hijo ya ha sido creado
// y se encuentra dentro del radio.
//=========================================================
bool CWeaponSpawn::DetectTouch()
{
	// Buscamos nuestra arma creada.
	CBaseHLCombatWeapon *pEntity = (CBaseHLCombatWeapon *)gEntList.FindEntityByNameNearest("survivor_weapon", GetAbsOrigin(), DETECT_SPAWN_RADIUS, pLastWeaponSpawned);

	// No se encontro ninguna arma.
	if ( !pEntity )
	{
		// Buscamos por munici�n creada.
		pEntity = (CBaseHLCombatWeapon *)gEntList.FindEntityByNameNearest("survivor_weapon_ammo", GetAbsOrigin(), DETECT_SPAWN_RADIUS, pLastWeaponSpawned);

		if ( !pEntity )
		{
			// Buscamos por salud creada.
			pEntity = (CBaseHLCombatWeapon *)gEntList.FindEntityByNameNearest("survivor_item", GetAbsOrigin(), DETECT_SPAWN_RADIUS, pLastWeaponSpawned);

			if ( pEntity )
				return true;

		}
		else
			return true;
	}
	else 
		return true;

	return false;
}

//=========================================================
//=========================================================
void CWeaponSpawn::Make()
{
	// Desactivado.
	if ( Disabled || !sv_weapons_respawn.GetBool() )
		return;

	// El arma que hemos creado anteriormente a�n sigue aqu�.
	if ( DetectTouch() )
		return;

	// Emm... �puso todas las clases en "no crear"? :genius:
	// @TODO: Mejorar c�digo.
	if ( !HasSpawnFlags(SF_SPAWN_CROWBAR | SF_SPAWN_PISTOL | SF_SPAWN_RIFLE | SF_SPAWN_SMG1 | SF_SPAWN_SHOTGUN | SF_SPAWN_357 | SF_SPAWN_ALYXGUN | SF_SPAWN_CROSSBOW | SF_SPAWN_FRAG | SF_SPAWN_HEALTHKIT | SF_SPAWN_BATTERY | SF_SPAWN_BLOOD | SF_SPAWN_BANDAGE))
		return;

	// Seleccionamos una clase de arma para crear.
	const char *pWeaponClass = SelectWeapon();

	// �Granadas!
	if ( pWeaponClass == "weapon_frag" )
	{
		int i;

		// Creamos de 2 a 4 granadas.
		for ( i = 0; i <= random->RandomInt(1, 3); i = i + 1 )
			MakeWeapon("weapon_frag");

		return;
	}

	// Es un objeto.
	if ( Q_stristr(pWeaponClass, "kit") || pWeaponClass == "item_battery" || pWeaponClass == "item_bandage" )
	{
		MakeItem(pWeaponClass);
		return;
	}

	// Esta vez creamos munici�n.
	if ( random->RandomInt(1, 3) == 2 || OnlyAmmo )
		MakeAmmo(pWeaponClass);
	else
		MakeWeapon(pWeaponClass);
}

//=========================================================
//=========================================================
void CWeaponSpawn::MakeWeapon(const char *pWeaponClass)
{
	// Desactivado.
	if ( Disabled )
		return;

	CBaseHLCombatWeapon *pWeapon = (CBaseHLCombatWeapon *)CreateEntityByName(pWeaponClass);

	if ( !pWeapon )
	{
		DevWarning("[WEAPON SPAWN] Ha ocurrido un problema al crear el arma.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pWeapon, &origin) )
		return;

	QAngle angles	= GetAbsAngles();

	// Lugar de creaci�n.
	pWeapon->SetAbsOrigin(origin);	

	// Nombre de la entidad.
	// Iv�n: �NO CAMBIAR SIN AVISAR!
	pWeapon->SetName(MAKE_STRING("survivor_weapon"));
	pWeapon->SetAbsAngles(angles);

	// Creamos el arma, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pWeapon);
	pWeapon->SetOwnerEntity(this);
	pWeapon->Activate();

	pLastWeaponSpawned = pWeapon;
}

//=========================================================
//=========================================================
void CWeaponSpawn::MakeAmmo(const char *pWeaponClass)
{
	// Desactivado.
	if ( Disabled )
		return;

	// Obtenemos el tipo de munici�n para esta arma.
	const char *pWeaponAmmo = SelectAmmo(pWeaponClass);

	// Esta arma no tiene munici�n.
	if ( pWeaponAmmo == "" )
		return;

	CItem *pAmmo = (CItem *)CreateEntityByName(pWeaponAmmo);

	if ( !pAmmo )
	{
		DevWarning("[WEAPON MAKER] Ha ocurrido un problema al crear la munici�n.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pAmmo, &origin) )
		return;

	QAngle angles = GetAbsAngles();

	// Lugar de creaci�n.
	pAmmo->SetAbsOrigin(origin);

	// Nombre de la Munici�n.
	// Iv�n: �NO CAMBIAR SIN AVISAR! Es utilizado por otras entidades para referirse a la munici�n creada por el director.
	pAmmo->SetName(MAKE_STRING("survivor_weapon_ammo"));
	pAmmo->SetAbsAngles(angles);

	// Creamos la arma, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pAmmo);
	pAmmo->SetOwnerEntity(this);
	pAmmo->Activate();

	pLastWeaponSpawned = pAmmo;
}

//=========================================================
//=========================================================
void CWeaponSpawn::MakeItem(const char *pItemClass)
{
	// Desactivado.
	if ( Disabled )
		return;

	// Creamos el objeto.
	CItem *pItem = (CItem *)CreateEntityByName(pItemClass);

	// Hubo un problema al crear el objeto.
	if ( !pItem )
	{
		DevWarning("[WEAPON MAKER] Ha ocurrido un problema al crear salud.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pItem, &origin) )
		return;

	QAngle angles	= GetAbsAngles();

	// Lugar de creaci�n.
	pItem->SetAbsOrigin(origin);

	// Nombre de la entidad.
	// Iv�n: �NO CAMBIAR SIN AVISAR!
	pItem->SetName(MAKE_STRING("survivor_item"));
	pItem->SetAbsAngles(angles);

	// Creamos el botiquin, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pItem);
	pItem->SetOwnerEntity(this);
	pItem->Activate();

	pLastWeaponSpawned = pItem;
}

//=========================================================
// Selecciona una clase de arma.
// @TODO: Seleccionar dependiendo del nivel de estres del
// director.
//=========================================================
const char *CWeaponSpawn::SelectWeapon()
{
	int pRandom = random->RandomInt(1, 14);

	if ( pRandom == 1 && HasSpawnFlags(SF_SPAWN_CROWBAR) )
		return "weapon_crowbar";

	if ( pRandom == 2 && HasSpawnFlags(SF_SPAWN_PISTOL) )
		return "weapon_pistol";

	if ( pRandom == 3 && HasSpawnFlags(SF_SPAWN_RIFLE) )
		return "weapon_ar2";

	if ( pRandom == 4 && HasSpawnFlags(SF_SPAWN_SMG1) )
		return "weapon_smg1";

	if ( pRandom == 5 && HasSpawnFlags(SF_SPAWN_SHOTGUN) )
		return "weapon_shotgun";

	if ( pRandom == 6 && HasSpawnFlags(SF_SPAWN_357) )
		return "weapon_357";

	if ( pRandom == 7 && HasSpawnFlags(SF_SPAWN_ALYXGUN) )
		return "weapon_alyxgun";

	if ( pRandom == 8 && HasSpawnFlags(SF_SPAWN_CROSSBOW) )
		return "weapon_crossbow";

	if ( pRandom == 9 && HasSpawnFlags(SF_SPAWN_FRAG) )
		return "weapon_frag";

	if ( pRandom == 10 && HasSpawnFlags(SF_SPAWN_HEALTHKIT) )
	{
		if ( random->RandomInt(1, 5) == 2 )
			return "item_healthvial";
		else
			return "item_healthkit";
	}

	if ( pRandom == 11 && HasSpawnFlags(SF_SPAWN_BATTERY) )
		return "item_battery";

	if ( pRandom == 12 && HasSpawnFlags(SF_SPAWN_BLOOD) )
		return "item_bloodkit";

	if ( pRandom == 13 && HasSpawnFlags(SF_SPAWN_BANDAGE) )
		return "item_bandage";


	return SelectWeapon();
}

//=========================================================
// Selecciona una clase de munici�n.
//=========================================================
const char *CWeaponSpawn::SelectAmmo(const char *pWeaponClass)
{
	const char *pResult = "";

	if ( pWeaponClass == "weapon_pistol" )
		pResult = "item_ammo_pistol";

	if ( pWeaponClass == "weapon_ar2" )
		pResult = "item_ammo_ar2";

	if ( pWeaponClass == "weapon_smg1" )
		pResult = "item_ammo_smg1";

	if ( pWeaponClass == "weapon_shotgun" )
		pResult = "item_box_buckshot";

	if ( pWeaponClass == "weapon_357" )
		pResult = "item_ammo_357";

	if ( pWeaponClass == "weapon_crossbow" )
		pResult = "item_ammo_crossbow";

	if ( pWeaponClass == "weapon_frag" )
		pResult = "weapon_frag";

	if ( pResult != "" && pResult != "item_box_buckshot" && pResult != "item_ammo_crossbow" && pResult != "weapon_frag" )
	{
		if ( random->RandomInt(1, 5) > 3 )
			pResult = CFmtStr("%s_large", pResult);
	}

	return pResult;
}