//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Funciones del jugador. Client-Side
//
//====================================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "takedamageinfo.h"
#include "c_in_player.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

// Modelo
static ConVar cl_playermodel("cl_playermodel",		"models/abigail.mdl",	FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE,		"Define el modelo del jugador");

//=========================================================
// Guardado y definición de datos
//=========================================================

void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

LINK_ENTITY_TO_CLASS(player, C_INPlayer);

IMPLEMENT_CLIENTCLASS_DT(C_INPlayer, DT_IN_Player, CINPlayer)
END_RECV_TABLE()

C_INPlayer::C_INPlayer()
{
	ConVarRef scissor("r_flashlightscissor");
	scissor.SetValue("0");
}

void C_INPlayer::TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr)
{
	Vector vecOrigin	= ptr->endpos - vecDir * 4;
	float flDistance	= 0.0f;

	if ( info.GetAttacker() )
		flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();

	if ( m_takedamage )
	{
		AddMultiDamage(info, this);
		int blood = BloodColor();

		CBaseEntity *pAttacker = info.GetAttacker();

		if ( blood != DONT_BLEED )
		{
			SpawnBlood(vecOrigin, vecDir, blood, flDistance);
			TraceBleed(flDistance, vecDir, ptr, info.GetDamageType());
		}
	}
}

void C_INPlayer::DoImpactEffect(trace_t &tr, int nDamageType)
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect(tr, nDamageType);
		return;
	}

	BaseClass::DoImpactEffect(tr, nDamageType);
}

bool C_INPlayer::ShouldDraw()
{
	if ( !IsAlive() )
		return false;

	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if ( IsLocalPlayer() && IsRagdoll() )
		return true;
	
	if ( IsRagdoll() )
		return false;

	return BaseClass::ShouldDraw();
}

float C_INPlayer::GetFOV()
{
	float flFOVOffset	= CBasePlayer::GetFOV() + GetZoom();
	int min_fov			= GetMinFOV();

	flFOVOffset = max(min_fov, flFOVOffset);
	return flFOVOffset;
}

const char *C_INPlayer::GetConVar(const char *pConVar)
{
	ConVarRef pVar(pConVar);

	if ( pVar.IsValid() )
		return pVar.GetString();
}