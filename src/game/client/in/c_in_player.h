//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Funciones del jugador del lado de cliente.
//
// InfoSmart 2013. Todos los derechos reservados.
//====================================================================================//

#ifndef	C_IN_PLAYER_H
#define C_IN_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "singleplayer_animstate.h"
#include "c_basehlplayer.h"
#include "in_player_shared.h"

#include "beamdraw.h"
#include "flashlighteffect.h"

class C_IN_Player : public C_BaseHLPlayer
{
public:
	DECLARE_CLASS( C_IN_Player, C_BaseHLPlayer );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_IN_Player();
	~C_IN_Player();

	static C_IN_Player* GetLocalINPlayer();

	// FUNCIONES DE DEVOLUCIÓN DE DATOS
	float GetBlood() const { return m_HL2Local.m_iBlood; }
	float GetHunger() const { return m_HL2Local.m_iHunger; }
	float GetThirst() const { return m_HL2Local.m_iThirst; }

	int GetEntities() const { return m_HL2Local.m_iEntities; }

	// FUNCIONES INICIALES
	virtual void PreThink();
	virtual void NotifyShouldTransmit(ShouldTransmitState_t state);

	// FUNCIONES RELACIONADAS AL ATAQUE
	virtual void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr);
	virtual void DoImpactEffect(trace_t &tr, int nDamageType);

	// FUNCIONES RELACIONADAS AL MOVIMIENTO
	float CalculateSpeed(CBaseCombatWeapon *pWeapon = NULL, float speed = 0); // Shared!

	bool CanSprint();
	void StartSprinting();
	void StopSprinting();

	void HandleSpeedChanges();

	void StartWalking();
	void StopWalking();
	bool IsWalking() { return m_fIsWalking; }

	// OTROS
	virtual void ItemPreFrame();
	virtual void ItemPostFrame();

	// FUNCIONES RELACIONADAS A LA ANIMACIÓN Y EL MODELO
	virtual bool ShouldDraw();
	virtual const Vector& GetRenderOrigin();
	virtual int	DrawModel(int flags);

	virtual float GetFOV();
	virtual float GetMinFOV() const { return 5.0f; }

	virtual const QAngle& EyeAngles();
	virtual const QAngle& GetRenderAngles();

	virtual void UpdateClientSideAnimation();
	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

	virtual bool ShouldReceiveProjectedTextures(int flags);
	virtual void CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);

	ShadowType_t ShadowCastType();
	CStudioHdr *OnNewModel();
	void InitializePoseParams();

	Activity TranslateActivity(Activity baseAct, bool *pRequired);

	// FUNCIONES RELACIONADAS A LAS ARMAS
	virtual void AddEntity();
	virtual void UpdateFlashlight();
	virtual void ReleaseFlashlight();

	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual const char *GetConVar(const char *pConVar);

	//=========================================================
	// FUNCIONES RELACIONADAS AL INVENTARIO
	//=========================================================

	bool Inventory_HasItem(int pEntity, int pSection = INVENTORY_POCKET);
	int Inventory_GetItem(int Position, int pSection = INVENTORY_POCKET);

	const char *Inventory_GetItemName(int Position, int pSection = INVENTORY_POCKET);
	const char *Inventory_GetItemNameByID(int pEntity);

	const char *Inventory_GetItemClassName(int Position, int pSection = INVENTORY_POCKET);
	const char *Inventory_GetItemClassNameByID(int pEntity);

	virtual int Inventory_GetItemCount(int pSection = INVENTORY_POCKET);

private:
	CFlashlightEffect *m_pFlashlight;
	CFlashlightEffect *m_pFlashlightNonLocal;
	Beam_t	*m_pFlashlightBeam;

	CSinglePlayerAnimState *m_PlayerAnimState;
	CountdownTimer m_blinkTimer;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	int	m_headYawPoseParam;
	int	m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;

	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;

	EHANDLE m_hRagdoll;

	bool m_fIsWalking;
};

inline C_IN_Player *GetInPlayer(C_BasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_IN_Player *>(pEntity);
	#else
		return static_cast<C_IN_Player *>(pEntity);
	#endif

}

inline C_IN_Player *ToInPlayer(C_BaseEntity *pEntity)
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	#if _DEBUG
		return dynamic_cast<C_IN_Player *>(pEntity);
	#else
		return static_cast<C_IN_Player *>(pEntity);
	#endif
}

class C_INRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS(C_INRagdoll, C_BaseAnimatingOverlay);
	DECLARE_CLIENTCLASS();
	
	C_INRagdoll();
	~C_INRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	void UpdateOnRemove();
	virtual void SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);
	
private:
	
	C_INRagdoll( const C_INRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pDestinationEntity );
	void CreateINRagdoll();

private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

#endif