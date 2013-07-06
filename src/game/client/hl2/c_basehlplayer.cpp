//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "playerandobjectenumerator.h"
#include "engine/ivdebugoverlay.h"
#include "c_ai_basenpc.h"
#include "in_buttons.h"
#include "collisionutils.h"
#include "iinput.h"

#include "hud_macros.h"

#include "flashlighteffect.h"
#include "input.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "r_efx.h"

#include "tier0/memdbgon.h"

#define AVOID_SPEED 2000.0f

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

extern ConVar zoom_sensitivity_ratio;
extern ConVar default_fov;
extern ConVar sensitivity;

ConVar cl_npc_speedmod_intime("cl_npc_speedmod_intime",		"0.25", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
ConVar cl_npc_speedmod_outtime("cl_npc_speedmod_outtime",	"1.5",	FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

IMPLEMENT_CLIENTCLASS_DT(C_BaseHLPlayer, DT_HL2_Player, CHL2_Player)
	RecvPropDataTable( RECVINFO_DT(m_HL2Local),0, &REFERENCE_RECV_TABLE(DT_HL2Local) ),
	RecvPropBool( RECVINFO( m_fIsSprinting ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_BaseHLPlayer)
	DEFINE_PRED_TYPEDESCRIPTION(m_HL2Local, C_HL2PlayerLocalData),
	DEFINE_PRED_FIELD(m_fIsSprinting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()

/*----------------------------------------------------
	CC_DropPrimary()
	Tirar el arma primaria.
----------------------------------------------------*/
void CC_DropPrimary(void)
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) C_BasePlayer::GetLocalPlayer();
	
	if (pPlayer == NULL)
		return;

	pPlayer->Weapon_DropPrimary();
}

static ConCommand dropprimary("dropprimary", CC_DropPrimary, "dropprimary: Drops the primary weapon of the player.");

#if !defined (HL2MP) && !defined (PORTAL) && !defined(INSOURCE)
	LINK_ENTITY_TO_CLASS(player, C_BaseHLPlayer);
#endif


/*----------------------------------------------------
	C_BaseHLPlayer
	Constructor primario.
----------------------------------------------------*/
C_BaseHLPlayer::C_BaseHLPlayer()
{
	AddVar(&m_Local.m_vecPunchAngle, &m_Local.m_iv_vecPunchAngle, LATCH_SIMULATION_VAR);
	AddVar(&m_Local.m_vecPunchAngleVel, &m_Local.m_iv_vecPunchAngleVel, LATCH_SIMULATION_VAR);

	m_flZoomStart		= 0.0f;
	m_flZoomEnd			= 0.0f;
	m_flZoomRate		= 0.0f;
	m_flZoomStartTime	= 0.0f;
	m_flSpeedMod		= cl_forwardspeed.GetFloat();

	ConVarRef scissor("r_flashlightscissor");
	scissor.SetValue("0");
}

/*----------------------------------------------------
	OnDataChanged()
	Cuando información cambia.
----------------------------------------------------*/
void C_BaseHLPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	if (updateType == DATA_UPDATE_CREATED)
		SetNextClientThink(CLIENT_THINK_ALWAYS);

	BaseClass::OnDataChanged( updateType );
}

/*----------------------------------------------------
	Weapon_DropPrimary()
	Tirar el arma primaria.
----------------------------------------------------*/
void C_BaseHLPlayer::Weapon_DropPrimary( void )
{
	engine->ServerCmd("DropPrimary");
}

/*----------------------------------------------------
	GetFOV()
	Obtener el FOV adecuado.
----------------------------------------------------*/
float C_BaseHLPlayer::GetFOV()
{
	float flFOVOffset	= BaseClass::GetFOV() + GetZoom();
	int min_fov			= (gpGlobals->maxClients == 1) ? 5 : default_fov.GetInt();
	
	flFOVOffset = max(min_fov, flFOVOffset);
	return flFOVOffset;
}

/*----------------------------------------------------
	GetZoom()
	Obtener el Zoom adecuado.
----------------------------------------------------*/
float C_BaseHLPlayer::GetZoom( void )
{
	float fFOV = m_flZoomEnd;

	if ((m_flZoomStart != m_flZoomEnd) && (m_flZoomRate > 0.0f))
	{
		float deltaTime = (float)(gpGlobals->curtime - m_flZoomStartTime) / m_flZoomRate;

		if (deltaTime >= 1.0f)
			fFOV = m_flZoomStart = m_flZoomEnd;
		else
			fFOV = SimpleSplineRemapVal(deltaTime, 0.0f, 1.0f, m_flZoomStart, m_flZoomEnd);
	}

	return fFOV;
}

/*----------------------------------------------------
	Zoom()
	Ajustar el Zoom
----------------------------------------------------*/
void C_BaseHLPlayer::Zoom(float FOVOffset, float time)
{
	m_flZoomStart		= GetZoom();
	m_flZoomEnd			= FOVOffset;
	m_flZoomRate		= time;
	m_flZoomStartTime	= gpGlobals->curtime;
}

/*----------------------------------------------------
	DrawModel()
	Dibujar el modelo del usuario.
----------------------------------------------------*/
int C_BaseHLPlayer::DrawModel(int flags)
{
	QAngle saveAngles	= GetLocalAngles();
	QAngle useAngles	= saveAngles;

	useAngles[PITCH] = 0.0f;

	SetLocalAngles(useAngles);

	int iret = BaseClass::DrawModel(flags);

	SetLocalAngles(saveAngles);

	return iret;
}

/*----------------------------------------------------
	ExitLadder()
	Salir de la escalera.
----------------------------------------------------*/
void C_BaseHLPlayer::ExitLadder()
{
	if (MOVETYPE_LADDER != GetMoveType())
		return;
	
	SetMoveType(MOVETYPE_WALK);
	SetMoveCollide(MOVECOLLIDE_DEFAULT);

	m_HL2Local.m_hLadder = NULL;
}

/*----------------------------------------------------
	TestMove()
	Verificar si un usuario puede moverse a cierto objetivo.
----------------------------------------------------*/
bool C_BaseHLPlayer::TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir )
{
	trace_t trUp;
	trace_t trOver;
	trace_t trDown;
	float flHit1, flHit2;
	
	UTIL_TraceHull(GetAbsOrigin(), pos, GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver);

	if (trOver.fraction < 1.0f)
	{
		if ( objDir.IsZero() ||
			( IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && 
			( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ) )
			)
		{
			UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, m_Local.m_flStepSize ), 
				GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trUp);

			UTIL_TraceHull(trUp.endpos, pos + Vector( 0, 0, trUp.endpos.z - trUp.startpos.z ), 
				GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver);

			if (trOver.fraction < 1.0f)
			{
				if (objDir.IsZero() ||
					IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && ( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ))
					return false;
			}
		}
	}

	UTIL_TraceLine(trOver.endpos, trOver.endpos - Vector( 0, 0, fVertDist ), 
		MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trDown);

	if (trDown.fraction == 1.0f) 
		return false;

	return true;
}

/*----------------------------------------------------
	PerformClientSideObstacleAvoidance()
	Evadir obstaculos.
----------------------------------------------------*/
void C_BaseHLPlayer::PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd )
{
	switch (GetMoveType())
	{
		case MOVETYPE_NOCLIP:
		case MOVETYPE_NONE:
		case MOVETYPE_OBSERVER:
			return;
		default:
		break;
	}

	// Calcular y mantenerse alejado de todo jugador u objeto no atravesable.
	Vector size		= WorldAlignSize();

	float radius	= 0.7f * sqrt( size.x * size.x + size.y * size.y );
	float curspeed	= GetLocalVelocity().Length2D();

	// Si estamos corriendo hacer más grande el radio.
	float factor = 1.0f;

	if (curspeed > 150.0f)
	{
		curspeed	= min(2048.0f, curspeed);
		factor		= (1.0f + ( curspeed - 150.0f ) / 150.0f);

		radius		= radius * factor;
	}

	Vector currentdir;
	Vector rightdir;

	QAngle vAngles	= pCmd->viewangles;
	vAngles.x		= 0;

	AngleVectors(vAngles, &currentdir, &rightdir, NULL);
		
	bool istryingtomove		= false;
	bool ismovingforward	= false;

	if (fabs( pCmd->forwardmove ) > 0.0f || 
		fabs( pCmd->sidemove ) > 0.0f)
	{
		istryingtomove = true;

		if (pCmd->forwardmove > 1.0f)
			ismovingforward = true;
	}

	if (istryingtomove == true)
		 radius *= 1.3f;

	CPlayerAndObjectEnumerator avoid(radius);
	partition->EnumerateElementsInSphere(PARTITION_CLIENT_SOLID_EDICTS, GetAbsOrigin(), radius, false, &avoid);

	int c = avoid.GetObjectCount();

	if (c <= 0)
		return;

	float adjustforwardmove = 0.0f;
	float adjustsidemove	= 0.0f;

	for (int i = 0; i < c; i++)
	{
		C_AI_BaseNPC *obj = dynamic_cast< C_AI_BaseNPC *>(avoid.GetObject( i ));

		if(!obj)
			continue;

		Vector vecToObject	= obj->GetAbsOrigin() - GetAbsOrigin();
		float flDist		= vecToObject.Length2D();
		
		Vector vecWorldMins, vecWorldMaxs;
		obj->CollisionProp()->WorldSpaceAABB(&vecWorldMins, &vecWorldMaxs);

		Vector objSize		= vecWorldMaxs - vecWorldMins;
		float objectradius	= 0.5f * sqrt(objSize.x * objSize.x + objSize.y * objSize.y);

		if (!obj->IsMoving() && flDist > objectradius)
			  continue;

		if (flDist > objectradius && obj->IsEffectActive(EF_NODRAW))
			obj->RemoveEffects(EF_NODRAW);

		Vector vecNPCVelocity;
		obj->EstimateAbsVelocity(vecNPCVelocity);
		float flNPCSpeed = VectorNormalize(vecNPCVelocity);

		Vector vPlayerVel = GetAbsVelocity();
		VectorNormalize(vPlayerVel);

		float flHit1, flHit2;
		Vector vRayDir = vecToObject;
		VectorNormalize(vRayDir);

		float flVelProduct = DotProduct(vecNPCVelocity, vPlayerVel);
		float flDirProduct = DotProduct(vRayDir, vPlayerVel);

		if (!IntersectInfiniteRayWithSphere(
				GetAbsOrigin(),
				vRayDir,
				obj->GetAbsOrigin(),
				radius,
				&flHit1,
				&flHit2))
			continue;

        Vector dirToObject = -vecToObject;
		VectorNormalize(dirToObject);

		float fwd	= 0;
		float rt	= 0;

		float sidescale		= 2.0f;
		float forwardscale	= 1.0f;
		bool foundResult	= false;
		Vector vMoveDir		= vecNPCVelocity;

		if (flNPCSpeed > 0.001f)
		{
			Vector vecNPCTrajectoryRight = CrossProduct(vecNPCVelocity, Vector( 0, 0, 1));

			int iDirection = (vecNPCTrajectoryRight.Dot(dirToObject) > 0) ? 1 : -1;

			for (int nTries = 0; nTries < 2; nTries++)
			{
				Vector vecTryMove		= vecNPCTrajectoryRight * iDirection;
				VectorNormalize(vecTryMove);
				
				Vector vTestPosition	= GetAbsOrigin() + vecTryMove * radius * 2;

				if (TestMove(vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir))
				{
					fwd = currentdir.Dot(vecTryMove);
					rt	= rightdir.Dot(vecTryMove);
					
					foundResult = true;
					break;
				}
				else
					iDirection *= -1;
			}
		}
		else
		{
			Vector vecNPCForward;
			obj->GetVectors(&vecNPCForward, NULL, NULL);
			
			Vector vTestPosition = GetAbsOrigin() - vecNPCForward * radius * 2;

			if (TestMove(vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir))
			{
				fwd = currentdir.Dot(-vecNPCForward);
				rt	= rightdir.Dot(-vecNPCForward);

				if (flDist < objectradius)
					obj->AddEffects(EF_NODRAW);

				foundResult = true;
			}
		}

		if (!foundResult)
		{
			Vector vTestPosition = GetAbsOrigin() + vMoveDir * radius * 2;

			if (TestMove(vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir))
			{
				fwd = currentdir.Dot( vMoveDir );
				rt	= rightdir.Dot( vMoveDir );

				if (flDist < objectradius)
					obj->AddEffects(EF_NODRAW);

				foundResult = true;
			}
			else
			{
				Vector vTestPosition = GetAbsOrigin() - dirToObject * radius * 2;

				if (TestMove(vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir))
				{
					fwd = currentdir.Dot(-dirToObject);
					rt	= rightdir.Dot(-dirToObject);

					foundResult = true;
				}
			}
		}

		if (!foundResult)
		{
			Vector vTestPosition = GetAbsOrigin() - vMoveDir * radius * 2;

			fwd = currentdir.Dot(-vMoveDir);
			rt	= rightdir.Dot(-vMoveDir);

			if (flDist < objectradius)
				obj->AddEffects( EF_NODRAW );

			foundResult = true;
		}

		if (istryingtomove)
			sidescale = 6.0f;

		if (flVelProduct > 0.0f && flDirProduct > 0.0f)
			sidescale = 0.1f;

		float force		= 1.0f;
		float forward	= forwardscale * fwd * force * AVOID_SPEED;
		float side		= sidescale * rt * force * AVOID_SPEED;

		adjustforwardmove	+= forward;
		adjustsidemove		+= side;
	}

	pCmd->forwardmove	+= adjustforwardmove;
	pCmd->sidemove		+= adjustsidemove;
	
	float flForwardScale = 1.0f;

	if (pCmd->forwardmove > fabs(cl_forwardspeed.GetFloat()))
		flForwardScale = fabs(cl_forwardspeed.GetFloat()) / pCmd->forwardmove;

	else if (pCmd->forwardmove < -fabs( cl_backspeed.GetFloat()))
		flForwardScale = fabs(cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove);
	
	float flSideScale = 1.0f;

	if (fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat()))
		flSideScale = fabs(cl_sidespeed.GetFloat()) / fabs(pCmd->sidemove);
	
	float flScale		= min(flForwardScale, flSideScale);
	pCmd->forwardmove	*= flScale;
	pCmd->sidemove		*= flScale;
}

/*----------------------------------------------------
	PerformClientSideNPCSpeedModifiers()
----------------------------------------------------*/
void C_BaseHLPlayer::PerformClientSideNPCSpeedModifiers(float flFrameTime, CUserCmd *pCmd)
{
	if (m_hClosestNPC == NULL)
	{
		if (m_flSpeedMod != cl_forwardspeed.GetFloat())
		{
			float flDeltaTime = (m_flSpeedModTime - gpGlobals->curtime);
			m_flSpeedMod = RemapValClamped( flDeltaTime, cl_npc_speedmod_outtime.GetFloat(), 0, m_flExitSpeedMod, cl_forwardspeed.GetFloat() );
		}
	}
	else
	{
		C_AI_BaseNPC *pNPC = dynamic_cast< C_AI_BaseNPC *>( m_hClosestNPC.Get() );

		if (pNPC)
		{
			float flDist			= (GetAbsOrigin() - pNPC->GetAbsOrigin()).LengthSqr();
			bool bShouldModSpeed	= false; 

			if (flDist < pNPC->GetSpeedModifyRadius())
			{
				Vector vecTargetOrigin	= pNPC->GetAbsOrigin();
				Vector los				= (vecTargetOrigin - EyePosition());

				los.z = 0;

				VectorNormalize(los);
				Vector facingDir;
				AngleVectors(GetAbsAngles(), &facingDir);

				float flDot = DotProduct(los, facingDir);

				if (flDot > 0.8)
					bShouldModSpeed = true;
			}

			if (!bShouldModSpeed)
			{
				m_hClosestNPC		= NULL;
				m_flSpeedModTime	= gpGlobals->curtime + cl_npc_speedmod_outtime.GetFloat();
				m_flExitSpeedMod	= m_flSpeedMod;

				return;
			}
			else 
			{
				if (m_flSpeedMod != pNPC->GetSpeedModifySpeed())
				{
					float flDeltaTime	= (m_flSpeedModTime - gpGlobals->curtime);
					m_flSpeedMod		= RemapValClamped(flDeltaTime, cl_npc_speedmod_intime.GetFloat(), 0, cl_forwardspeed.GetFloat(), pNPC->GetSpeedModifySpeed());
				}
			}
		}
	}

	if (pCmd->forwardmove > 0.0f)
		pCmd->forwardmove = clamp(pCmd->forwardmove, -m_flSpeedMod, m_flSpeedMod);
	else
		pCmd->forwardmove = clamp(pCmd->forwardmove, -m_flSpeedMod, m_flSpeedMod);
	
	pCmd->sidemove = clamp(pCmd->sidemove, -m_flSpeedMod, m_flSpeedMod);
}

/*----------------------------------------------------
	CreateMove()
----------------------------------------------------*/
bool C_BaseHLPlayer::CreateMove(float flInputSampleTime, CUserCmd *pCmd)
{
	bool bResult = BaseClass::CreateMove(flInputSampleTime, pCmd);	

	if (!IsInAVehicle())
	{
		PerformClientSideObstacleAvoidance(TICK_INTERVAL, pCmd);
		PerformClientSideNPCSpeedModifiers(TICK_INTERVAL, pCmd);
	}

	return bResult;
}

void C_BaseHLPlayer::CalcThirdPersonDeathView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// El jugador no esta preparado.
    if ( !pPlayer )
		return;

	// Guardamos una copia del modelo actual del jugador.
    pPlayer->SnatchModelInstance(this);

	// Creates the ragdoll asset from the player's model.
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if ( pPlayer && !pPlayer->IsDormant() )
		pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	else
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );

	// Ajustamos el ragdoll.
	InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);

	// Calculamos el origen del ragdoll.
	IRagdoll *pRagdoll	= GetRepresentativeRagdoll();
	Vector origin		= EyePosition();

	if ( pRagdoll )
	{
		origin = pRagdoll->GetRagdollOrigin();
		origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
	}

	eyeOrigin = origin;
   
	Vector vForward;
	AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );
	VectorMA( origin, -CHASE_CAM_DISTANCE, vForward, eyeOrigin );

	Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
	Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if (trace.fraction < 1.0)
		eyeOrigin = trace.endpos;

	fov = GetFOV();
}