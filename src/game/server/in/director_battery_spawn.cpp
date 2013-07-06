//=====================================================================================//
//
// Creador de baterias
//
// Entidad que trabajara en conjunto con Director para la creación de baterias según
// el nivel y estres del jugador.
//
// Inspiración: Left 4 Dead 2
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "director_base_spawn.h"
#include "director_battery_spawn.h"

#include "director.h"
#include "items.h"
#include "in_utils.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar director_min_energy_tospawn_battery("director_min_energy_tospawn_battery", "80", 0, "Energia minima para la creación de baterias.");

#define ITEM_NAME			"director_battery"
#define MINHEALTH_EASY		director_min_energy_tospawn_battery.GetInt()
#define MINHEALTH_MEDIUM	( MINHEALTH_EASY - 20 )
#define MINHEALTH_HARD		( MINHEALTH_MEDIUM - 10 )

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

	// Creamos la bateria.
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
	pItem->SetName(MAKE_STRING(ITEM_NAME));
	pItem->SetAbsAngles(angles);

	// Creamos la bateria, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pItem);
	pItem->SetOwnerEntity(this);
	pItem->Activate();
}

//=========================================================
//=========================================================
bool CDirectorBatterySpawn::MaySpawn()
{
	CDirector *pDirector = Director();

	switch ( GameRules()->GetSkillLevel() )
	{
		// Fácil
		case SKILL_EASY:
			if ( UTIL_GetPlayersBattery() < MINHEALTH_EASY )
				return true;
		break;

		// Normal
		case SKILL_MEDIUM:
			if ( UTIL_GetPlayersBattery() < MINHEALTH_MEDIUM )
				return true;
		break;

		// Dificil
		case SKILL_HARD:
			if ( UTIL_GetPlayersBattery() < MINHEALTH_HARD )
				return true;
		break;
	}

	return false;
}