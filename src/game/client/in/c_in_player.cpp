//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Funciones del jugador del lado de cliente.
//
// InfoSmart 2013. Todos los derechos reservados.
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
#include "in_buttons.h"

//#include "materialsystem/IMaterialProxy.h"
//#include "materialsystem/IMaterialVar.h"

#include "in_gamerules.h"

#include "c_basetempentity.h"

#if defined( CIN_Player )
	#undef CIN_Player	
#endif

void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

//=========================================================
// Definición de comandos de consola.
//=========================================================

// Modelo
static ConVar cl_playermodel("cl_playermodel",	"models/abigail.mdl",	FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Modelo del jugador");
ConVar cl_in_normal_death("cl_in_normal_death", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

//=========================================================
// Guardado y definición de datos
//=========================================================

#define FLASHLIGHT_DISTANCE			1000	// Distancia de la linterna.
#define FLASHLIGHT_OTHER_DISTANCE	300		// Distancia de la linterna de otro jugador.

#ifdef APOCALYPSE
	LINK_ENTITY_TO_CLASS(player, C_IN_Player);
#endif

BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_INLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_INNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_IN_Player, DT_IN_Player, CIN_Player)
	RecvPropDataTable( "inlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_INLocalPlayerExclusive) ),
	RecvPropDataTable( "innonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_INNonLocalPlayerExclusive) ),

	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropBool( RECVINFO( m_fIsWalking ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_IN_Player )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

//=========================================================
// Constructor
//=========================================================
C_IN_Player::C_IN_Player() : m_iv_angEyeAngles("C_IN_Player::m_iv_angEyeAngles")
{
	AddVar(&m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR);
	m_PlayerAnimState = CreatePlayerAnimationState(this);	// Creamos la instancia para la animación y el movimiento.

	m_blinkTimer.Invalidate();			// Cronometro para el parpadeo.

	m_pFlashlightBeam		= NULL;		// Eliminamos la luz de nuestra linterna.
	m_pFlashlightNonLocal	= NULL;		// Eliminamos la luz de nuestra linterna.
}

//=========================================================
// Destructor
//=========================================================
C_IN_Player::~C_IN_Player()
{
	ReleaseFlashlight();

	if ( m_PlayerAnimState )
		m_PlayerAnimState->Release();
}

//=========================================================
// Obtiene el jugador local convertido a C_IN_Player
//=========================================================
C_IN_Player* C_IN_Player::GetLocalINPlayer()
{
	return (C_IN_Player *)C_BasePlayer::GetLocalPlayer();
}

//=========================================================
// Pensamiento: Bucle de ejecución de tareas.
//=========================================================
void C_IN_Player::PreThink()
{
	BaseClass::PreThink();

	HandleSpeedChanges();

	if ( m_HL2Local.m_flSuitPower <= 0.0f )
	{
		if ( IsSprinting() )
			StopSprinting();
	}
}

//=========================================================
//=========================================================
void C_IN_Player::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	if ( state == SHOULDTRANSMIT_END )
	{
		if ( m_pFlashlightBeam != NULL )
			ReleaseFlashlight();
	}

	BaseClass::NotifyShouldTransmit(state);
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
// ¿Puede correr ahora mismo?
//=========================================================
bool C_IN_Player::CanSprint()
{
	return ( (!m_Local.m_bDucked && !m_Local.m_bDucking) && (GetWaterLevel() != 3) );
}

//=========================================================
// Empezar a correr
//=========================================================
void C_IN_Player::StartSprinting()
{
	if ( m_HL2Local.m_flSuitPower < 10 )
	{
		// Don't sprint unless there's a reasonable
		// amount of suit power.
		CPASAttenuationFilter filter(this);
		filter.UsePredictionRules();
		EmitSound(filter, entindex(), "HL2Player.SprintNoPower");

		return;
	}

	CPASAttenuationFilter filter(this);
	filter.UsePredictionRules();
	EmitSound(filter, entindex(), "HL2Player.SprintStart");

	SetMaxSpeed(CalculateSpeed());
	m_fIsSprinting = true;
}

//=========================================================
// Para de correr.
//=========================================================
void C_IN_Player::StopSprinting()
{
	ConVarRef hl2_walkspeed("hl2_walkspeed"); // Velocidad con traje de protección.
	ConVarRef hl2_normspeed("hl2_normspeed"); // Velocidad sin traje de protección.

	// Ajustar la velocidad dependiendo si tenemos el traje o no.
	if ( IsSuitEquipped() )
		SetMaxSpeed(CalculateSpeed(NULL, hl2_walkspeed.GetFloat()));
	else
		SetMaxSpeed(hl2_normspeed.GetFloat());

	m_fIsSprinting = false;
}

//=========================================================
// Controla los cambios de velocidad.
//=========================================================
void C_IN_Player::HandleSpeedChanges()
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	if ( buttonsChanged & IN_SPEED )
	{
		// The state of the sprint/run button has changed.
		if ( IsSuitEquipped() )
		{
			if ( !(m_afButtonPressed & IN_SPEED)  && IsSprinting() )
				StopSprinting();

			else if ( (m_afButtonPressed & IN_SPEED) && !IsSprinting() )
			{
				if ( CanSprint() )
					StartSprinting();

				// Reset key, so it will be activated post whatever is suppressing it.
				else
					m_nButtons &= ~IN_SPEED;
			}
		}
	}
	else if( buttonsChanged & IN_WALK )
	{
		if ( IsSuitEquipped() )
		{
			// The state of the WALK button has changed. 
			if( IsWalking() && !(m_afButtonPressed & IN_WALK) )
				StopWalking();

			else if( !IsWalking() && !IsSprinting() && (m_afButtonPressed & IN_WALK) && !(m_nButtons & IN_DUCK) )
				StartWalking();
		}
	}

	if ( IsSuitEquipped() && m_fIsWalking && !(m_nButtons & IN_WALK)  ) 
		StopWalking();
}

//=========================================================
// Empezar a caminar
//=========================================================
void C_IN_Player::StartWalking()
{
	SetMaxSpeed(CalculateSpeed());
	m_fIsWalking = true;
}

//=========================================================
// Paramos de caminar
//=========================================================
void C_IN_Player::StopWalking( void )
{
	SetMaxSpeed(CalculateSpeed());
	m_fIsWalking = false;
}

//=========================================================
//=========================================================
void C_IN_Player::ItemPreFrame()
{
	if ( GetFlags() & FL_FROZEN )
		 return;

	BaseClass::ItemPreFrame();

}

//=========================================================
//=========================================================
void C_IN_Player::ItemPostFrame()
{
	if ( GetFlags() & FL_FROZEN )
		 return;

	BaseClass::ItemPostFrame();
}


ConVar cl_render_model("cl_render_model", "0", 0);
ConVar cl_render_shift("cl_render_shift", "-20", 0);

//=========================================================
// Devuelve si es posible dibujar el modelo en 3 persona.
//=========================================================
bool C_IN_Player::ShouldDraw()
{
	// Esta vivo.
	if ( IsAlive() && cl_render_model.GetBool() )
		return true;

	// 
	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	// Es el jugador actual y esta muerto (como cadaver)
	if ( IsLocalPlayer() && IsRagdoll() )
		return true;
	
	// Es un cadaver.
	if ( IsRagdoll() )
		return false;

	return BaseClass::ShouldDraw();
}


const Vector& C_IN_Player::GetRenderOrigin()
{
	//if ( view->GetCurrentlyDrawingEntity() != this )

	if ( IsRagdoll() )
		return m_pRagdoll->GetRagdollOrigin();

	// Get the forward vector
	static Vector forward; // static because this method returns a reference
	AngleVectors(GetRenderAngles(), &forward);
 
	// Shift the render origin by a fixed amount
	forward *= cl_render_shift.GetFloat();
	forward += GetAbsOrigin();
 
	return forward;
}

int C_IN_Player::DrawModel(int flags)
{
	return BaseClass::DrawModel(flags);
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
// Devuelve los angulos de los ojos sea del jugador local o
// de otro jugador.
//=========================================================
const QAngle &C_IN_Player::EyeAngles()
{
	if ( IsLocalPlayer() )
		return BaseClass::EyeAngles();
	
	return m_angEyeAngles;
}

//=========================================================
// Devuelve 
//=========================================================
const QAngle &C_IN_Player::GetRenderAngles()
{
	if ( IsRagdoll() )
		return vec3_angle;
	else
		return m_PlayerAnimState->GetRenderAngles();
}

//=========================================================
// Actualiza las animaciones del lado del cliente.
//=========================================================
void C_IN_Player::UpdateClientSideAnimation()
{
	m_PlayerAnimState->Update(EyeAngles()[YAW], EyeAngles()[PITCH]);
	
	// Es hora de parpadear.
	if ( m_blinkTimer.IsElapsed() )
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start(RandomFloat(1.5f, 4.0f));
	}

	BaseClass::UpdateClientSideAnimation();
}

//=========================================================
// Realiza un evento de animación.
//=========================================================
void C_IN_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent(event, nData);
}

//=========================================================
// Decide si el jugador puede recibir luz de las linternas.
//=========================================================
bool C_IN_Player::ShouldReceiveProjectedTextures(int flags)
{
	Assert(flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK);

	if ( IsEffectActive(EF_NODRAW) )
		 return false;

	if ( flags & SHADOW_FLAGS_FLASHLIGHT )
		return true;

	return BaseClass::ShouldReceiveProjectedTextures(flags);
}

void C_IN_Player::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	// Muerte normal.
	if ( cl_in_normal_death.GetBool() || InGameRules()->IsMultiplayer() )
		return BaseClass::CalcDeathCamView(eyeOrigin, eyeAngles, fov);

	// El ragdoll del jugador ha sido creado.
	if ( m_hRagdoll.Get() )
	{
		// Obtenemos la ubicación de los ojos del modelo.
		C_INRagdoll *pRagdoll = dynamic_cast<C_INRagdoll*>(m_hRagdoll.Get());
		pRagdoll->GetAttachment(pRagdoll->LookupAttachment("eyes"), eyeOrigin, eyeAngles);

		// Ajustamos la camará en los ojos del modelo.
		Vector vForward;
		AngleVectors(eyeAngles, &vForward);

		trace_t tr;
		UTIL_TraceLine(eyeOrigin, eyeOrigin + ( vForward * 10000 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr);

		// ¡Oh! Al parecer los ojos chocan contra alguna pared.
		//if ( tr.fraction < 1 || tr.endpos.DistTo(eyeOrigin) <= 25 )
			//CalcThirdPersonDeathView(eyeOrigin, eyeAngles, fov);
	}
}

//=========================================================
// Tipo de sombra.
//=========================================================
ShadowType_t C_IN_Player::ShadowCastType()
{
	if ( !IsVisible() )
		 return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

//=========================================================
// Cuando se actualiza un modelo.
//=========================================================
CStudioHdr *C_IN_Player::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	InitializePoseParams();

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
		m_PlayerAnimState->OnNewModel();

	return hdr;
}

//=========================================================
// Inicializa
//=========================================================
void C_IN_Player::InitializePoseParams()
{
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );

	CStudioHdr *hdr = GetModelPtr();

	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
		SetPoseParameter( hdr, i, 0.0 );
}

//=========================================================
// Agrega la entidad.
//=========================================================
void C_IN_Player::AddEntity()
{
	BaseClass::AddEntity();

	// Linterna encendida
	// y no somos nosotros mismos (otro jugador)
	if ( IsEffectActive(EF_DIMLIGHT) && this != C_BasePlayer::GetLocalPlayer() )
	{
		ConVarRef sv_flashlight_weapon("sv_flashlight_weapon");
		ConVarRef sv_flashlight_realistic("sv_flashlight_realistic");

		Vector vecForward, vecRight, vecUp;
		Vector position = EyePosition();

		int iAttachment = 0;

		// El servidor quiere que las luces esten acopladas en las armas.
		if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() )
			iAttachment = GetActiveWeapon()->LookupAttachment("muzzle");
		else
			iAttachment = LookupAttachment("eyes");
		
		// ¡Es un acoplamiento válido!
		if ( iAttachment >= 0 )
		{
			Vector vecOrigin;
			QAngle eyeAngles = EyeAngles();

			if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() )
				GetActiveWeapon()->GetAttachment(iAttachment, vecOrigin, eyeAngles);
			else
				GetAttachment(iAttachment, vecOrigin, eyeAngles);

			Vector vForward;
			AngleVectors(eyeAngles, &vecForward, &vecRight, &vecUp);
			position = vecOrigin;
		}
		else
			EyeVectors(&vecForward, &vecRight, &vecUp);

		// Luz realistica: Un projectedtexture capaz de iluminar realmente.
		// Limitaciones: Calcula las sombras dinamicas en tiempo real ¡mucho cpu! y esta limitado el numero de luces por el motor.
		if ( sv_flashlight_realistic.GetBool() )
		{
			// No ha sido creado el efecto de luz, intentamos crearlo.
			if ( !m_pFlashlightNonLocal )
			{
				// Turned on the headlight; create it.
				m_pFlashlightNonLocal = new CFlashlightEffect(index);

				if ( !m_pFlashlightNonLocal )
					return;

				m_pFlashlightNonLocal->TurnOn();
			}

			// Actualizamos la luz con la nueva dirección y velocidad.	
			m_pFlashlightNonLocal->UpdateLight(position, vecForward, vecRight, vecUp, FLASHLIGHT_OTHER_DISTANCE);
		}

		// Destello: Capaz de crear un efecto de linterna cuya iluminación es muy baja.
		else
		{
			trace_t tr;
			UTIL_TraceLine(position, position + (vecForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

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
			}
		}
	}
	else
	{
		if ( this != C_BasePlayer::GetLocalPlayer() )
			ReleaseFlashlight();
	}
}

//=========================================================
// Actualiza la ubicación de la luz de la linterna.
//=========================================================
void C_IN_Player::UpdateFlashlight()
{
	// Linterna encendida.
	if ( IsEffectActive(EF_DIMLIGHT) )
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

		ConVarRef sv_flashlight_weapon("sv_flashlight_weapon");
		
		// Estamos en tercera persona.
		if ( ::input->CAM_IsThirdPerson() || ::input->CAM_IsThirdPersonOverShoulder() )
		{
			int iAttachment = 0;

			// El servidor quiere que las luces esten acopladas en las armas.
			if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() )
				iAttachment = GetActiveWeapon()->LookupAttachment("muzzle");
			else
				iAttachment = LookupAttachment("eyes");

			if ( iAttachment >= 0 )
			{
				Vector vecOrigin;
				QAngle eyeAngles = EyeAngles();

				if ( sv_flashlight_weapon.GetBool() && GetActiveWeapon() )
					GetActiveWeapon()->GetAttachment(iAttachment, vecOrigin, eyeAngles);
				else
					GetAttachment(iAttachment, vecOrigin, eyeAngles);

				Vector vForward;
				AngleVectors(eyeAngles, &vecForward, &vecRight, &vecUp);
				position = vecOrigin;
			}
			else
				EyeVectors(&vecForward, &vecRight, &vecUp);
		}

		// Estamos en primera persona.
		else
		{
			// El servidor quiere que las luces esten acopladas en las armas.
			if ( sv_flashlight_weapon.GetBool() && GetViewModel() )
			{
				C_BaseViewModel *pView		= GetViewModel();
				int iAttachment				= pView->LookupAttachment("muzzle");

				// ¡Esta arma es válida!
				if ( iAttachment > 0 )
				{
					QAngle ang;
					pView->GetAttachment(iAttachment, position, ang);
					AngleVectors(ang, &vecForward, &vecRight, &vecUp);
				}

				// No tiene el acoplamiento, usar la vista del usuario.
				else
					EyeVectors(&vecForward, &vecRight, &vecUp);
			}
			else
				EyeVectors(&vecForward, &vecRight, &vecUp);
		}

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
		m_pFlashlightBeam->flags		= 0;
		m_pFlashlightBeam->die		= gpGlobals->curtime - 1;
		m_pFlashlightBeam			= NULL;
	}

	if ( m_pFlashlightNonLocal )
	{
		delete m_pFlashlightNonLocal;
		m_pFlashlightNonLocal = NULL;
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

	return "";
}

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEPlayerAnimEvent, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate(DataUpdateType_t updateType)
	{
		// Create the effect.
		C_IN_Player *pPlayer = dynamic_cast<C_IN_Player*>(m_hPlayer.Get());

		if ( pPlayer && !pPlayer->IsDormant() )
			pPlayer->DoAnimationEvent((PlayerAnimEvent_t)m_iEvent.Get(), m_nData);
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent);

BEGIN_RECV_TABLE_NOBASE(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent)
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()

// INRAGDOLL
IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_INRagdoll, DT_INRagdoll, CINRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()

C_INRagdoll::C_INRagdoll()
{

}

C_INRagdoll::~C_INRagdoll()
{
	PhysCleanupFrictionSounds(this);

	if ( m_hPlayer )
		m_hPlayer->CreateModelInstance();
}

void C_INRagdoll::Interp_Copy(C_BaseAnimatingOverlay *pSourceEntity)
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

void C_INRagdoll::ImpactTrace(trace_t *pTrace, int iDamageType, char *pCustomImpactName)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght
				
		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		// Blood spray!
//		FX_CS_BloodSpray( hitpos, dir, 10 );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}


void C_INRagdoll::CreateINRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_IN_Player *pPlayer = dynamic_cast<C_IN_Player*>(m_hPlayer.Get());
	
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance(this);

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());

		if ( bRemotePlayer )
		{
			Interp_Copy(pPlayer);

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );
			
			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = pPlayer->GetSequence();
			if ( iSeq == -1 )
			{
				Assert( false );	// missing walk_lower?
				iSeq = 0;
			}
			
			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
		
	}

	SetModelIndex( m_nModelIndex );

	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	else
	{
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}

	InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
}


void C_INRagdoll::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateINRagdoll();
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}

	UpdateVisibility();
}

IRagdoll* C_INRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_INRagdoll::UpdateOnRemove( void )
{
	VPhysicsSetObject( NULL );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_INRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );

	static float destweight[128];
	static bool bIsInited = false;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	int nFlexDescCount = hdr->numflexdesc();
	if ( nFlexDescCount )
	{
		Assert( !pFlexDelayedWeights );
		memset( pFlexWeights, 0, nFlexWeightCount * sizeof(float) );
	}

	if ( m_iEyeAttachment > 0 )
	{
		matrix3x4_t attToWorld;
		if (GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
		}
	}
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
	"#Inventory_EmptyBloodKIT",
	"#Inventory_Soda",
	"#Inventory_EmptySoda",
	"#Inventory_Food",
	"#Inventory_EmptyFood",
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
	"item_empty_bloodkit",
	"item_soda",
	"item_empty_soda",
	"item_food",
	"item_empty_food",
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
	return Inventory_GetItemNameByID(pEntity);
}

//=========================================================
// Obtiene el nombre de un objeto por su ID.
//=========================================================
const char *C_IN_Player::Inventory_GetItemNameByID(int pEntity)
{
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
	return Inventory_GetItemClassNameByID(pEntity);
}

//=========================================================
// Obtiene el nombre clase de un objeto por su ID.
//=========================================================
const char *C_IN_Player::Inventory_GetItemClassNameByID(int pEntity)
{
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