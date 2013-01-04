//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "Sprite.h"
#include "grenade_satchel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar    sk_plr_dmg_satchel("sk_plr_dmg_satchel", "0");
ConVar    sk_npc_dmg_satchel("sk_npc_dmg_satchel", "0");
ConVar    sk_satchel_radius("sk_satchel_radius", "0");

//=========================================================
// Configuración del Arma
//=========================================================

#define MODEL_BASE		"models/weapons/w_slam.mdl"
#define	SPRITE			"sprites/redglow1.vmt"

//=========================================================
// Guardado y definición de datos
//=========================================================

BEGIN_DATADESC(CSatchelCharge)

	DEFINE_FIELD(m_flNextBounceSoundTime, FIELD_TIME),
	DEFINE_FIELD(m_bInAir, FIELD_BOOLEAN),
	DEFINE_FIELD(m_vLastPosition, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_pMyWeaponSLAM, FIELD_CLASSPTR),
	DEFINE_FIELD(m_bIsAttached, FIELD_BOOLEAN),

	// Function Pointers
	DEFINE_THINKFUNC(SatchelThink),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Explode", InputExplode),

END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_satchel, CSatchelCharge);

//=========================================================
// CSatchelCharge()
// Constructor
//=========================================================
CSatchelCharge::CSatchelCharge()
{
	m_vLastPosition.Init();
	m_pMyWeaponSLAM = NULL;
}

//=========================================================
// CSatchelCharge()
// Destructor
//=========================================================
CSatchelCharge::~CSatchelCharge()
{
	if (m_hGlowSprite != NULL)
	{
		UTIL_Remove(m_hGlowSprite);
		m_hGlowSprite = NULL;
	}
}

//=========================================================
// Deactivate()
// Desactivar y quitarlo del mundo.
//=========================================================
void CSatchelCharge::Deactivate()
{
	AddSolidFlags(FSOLID_NOT_SOLID);
	UTIL_Remove(this);

	if (m_hGlowSprite != NULL)
	{
		UTIL_Remove(m_hGlowSprite);
		m_hGlowSprite = NULL;
	}
}

//=========================================================
// Spawn()
// Crear un nuevo Satchel
//=========================================================
void CSatchelCharge::Spawn( void )
{
	Precache();

	// Modelo
	SetModel(MODEL_BASE);

	// Navegación
	VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false);
	SetMoveType(MOVETYPE_VPHYSICS);

	// Colisión
	SetCollisionGroup(COLLISION_GROUP_WEAPON);

	// Tamaño
	UTIL_SetSize(this, Vector(-6, -6, -2), Vector(6, 6, 2));

	SetThink(&CSatchelCharge::SatchelThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	// Salud, estado y daño.
	m_flDamage		= sk_plr_dmg_satchel.GetFloat();
	m_DmgRadius		= sk_satchel_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	// Gravedad, fricción y daño.
	SetGravity(UTIL_ScaleForGravity(560));
	SetFriction(1.0);
	SetSequence(1);
	SetDamage(150);

	// Más información
	m_bIsAttached			= false;
	m_bInAir				= true;
	m_flNextBounceSoundTime	= 0;
	m_vLastPosition			= vec3_origin;
	m_hGlowSprite			= NULL;

	CreateEffects();
}

//=========================================================
// Precache()
// Guardar objetos necesarios en caché.
//=========================================================
void CSatchelCharge::Precache()
{
	PrecacheModel(MODEL_BASE);
	PrecacheModel(SPRITE);
}

//=========================================================
// CreateEffects()
// Inicia y crea efectos para el modelo.
//=========================================================
void CSatchelCharge::CreateEffects()
{
	// Solo ejecutar una vez.
	if (m_hGlowSprite != NULL)
		return;

	// Create a blinking light to show we're an active SLAM
	m_hGlowSprite = CSprite::SpriteCreate(SPRITE, GetAbsOrigin(), false);
	m_hGlowSprite->SetAttachment(this, 0);
	m_hGlowSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxStrobeFast);
	m_hGlowSprite->SetBrightness(255, 1.0f);
	m_hGlowSprite->SetScale(0.2f, 0.5f);
	m_hGlowSprite->TurnOn();
}

//=========================================================
// InputExplode() - TIPO DE ENTRADA
// ¡Explotar!
//=========================================================
void CSatchelCharge::InputExplode(inputdata_t &inputdata)
{
	ExplosionCreate(GetAbsOrigin() + Vector( 0, 0, 16 ), GetAbsAngles(), GetThrower(), GetDamage(), 200, SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this);
	UTIL_Remove(this);
}

//=========================================================
// SatchelThink()
// Bucle de tareas para el objeto.
//=========================================================
void CSatchelCharge::SatchelThink()
{
	// Disminuir el tamaño del objeto si esta "colocado"
	if (m_bIsAttached)
		UTIL_SetSize(this, Vector( -2, -2, -6), Vector(2, 2, 6));

	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can shoot the satchel charge
	if (GetOwnerEntity())
	{
		trace_t tr;

		Vector	vUpABit		= GetAbsOrigin();
		vUpABit.z			+= 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity(NULL);

		UTIL_TraceEntity(this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr);

		if (tr.startsolid || tr.fraction != 1.0)
			SetOwnerEntity(saveOwner);
	}
	
	// Bounce movement code gets this think stuck occasionally so check if I've 
	// succeeded in moving, otherwise kill my motions.
	else if ((GetAbsOrigin() - m_vLastPosition).LengthSqr()<1)
	{
		SetAbsVelocity(vec3_origin);

		QAngle angVel	= GetLocalAngularVelocity();
		angVel.y		= 0;

		SetLocalAngularVelocity(angVel);

		SetThink(NULL);
		return;
	}

	m_vLastPosition	= GetAbsOrigin();

	StudioFrameAdvance();
	SetNextThink(gpGlobals->curtime + 0.1f);

	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	// Is it attached to a wall?
	if (m_bIsAttached)
		return;
}

//=========================================================
// BounceSound()
// 
//=========================================================
void CSatchelCharge::BounceSound()
{
	if (gpGlobals->curtime > m_flNextBounceSoundTime)
		m_flNextBounceSoundTime = gpGlobals->curtime + 0.1;
}