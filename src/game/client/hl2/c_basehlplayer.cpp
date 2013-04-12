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

ConVar cl_in_normal_death("cl_in_normal_death", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

IMPLEMENT_CLIENTCLASS_DT(C_BaseHLPlayer, DT_HL2_Player, CHL2_Player)
	RecvPropDataTable( RECVINFO_DT(m_HL2Local),0, &REFERENCE_RECV_TABLE(DT_HL2Local) ),
	RecvPropBool( RECVINFO( m_fIsSprinting ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
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

#if !defined (HL2MP) && !defined (PORTAL)
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

void C_BaseHLPlayer::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	// Muerte normal.
	if ( cl_in_normal_death.GetBool() )
		return BaseClass::CalcDeathCamView(eyeOrigin, eyeAngles, fov);

	// El ragdoll del jugador ha sido creado.
	if ( m_hRagdoll.Get() )
	{
		// Obtenemos la ubicación de los ojos del modelo.
		C_BaseHLRagdoll *pRagdoll = dynamic_cast<C_BaseHLRagdoll*>( m_hRagdoll.Get() );
		pRagdoll->GetAttachment(pRagdoll->LookupAttachment("eyes"), eyeOrigin, eyeAngles);

		// Ajustamos la camará en los ojos del modelo.
		Vector vForward;
		AngleVectors(eyeAngles, &vForward);

		trace_t tr;
		UTIL_TraceLine(eyeOrigin, eyeOrigin + ( vForward * 10000 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr);

		// ¡Oh! Al parecer los ojos chocan contra alguna pared.
		if ( (!(tr.fraction < 1) || (tr.endpos.DistTo(eyeOrigin) > 25)) )
			return;
	}
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
	InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );

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

IRagdoll* C_BaseHLPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_BaseHLRagdoll *pRagdoll = (C_BaseHLRagdoll*)m_hRagdoll.Get();
		return pRagdoll->GetIRagdoll();
	}

	return BaseClass::GetRepresentativeRagdoll();
}

//C_BaseHLRagdoll

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_BaseHLRagdoll, DT_HL2Ragdoll, CHL2Ragdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()

C_BaseHLRagdoll::C_BaseHLRagdoll()
{

}

C_BaseHLRagdoll::~C_BaseHLRagdoll()
{
	PhysCleanupFrictionSounds( this );

	if ( m_hPlayer )
	{
		m_hPlayer->CreateModelInstance();
	}
}

void C_BaseHLRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
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

void C_BaseHLRagdoll::AddEntity()
{
	BaseClass::AddEntity();

	// Tony; modified so third person can do the beam too.
	// TODO: Incorporar.
	/*
	if ( IsEffectActive( EF_DIMLIGHT ) )
	{
		//Tony; if local player, not in third person, and there's a beam, destroy it. It will get re-created if they go third person again.
		if ( this == C_BasePlayer::GetLocalPlayer() && !::input->CAM_IsThirdPerson() && m_pFlashlightBeam != NULL )
		{
			ReleaseFlashlight();
			return;
		}
		else if( this != C_BasePlayer::GetLocalPlayer() || ::input->CAM_IsThirdPerson() )
		{
			int iAttachment = LookupAttachment( "anim_attachment_RH" );

			if ( iAttachment < 0 )
				return;

			Vector vecOrigin;
			//Tony; EyeAngles will return proper whether it's local player or not.
			QAngle eyeAngles = EyeAngles();

			GetAttachment( iAttachment, vecOrigin, eyeAngles );

			Vector vForward;
			AngleVectors( eyeAngles, &vForward );

			trace_t tr;
			UTIL_TraceLine( vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if( !m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_nType = TE_BEAMPOINTS;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_pszModelName = "sprites/glow01.vmt";
				beamInfo.m_pszHaloName = "sprites/glow01.vmt";
				beamInfo.m_flHaloScale = 3.0;
				beamInfo.m_flWidth = 8.0f;
				beamInfo.m_flEndWidth = 35.0f;
				beamInfo.m_flFadeLength = 300.0f;
				beamInfo.m_flAmplitude = 0;
				beamInfo.m_flBrightness = 60.0;
				beamInfo.m_flSpeed = 0.0f;
				beamInfo.m_nStartFrame = 0.0;
				beamInfo.m_flFrameRate = 0.0;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beamInfo.m_nSegments = 8;
				beamInfo.m_bRenderable = true;
				beamInfo.m_flLife = 0.5;
				beamInfo.m_nFlags = FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;

				m_pFlashlightBeam = beams->CreateBeamPoints( beamInfo );
			}

			if( m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;

				beams->UpdateBeamInfo( m_pFlashlightBeam, beamInfo );

				//Tony; local players don't make the dlight.
				if( this != C_BasePlayer::GetLocalPlayer() )
				{
					dlight_t *el = effects->CL_AllocDlight( 0 );
					el->origin = tr.endpos;
					el->radius = 50; 
					el->color.r = 200;
					el->color.g = 200;
					el->color.b = 200;
					el->die = gpGlobals->curtime + 0.1;
				}
			}
		}
	}
	else if ( m_pFlashlightBeam )
	{
		ReleaseFlashlight();
	}
	*/
}

void C_BaseHLRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
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


void C_BaseHLRagdoll::CreateHL2Ragdoll( void )
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_BasePlayer *pPlayer = dynamic_cast< C_BasePlayer* >( m_hPlayer.Get() );
	
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{
			Interp_Copy( pPlayer );

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


void C_BaseHLRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateHL2Ragdoll();
	}
}

IRagdoll* C_BaseHLRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_BaseHLRagdoll::UpdateOnRemove( void )
{
	VPhysicsSetObject( NULL );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_BaseHLRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
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