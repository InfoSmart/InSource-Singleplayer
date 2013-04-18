//=====================================================================================//
//
// Creador de objetos base
//
// Sistema base para la creación de una entidad que pueda crear objetos/armas/etc
// dependiendo del nivel de estres del jugador, el nivel de enojo del Director y otros
// aspectos del juego.
//
// Inspiración: Left 4 Dead 2
//
//=====================================================================================//

#include "cbase.h"
#include "director_base_spawn.h"

#include "tier0/memdbgon.h"

#define SPAWN_RADIUS 10

//=========================================================
// Guardado y definición de datos
//=========================================================

BEGIN_DATADESC( CDirectorBaseSpawn )

	DEFINE_INPUTFUNC( FIELD_VOID, "Spawn",		InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",		InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",		InputToggle ),

	DEFINE_KEYFIELD( Disabled, FIELD_BOOLEAN, "StartDisabled" )

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CDirectorBaseSpawn::CDirectorBaseSpawn()
{
}

//=========================================================
// Destructor
//=========================================================
CDirectorBaseSpawn::~CDirectorBaseSpawn()
{
}

//=========================================================
// Creación
//=========================================================
void CDirectorBaseSpawn::Spawn()
{
	BaseClass::Spawn();

	// Somos invisibles.
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);

	// Caché
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================

void CDirectorBaseSpawn::Precache()
{
	BaseClass::Precache();
}

//=========================================================
// Activa el creador
//=========================================================
void CDirectorBaseSpawn::Enable()
{
	Disabled = false;
}

//=========================================================
// Desactiva el creador
//=========================================================
void CDirectorBaseSpawn::Disable()
{
	Disabled = true;
}

//=========================================================
//=========================================================
bool CDirectorBaseSpawn::CanSpawn(CBaseEntity *pItem, Vector *pResult)
{
	if ( Disabled )
	{
		UTIL_RemoveImmediate(pItem);
		return false;
	}

	Vector origin;

	if ( !FindSpotInRadius(&origin, GetAbsOrigin(), pItem, SPAWN_RADIUS) )
		origin = GetAbsOrigin();
	else
		origin.z = GetAbsOrigin().z;

	if ( HasSpawnFlags(SF_NO_SPAWN_VIEWCONE) )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		if ( pPlayer->FInViewCone(origin) || pPlayer->FVisible(origin) )
		{
			DevWarning("[DIRECTOR SPAWN] El lugar de creación estaba en el campo de vision. \r\n");
			UTIL_RemoveImmediate(pItem);

			return false;
		}
	}

	*pResult = origin;
	return true;
}

//=========================================================
// Intenta encontrar un lugar disponible para la creación.
//=========================================================
bool CDirectorBaseSpawn::FindSpotInRadius(Vector *pResult, const Vector &vStartPos, CBaseEntity *pItem, float radius)
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	QAngle fan;

	fan.x = 0;
	fan.z = 0;

	for ( fan.y = 0; fan.y < 360; fan.y += 18.0 )
	{
		Vector vecTest;
		Vector vecDir;

		AngleVectors(fan, &vecDir);
		vecTest = vStartPos + vecDir * radius;

		trace_t tr;
		UTIL_TraceLine(vecTest, vecTest - Vector(0, 0, 8192), MASK_SHOT, pItem, COLLISION_GROUP_NONE, &tr);

		if ( tr.fraction == 1.0 )
			continue;

		*pResult = tr.endpos;
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS)
//=========================================================
//=========================================================

void CDirectorBaseSpawn::InputSpawn(inputdata_t &inputdata)
{
	Make();
}

void CDirectorBaseSpawn::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

void CDirectorBaseSpawn::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

void CDirectorBaseSpawn::InputToggle(inputdata_t &inputdata)
{
	if( Disabled )
		Enable();
	else
		Disable();
}