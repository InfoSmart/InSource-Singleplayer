//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Funciones del jugador. Client-Side
//
//====================================================================================//

#include "cbase.h"
#include "takedamageinfo.h"
#include "c_in_player.h"
#include "prediction.h"

#include "iviewrender_beams.h"	
#include "r_efx.h"
#include "dlight.h"
#include "beam_shared.h"
#include "iinput.h"
#include "input.h"

#if defined( CIN_Player )
#undef CIN_Player	
#endif

//=========================================================
// Definición de variables de configuración.
//=========================================================

// Modelo
static ConVar cl_playermodel("cl_playermodel",		"models/abigail.mdl",	FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE,		"Define el modelo del jugador");

//=========================================================
// Guardado y definición de datos
//=========================================================

#define FLASHLIGHT_DISTANCE 1000
void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

LINK_ENTITY_TO_CLASS(player, C_IN_Player);

BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_INLocalPlayerExclusive )
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_INNonLocalPlayerExclusive )
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_IN_Player, DT_IN_Player, CIN_Player)
	RecvPropDataTable( "inlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_INLocalPlayerExclusive) ),
	RecvPropDataTable( "innonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_INNonLocalPlayerExclusive) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_IN_Player )
END_PREDICTION_DATA()

//=========================================================
// Constructor
//=========================================================
C_IN_Player::C_IN_Player() : m_iv_angEyeAngles("C_IN_Player::m_iv_angEyeAngles")
{
	AddVar(&m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR);
	m_pFlashlightBeam = NULL;
}

//=========================================================
// Obtiene el jugador local convertido a C_IN_Player
//=========================================================
C_IN_Player* C_IN_Player::GetLocalINPlayer()
{
	return (C_IN_Player *)C_BasePlayer::GetLocalPlayer();
}

//=========================================================
// Traza un ataque.
// @TODO: ¿Es utilizado?
//=========================================================
void C_IN_Player::TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr)
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

//=========================================================
// Crea el efecto de impacto.
// @TODO: ¿Es utilizado?
//=========================================================
void C_IN_Player::DoImpactEffect(trace_t &tr, int nDamageType)
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect(tr, nDamageType);
		return;
	}

	BaseClass::DoImpactEffect(tr, nDamageType);
}

//=========================================================
// Devuelve si esta permitido dibujar el modelo.
// @TODO: ¿Es utilizado?
//=========================================================
bool C_IN_Player::ShouldDraw()
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

//=========================================================
// Obtiene el FOV
// @TODO: ¿Es utilizado?
//=========================================================
float C_IN_Player::GetFOV()
{
	float flFOVOffset	= CBasePlayer::GetFOV() + GetZoom();
	int min_fov			= GetMinFOV();

	flFOVOffset = max(min_fov, flFOVOffset);
	return flFOVOffset;
}

//=========================================================
// Obtiene los angulos de los ojos sea del jugador local o
// de otro jugador.
//=========================================================
const QAngle &C_IN_Player::EyeAngles()
{
	if ( IsLocalPlayer() )
		return BaseClass::EyeAngles();
	else
		return m_angEyeAngles;
}

//=========================================================
// Agrega la entidad.
//=========================================================
void C_IN_Player::AddEntity()
{
	BaseClass::AddEntity();

	// Linterna encendida.
	if ( IsEffectActive(EF_DIMLIGHT) )
	{
		// Si el jugador local es el jugador actual y se ha creado un destello, eliminarlo.
		// Esto no debería pasar nunca pero por si acaso (o si alguien soluciona el bug de más abajo)
		if ( this == C_BasePlayer::GetLocalPlayer() && m_pFlashlightBeam != NULL )
		{
			ReleaseFlashlight();
			return;
		}

		// El jugador local es alguien más (Estamos viendolo), creamos un destello de su linterna.
		// !!!BUG: Si activamos esto para el mismo jugador en tercera persona el destello se ve mal y no sincronizado con la luz de la linterna.
		// una posible solución sería crear un acoplamiento especial para el destello/linterna en cada modelo.
		else if ( this != C_BasePlayer::GetLocalPlayer() )
		{
			ConVarRef sv_flashlight_weapon("sv_flashlight_weapon");

			C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
			int iAttachment				= (sv_flashlight_weapon.GetBool() && GetActiveWeapon()) ? pWeapon->LookupAttachment("muzzle") : LookupAttachment("eyes");

			// No existe ningún acoplamiento (¿Arma/Modelo personalizado?)
			if ( iAttachment < 0)
				return;

			// Valores predeterminados.
			Vector vecOrigin = EyePosition();
			QAngle eyeAngles = EyeAngles();

			// El servidor quiere que las luces esten acopladas en las armas.
			if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() )
			{
				if ( iAttachment < 1 )
					vecOrigin = EyePosition() - Vector(0, 0, 32);
				else
					pWeapon->GetAttachment(iAttachment, vecOrigin, eyeAngles);
			}

			// Todo normal
			else
				GetAttachment(iAttachment, vecOrigin, eyeAngles);

			Vector vForward;
			AngleVectors(eyeAngles, &vForward);

			trace_t tr;
			UTIL_TraceLine(vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

			// Creamos el destello.
			if ( !m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_nType		= TE_BEAMPOINTS;
				beamInfo.m_vecStart		= tr.startpos;
				beamInfo.m_vecEnd		= tr.endpos;
				beamInfo.m_pszModelName = "sprites/glow_test02.vmt"; // @TODO: ¿Mejores texturas?
				beamInfo.m_pszHaloName	= "sprites/light_glow03.vmt";
				beamInfo.m_flHaloScale	= 3.0;
				beamInfo.m_flWidth		= 8.0f;
				beamInfo.m_flEndWidth	= 100.0f;
				beamInfo.m_flFadeLength = 400.0f;
				beamInfo.m_flAmplitude	= 0;
				beamInfo.m_flBrightness = 180.0;
				beamInfo.m_flSpeed		= 0.0f;
				beamInfo.m_nStartFrame	= 0.0;
				beamInfo.m_flFrameRate	= 0.0;
				beamInfo.m_flRed		= 255.0;
				beamInfo.m_flGreen		= 255.0;
				beamInfo.m_flBlue		= 255.0;
				beamInfo.m_nSegments	= 8;
				beamInfo.m_bRenderable	= true;
				beamInfo.m_flLife		= 0.5;
				beamInfo.m_nFlags		= FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE;

				m_pFlashlightBeam = beams->CreateBeamPoints(beamInfo);
			}

			// El destello ya ha sido creado, actualizarlo.
			if ( m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd	= tr.endpos;
				beamInfo.m_flRed	= 255.0;
				beamInfo.m_flGreen	= 255.0;
				beamInfo.m_flBlue	= 255.0;

				beams->UpdateBeamInfo(m_pFlashlightBeam, beamInfo);

				// Tony; local players don't make the dlight.
				if ( this != C_BasePlayer::GetLocalPlayer() )
				{
					dlight_t *el	= effects->CL_AllocDlight(0);
					el->origin		= tr.endpos;
					el->radius		= 50; 
					el->color.r		= 200;
					el->color.g		= 200;
					el->color.b		= 200;
					el->die			= gpGlobals->curtime + 0.1;
				}
			}
		}
	}
	else if ( m_pFlashlightBeam )
		ReleaseFlashlight();
}

//=========================================================
// Actualiza la ubicación de la luz de la linterna.
//=========================================================
void C_IN_Player::UpdateFlashlight()
{
	// Linterna encendida.
	if ( IsEffectActive( EF_DIMLIGHT ) )
	{
		// No ha sido creado el efecto de luz, intentamos crearlo.
		if ( !m_pFlashlight )
		{
			// Turned on the headlight; create it.
			m_pFlashlight = new CFlashlightEffect(index);

			if ( !m_pFlashlight )
				return;

			m_pFlashlight->TurnOn();
		}

		Vector vecForward, vecRight, vecUp;
		Vector position = EyePosition();
		
		// FIXME: En tercera persona el simple hecho de usar "LookupAttachment"
		// hace que el modelo se comporte de manera extraña.
		/*if ( ::input->CAM_IsThirdPerson() || ::input->CAM_IsThirdPersonOverShoulder() )
		{
			int iAttachment = LookupAttachment("anim_attachment_RH");

			if ( iAttachment >= 0 )
			{
				Vector vecOrigin;
				QAngle eyeAngles = EyeAngles();

				GetAttachment(iAttachment, vecOrigin, eyeAngles);

				Vector vForward;
				AngleVectors(eyeAngles, &vecForward, &vecRight, &vecUp);
				position = vecOrigin;
			}
			else
				EyeVectors(&vecForward, &vecRight, &vecUp);
		}
		else*/

		ConVarRef sv_flashlight_weapon("sv_flashlight_weapon");

		// El servidor quiere que las luces esten acopladas en las armas.
		if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() && !::input->CAM_IsThirdPerson() && !::input->CAM_IsThirdPersonOverShoulder() )
		{
			C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
			int iAttachment				= pWeapon->LookupAttachment("muzzle");

			// ¡Esta arma es válida!
			if ( iAttachment > 0 )
			{
				QAngle ang;
				pWeapon->GetAttachment(iAttachment, position, ang);

				AngleVectors(ang, &vecForward, &vecRight, &vecUp);
			}

			// No tiene el acoplamiento, usar la vista del usuario.
			else
				EyeVectors(&vecForward, &vecRight, &vecUp);
		}
		else
			EyeVectors(&vecForward, &vecRight, &vecUp);

		// Actualizamos la luz con la nueva dirección y velocidad.	
		m_pFlashlight->UpdateLight(position, vecForward, vecRight, vecUp, FLASHLIGHT_DISTANCE);
	}
	else if ( m_pFlashlight )
	{
		// Turned off the flashlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}
}

//=========================================================
// Libera el destello de la linterna.
//=========================================================
void C_IN_Player::ReleaseFlashlight()
{
	if ( m_pFlashlightBeam )
	{
		m_pFlashlightBeam->flags	= 0;
		m_pFlashlightBeam->die		= gpGlobals->curtime - 1;
		m_pFlashlightBeam			= NULL;
	}
}

//=========================================================
// Crea el cadaver en el lado del cliente.
//=========================================================
C_BaseAnimating *C_IN_Player::BecomeRagdollOnClient()
{
	// Iván: Creo que esto soluciona el BUG de los dobles cadaveres.
	return NULL;
}

//=========================================================
// Forma más fácil y corta de acceder a un comando del
// lado del cliente.
// ¡NO UTILIZAR CON COMANDOS DE SERVIDOR!
//=========================================================
const char *C_IN_Player::GetConVar(const char *pConVar)
{
	ConVarRef pVar(pConVar);

	if ( pVar.IsValid() )
		return pVar.GetString();
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL INVENTARIO
//=========================================================
//=========================================================

const char *pItemsNames[] =
{
	"", // 0
	"#Inventory_BloodKIT",
	"#Inventory_Bandage",
	"#Inventory_Battery",
	"#Inventory_HealthKIT",
	"#Inventory_HealthVial",
	"#Inventory_AmmoPistol",
	"#Inventory_AmmoPistolLarge",
	"#Inventory_AmmoSMG1",
	"#Inventory_AmmoSMG1Large",
	"#Inventory_AmmoAR2",
	"#Inventory_AmmoAR2Large",
	"#Inventory_Ammo357",
	"#Inventory_Ammo357Large",
	"#Inventory_AmmoCrossbow",
	"#Inventory_AmmoFlare",
	"#Inventory_AmmoFlareLarge",
	"#Inventory_AmmoRPG",
	"#Inventory_AmmoAR2Grenade",
	"#Inventory_AmmoSMG1Grenade",
	"#Inventory_AmmoBuckShot",
	"#Inventory_AmmoAR2AltFire",
};

const char *pItems[] = 
{
	"", // 0
	"item_bloodkit",
	"item_bandage",
	"item_battery",
	"item_healthkit",
	"item_healthvial",
	"item_ammo_pistol",
	"item_ammo_pistol_large",
	"item_ammo_smg1",
	"item_ammo_smg1_large",
	"item_ammo_ar2",
	"item_ammo_ar2_large",
	"item_ammo_357",
	"item_ammo_357_large",
	"item_ammo_crossbow",
	"item_flare_round",
	"item_box_flare_rounds",
	"item_rpg_round",
	"item_ar2_grenade",
	"item_ammo_smg1_grenade",
	"item_box_buckshot",
	"item_ammo_ar2_altfire",
};

//=========================================================
// Verifica si el jugador tiene un objeto con esta clase
// en su inventario.
//=========================================================
bool C_IN_Player::Inventory_HasItem(int pEntity, int pSection)
{
	// Bolsillo
	if ( pSection == INVENTORY_POCKET || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < ARRAYSIZE(m_HL2Local.PocketItems); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.PocketItems[i] == pEntity )
				return true;
		}
	}

	// Mochila
	if ( pSection == INVENTORY_POCKET || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < ARRAYSIZE(m_HL2Local.BackpackItems); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.BackpackItems[i] == pEntity )
				return true;
		}
	}

	return false;
}

//=========================================================
// Obtiene el nombre del objeto que se encuentra en esta posición.
//=========================================================
int C_IN_Player::Inventory_GetItem(int Position, int pSection)
{
	if ( pSection == INVENTORY_POCKET )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.PocketItems[Position] == 0 || !m_HL2Local.PocketItems[Position] )
			return 0;

		return m_HL2Local.PocketItems[Position];
	}

	if ( pSection == INVENTORY_BACKPACK )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.BackpackItems[Position] == 0 || !m_HL2Local.BackpackItems[Position] )
			return 0;

		return m_HL2Local.BackpackItems[Position];
	}

	return 0;
}

//=========================================================
// Obtiene el nombre de un objeto de una posición en el inventario.
//=========================================================
const char *C_IN_Player::Inventory_GetItemName(int Position, int pSection)
{
	// Obtenemos la ID de este objeto.
	int pEntity = Inventory_GetItem(Position, pSection);

	// No hay ningún objeto con esta ID en la lista.
	if ( pItemsNames[pEntity] == "" || !pItemsNames[pEntity] )
		return "";

	return pItemsNames[pEntity];
}

//=========================================================
// Obtiene el nombre clase de un objeto de una posición en el inventario.
//=========================================================
const char *C_IN_Player::Inventory_GetItemClassName(int Position, int pSection)
{
	// Obtenemos la ID de este objeto.
	int pEntity = Inventory_GetItem(Position, pSection);

	// No hay ningún objeto con esta ID en la lista.
	if ( pItems[pEntity] == "" || !pItems[pEntity] )
		return "";

	return pItems[pEntity];
}

int C_IN_Player::Inventory_GetItemCount(int pSection)
{
	int pTotal = 0;

	if ( pSection == INVENTORY_POCKET || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < 100; i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.PocketItems[i] == 0 || !m_HL2Local.PocketItems[i] )
				continue;

			pTotal++;
		}
	}

	if ( pSection == INVENTORY_BACKPACK || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < 100; i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.BackpackItems[i] == 0 || !m_HL2Local.BackpackItems[i] )
				continue;

			pTotal++;
		}
	}

	return pTotal;
}