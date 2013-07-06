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
#include "director_healthkit_spawn.h"

#include "director.h"
#include "items.h"
#include "in_utils.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar director_min_health_tospawn_healthkit("director_min_health_tospawn_healthkit", "80");

#define ITEM_NAME			"director_healthkit"
#define MINHEALTH_EASY		director_min_health_tospawn_healthkit.GetInt()
#define MINHEALTH_MEDIUM	( MINHEALTH_EASY - 20 )
#define MINHEALTH_HARD		( MINHEALTH_MEDIUM - 10 )

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( director_healthkit_spawn, CDirectorHealthSpawn );

BEGIN_DATADESC( CDirectorHealthSpawn )	
END_DATADESC();

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirectorHealthSpawn::Precache()
{
	UTIL_PrecacheOther("item_healthkit");
	UTIL_PrecacheOther("item_healthvial");

	BaseClass::Precache();
}

//=========================================================
//=========================================================
void CDirectorHealthSpawn::Make()
{
	// Desactivado.
	if ( Disabled )
		return;

	if ( !MaySpawn() )
		return;

	// Creamos la salud.
	CItem *pItem = (CItem *)CreateEntityByName(SelectClass());

	// Hubo un problema al crear el objeto.
	if ( !pItem )
	{
		DevWarning("[DIRECTOR HEALTHKIT MAKER] Ha ocurrido un problema al crear salud.");
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

	// Creamos el botiquin, le decimos quien es su dios (creador) y lo activamos.
	DispatchSpawn(pItem);
	pItem->SetOwnerEntity(this);
	pItem->Activate();
}

//=========================================================
//=========================================================
const char *CDirectorHealthSpawn::SelectClass()
{
	CDirector *pDirector = Director();

	switch ( GameRules()->GetSkillLevel() )
	{
		// Fácil
		case SKILL_EASY:
			if ( random->RandomInt(1, UTIL_GetPlayersHealth()) > 50 )
				return "item_healthvial";
			else
				return "item_healthkit";
		break;

		// Normal
		case SKILL_MEDIUM:
			if ( random->RandomInt(1, UTIL_GetPlayersHealth()) > 40 )
				return "item_healthvial";
			else
				return "item_healthkit";
		break;

		// Dificil
		case SKILL_HARD:
			if ( random->RandomInt(1, UTIL_GetPlayersHealth()) > 30 )
				return "item_healthvial";
			else
				return "item_healthkit";
		break;
	}
	
	// Susupone que jamas llegaría hasta aquí, pero por si acaso...
	return "item_healthvial";
}

//=========================================================
//=========================================================
bool CDirectorHealthSpawn::MaySpawn()
{
	CDirector *pDirector = Director();

	switch ( GameRules()->GetSkillLevel() )
	{
		// Fácil
		case SKILL_EASY:
			if ( UTIL_GetPlayersHealth() < MINHEALTH_EASY )
				return true;
		break;
			
		// Normal
		case SKILL_MEDIUM:
			if ( UTIL_GetPlayersHealth() < MINHEALTH_MEDIUM )
				return true;
		break;
			
		// Dificil
		case SKILL_HARD:
			if ( UTIL_GetPlayersHealth() < MINHEALTH_HARD )
				return true;
		break;
	}

	return false;
}