//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if !defined( C_HL2_PLAYERLOCALDATA_H )
#define C_HL2_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "dt_recv.h"
#include "hl2/hl_movedata.h"

EXTERN_RECV_TABLE( DT_HL2Local );


class C_HL2PlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( C_HL2PlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	C_HL2PlayerLocalData();

	float	m_flSuitPower;
	bool	m_bZooming;
	int		m_bitsActiveDevices;
	int		m_iSquadMemberCount;
	int		m_iSquadMedicCount;
	bool	m_fSquadInFollowMode;
	bool	m_bWeaponLowered;
	EHANDLE m_hAutoAimTarget;
	Vector	m_vecAutoAimPoint;
	bool	m_bDisplayReticle;
	bool	m_bStickyAutoAim;
	bool	m_bAutoAimTarget;
	float	m_flFlashBattery;
	Vector	m_vecLocatorOrigin;

	// Ladder related data
	EHANDLE			m_hLadder;
	LadderMove_t	m_LadderMove;

	//=========================================================
	// INSOURCE

	float m_iBlood;				// InSource - Sangre
	float m_iHunger;			// InSource - Hambre
	float m_iThirst;			// InSource - Sed

	int PocketItems[100];		// InSource - Inventario.
	int BackpackItems[100];		// InSource - Inventario.

	int m_iEntities;

#ifdef FURNANCE
	int m_iNectar;	// Furnance - Nectar
#endif

};


#endif
