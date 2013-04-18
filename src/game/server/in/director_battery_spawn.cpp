//=====================================================================================//
//
// Creador de armas
//
// Entidad que trabajara en conjunto con InDirector para la creación de armas según
// el nivel y estres del jugador.
//
// Inspiración: Left 4 Dead 2
//
//=====================================================================================//

#include "cbase.h"
#include "director_base_spawn.h"
#include "director_battery_spawn.h"

#include "director.h"
#include "items.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar indirector_minenergy_tospawn_battery("indirector_minenergy_tospawn_battery", "80");

#define MINHEALTH_EASY		indirector_minenergy_tospawn_battery.GetInt()
#define MINHEALTH_MEDIUM	(MINHEALTH_EASY - 20)
#define MINHEALTH_HARD		(MINHEALTH_MEDIUM - 10)

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_battery_spawn, CDirectorBatterySpawn );

BEGIN_DATADESC( CDirectorBatterySpawn )	
END_DATADESC();

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirectorBatterySpawn::Precache()
{
	UTIL_PrecacheOther("item_battery");
	BaseClass::Precache();
}

//=========================================================
//=========================================================
void CDirectorBatterySpawn::Make()
{
	// Desactivado.
	if ( Disabled )
		return;

	if ( !MaySpawn() )
		return;

	// Creamos la salud.
	CItem *pItem = (CItem *)CreateEntityByName("item_battery");

	// Hubo un problema al crear el objeto.
	if ( !pItem )
	{
		DevWarning("[DIRECTOR BATTERY MAKER] Ha ocurrido un problema al crear la bateria.");
		return;
	}

	Vector origin;

	if ( !CanSpawn(pItem, &origin) )
		return;

	QAngle angles = GetAbsAngles();

	// Lugar de creación.
	pItem->SetAbsOrigin(origin);

	// Nombre de la entidad.
	// Iván: ¡NO CAMBIAR SIN AVISAR! Es utilizado por otras entidades para referirse a las baterias creados por el director.
	pItem->SetName(MAKE_STRING("director_battery"));
	pItem->SetAbsAngles(angles);

	// Creamos el botiquin, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pItem);
	pItem->SetOwnerEntity(this);
	pItem->Activate();
}

//=========================================================
//=========================================================
bool CDirectorBatterySpawn::MaySpawn()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	switch ( GameRules()->GetSkillLevel() )
	{
		case SKILL_EASY:
			if ( pPlayer->GetBattery() < MINHEALTH_EASY )
				return true;
		break;

		case SKILL_MEDIUM:
			if ( pPlayer->GetBattery() < MINHEALTH_MEDIUM )
				return true;
		break;

		case SKILL_HARD:
			if ( pPlayer->GetBattery() < MINHEALTH_HARD )
				return true;
		break;
	}

	return false;
}