//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Funciones del jugador del lado de cliente.
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#include "cbase.h"
#include "c_fr_player.h"

#if defined( CFR_Player )
#undef CFR_Player	
#endif

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef FURNANCE
	LINK_ENTITY_TO_CLASS(player, C_FR_Player);
#endif

//=========================================================
// Obtiene el jugador local convertido a C_FR_Player
//=========================================================
C_FR_Player* C_FR_Player::GetLocalFRPlayer()
{
	return (C_FR_Player *)C_BasePlayer::GetLocalPlayer();
}

//=========================================================
// Constructor
//=========================================================
C_FR_Player::C_FR_Player()
{
}