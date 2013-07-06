//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Funciones del jugador del lado de cliente.
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#ifndef	C_FR_PLAYER_H
#define C_FR_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "c_in_player.h"

class C_FR_Player : public C_IN_Player
{
	DECLARE_CLASS( C_FR_Player, C_IN_Player );
	//DECLARE_CLIENTCLASS();

public:
	C_FR_Player();
	~C_FR_Player() { }

	static C_FR_Player* GetLocalFRPlayer();

	// FUNCIONES DE DEVOLUCIÓN DE DATOS
	
	// Devuelve la cantidad de Nectar en el cuerpo.
	int GetNectar() const { return m_HL2Local.m_iNectar; }
};

#endif