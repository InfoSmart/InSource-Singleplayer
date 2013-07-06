//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Información del jugador que se comparte (o es necesario) entre Servidor y Cliente.
//
//=============================================================================//
#ifndef IN_PLAYER_SHARED_H

#define IN_PLAYER_SHARED_H
#pragma once

#include "studio.h"

// Inventario
enum PlayerInventory
{
	INVENTORY_POCKET = 1,	// Bolsillo
	INVENTORY_BACKPACK,		// Mochila
	INVENTORY_ALL			// Todos
};

// Precisión.
enum PlayerAccuracy
{
	ACCURACY_HIGH = 1,		// Alto
	ACCURACY_MEDIUM,		// Medio
	ACCURACY_LOW,			// Bajo
	ACCURACY_NONE			// Ninguna
};

// Genero
enum PlayerGender
{
	PLAYER_MALE = 1,		// Hombre
	PLAYER_FEMALE			// Mujer
};

enum
{
	CHAT_IGNORE_NONE = 0,
	CHAT_IGNORE_ALL
};

#ifdef CLIENT_DLL
	#define CIN_Player C_IN_Player
#endif

#endif