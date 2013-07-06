//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
	#include "prediction.h"
	#define CRecipientFilter C_RecipientFilter
#else
	#include "in_player.h"
	#include "ai_basenpc.h"
	#include "util.h"
#endif

#include "in_player_shared.h"

// Traducción de animaciones sin armas.
acttable_t	unarmedActtable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE,					false },	// Sin hacer nada
	{ ACT_MP_WALK,						ACT_HL2MP_WALK,					false },	// Caminando
	{ ACT_MP_RUN,						ACT_HL2MP_RUN,					false },	// Corriendo
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH,			false },	// Agachado
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH,			false },	// Agachado y caminando
	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_DUEL,			false },	// Saltando
	{ ACT_MP_JUMP_START,				ACT_HL2MP_JUMP_DUEL,			false },	// Saltando
	{ ACT_MP_SWIM,						ACT_HL2MP_SWIM,					false },	// Nadando
	{ ACT_MP_SWIM_IDLE,					ACT_HL2MP_SWIM_IDLE,			false },	// Nadando sin hacer nada.
	{ ACT_MP_AIRWALK,					ACT_HL2MP_SWIM_IDLE,			false },	// Flotando (NOCLIP)
};

//=========================================================
// Traduce una animación.
//=========================================================
Activity CIN_Player::TranslateActivity(Activity baseAct, bool *pRequired)
{
	// Esta será la nueva animación.
	Activity translated = baseAct;
		
	// El jugador tiene una arma, usar la animación con esa arma.
	if ( GetActiveWeapon() )
		translated = GetActiveWeapon()->ActivityOverride(baseAct, pRequired);

	// Estamos desarmados, verificar si existe una animación.
	else if ( unarmedActtable )
	{
		acttable_t *pTable	= unarmedActtable;
		int actCount		= ARRAYSIZE(unarmedActtable);

		for ( int i = 0; i < actCount; i++, pTable++ )
		{
			if ( baseAct == pTable->baseAct )
				translated = (Activity)pTable->weaponAct;
		}
	}
	else if ( pRequired )
		*pRequired = false;

	// @DEBUG: Guardamos la animación nueva para poder usarla en la información de depuración: anim_showstatelog
#ifndef CLIENT_DLL
	//DevMsg("[ANIM] %s \r\n", ActivityList_NameForIndex(translated));
#endif

	return translated;
}

//=========================================================
// Calcula la nueva velocidad del jugador dependiendo del
// peso de la arma.
//=========================================================
float CIN_Player::CalculateSpeed(CBaseCombatWeapon *pWeapon, float speed)
{
	// No se especifico el arma, obtenerla automaticamente.
	if ( pWeapon == NULL )
		pWeapon = GetActiveWeapon();

	ConVarRef hl2_sprintspeed("hl2_sprintspeed");
	ConVarRef hl2_walkspeed("hl2_walkspeed");

	// Obtenemos la velocidad inicial.
	if ( speed == 0 && IsSprinting() )
		speed = hl2_sprintspeed.GetFloat();

	if ( speed == 0 && !IsSprinting() )
		speed = hl2_walkspeed.GetFloat();

	float newSpeed = speed;

	// Arma válida.
	if ( pWeapon )
	{
		// Obtenemos el peso del arma.
		float weaponWeight = pWeapon->GetWpnData().m_WeaponWeight;

		// El arma es muy ligera, te da velocidad.
		if ( weaponWeight < 0 )
		{
			weaponWeight	= fabs(weaponWeight);
			newSpeed		= newSpeed + weaponWeight;
		}
		else
			newSpeed = newSpeed - weaponWeight;
	}

	// Menos de 40% de salud.
	if ( GetHealth() <= 40 )
	{
		// Disminuimos más velocidad entre menos salud tengamos.

		if ( GetHealth() <= 5 )
			newSpeed = newSpeed - 10;

		else if ( GetHealth() <= 10 )
			newSpeed = newSpeed - 5;

		else if ( GetHealth() <= 20 )
			newSpeed = newSpeed - 3;

		else if ( GetHealth() <= 40 )
			newSpeed = newSpeed - 2;
	}

	if ( newSpeed < 10 )
		newSpeed = 10;

	return newSpeed;
}