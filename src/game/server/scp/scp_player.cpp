//=====================================================================================//
//
// Sistema encargado de constrolar al personaje.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "scp_player.h"

#include "scp_gamerules.h"

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef SCP
	LINK_ENTITY_TO_CLASS( player, CSCP_Player );
	PRECACHE_REGISTER(player);
#endif

BEGIN_DATADESC( CSCP_Player )
END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CSCP_Player::CSCP_Player()
{
}

//=========================================================
// Destructor
//=========================================================
CSCP_Player::~CSCP_Player()
{
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CSCP_Player::Precache()
{
	BaseClass::Precache();
}

//=========================================================
// Crear al jugador.
//=========================================================
void CSCP_Player::Spawn()
{
	BaseClass::Spawn();
}

//=========================================================
//=========================================================
// FUNCIONES DE UTILIDAD
//=========================================================
//=========================================================

//=========================================================
// Dependiendo del modelo, devuelve si el jugador es hombre
// o mujer. (Util para las voces de dolor)
//=========================================================
PlayerGender CSCP_Player::Gender()
{
	// En modo Historia jugamos como D-9341 (Hombre)
	if ( !InGameRules()->IsMultiplayer() )
		return PLAYER_MALE;

	return BaseClass::Gender();
}

//=========================================================
// Ejecuta un comando de tipo impulse <iImpulse>
//=========================================================
void CSCP_Player::CheatImpulseCommands(int iImpulse)
{
	if ( !sv_cheats->GetBool() )
		return;

	switch ( iImpulse )
	{
		case 102:
		{
			if ( GetHealth() < GetMaxHealth() )
				TakeHealth(GetMaxHealth(), DMG_GENERIC);

			return;
		}

		default:
			BaseClass::CheatImpulseCommands(iImpulse);
	}
}