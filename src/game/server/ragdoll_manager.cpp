//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// game_ragdoll_manager
//
// Entidad encargada de controlar (y hacer desaparecer) los mu�ecos de trapo (ragdolls)
// con el fin de liberar memoria y gr�ficos.
//
//=====================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"
#include "ragdoll_shared.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CRagdollManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CRagdollManager, CBaseEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CRagdollManager();

	virtual void	Activate();
	virtual int		UpdateTransmitState();

	void UpdateCurrentMaxRagDollCount();

	void InputSetMaxRagdollCount(inputdata_t &data);
	void InputSetMaxRagdollCountDX8(inputdata_t &data);

	int DrawDebugTextOverlays();

	CNetworkVar(int,  m_iCurrentMaxRagdollCount);

	int m_iDXLevel;
	int m_iMaxRagdollCount;
	int m_iMaxRagdollCountDX8;

	bool m_bSaveImportant;
};


IMPLEMENT_SERVERCLASS_ST_NOBASE(CRagdollManager, DT_RagdollManager)
	SendPropInt(SENDINFO(m_iCurrentMaxRagdollCount), 6),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(game_ragdoll_manager, CRagdollManager);

BEGIN_DATADESC(CRagdollManager)

	DEFINE_FIELD(m_iCurrentMaxRagdollCount, FIELD_INTEGER),
	DEFINE_KEYFIELD(m_iMaxRagdollCount, FIELD_INTEGER,	"MaxRagdollCount"),
	DEFINE_KEYFIELD(m_iMaxRagdollCountDX8, FIELD_INTEGER, "MaxRagdollCountDX8"),

	DEFINE_KEYFIELD(m_bSaveImportant, FIELD_BOOLEAN, "SaveImportant"),

	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMaxRagdollCount",  InputSetMaxRagdollCount),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMaxRagdollCountDX8",  InputSetMaxRagdollCountDX8),

END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CRagdollManager::CRagdollManager()
{
	m_iMaxRagdollCount			= -1;
	m_iMaxRagdollCountDX8		= -1;
	m_iCurrentMaxRagdollCount	= -1;
}

//=========================================================
// Actualiza el estado de la transmisi�n.
//=========================================================
int CRagdollManager::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}

//=========================================================
// Activa la entidad
//=========================================================
void CRagdollManager::Activate()
{
	BaseClass::Activate();

	// Guardamos en cach� la versi�n de DirectX para su uso m�s tarde.
	ConVarRef mat_dxlevel("mat_dxlevel");
	m_iDXLevel = mat_dxlevel.GetInt();

	// [FIX] InSource - Valor predeterminado.
	if(m_iMaxRagdollCount == -1)
	{
		ConVarRef g_ragdoll_maxcount("g_ragdoll_maxcount");
		m_iMaxRagdollCount = g_ragdoll_maxcount.GetInt();
	}
	
	UpdateCurrentMaxRagDollCount();
}

//=========================================================
// Actualiza la cantidad actual de mu�ecos de trapo.
//=========================================================
void CRagdollManager::UpdateCurrentMaxRagDollCount()
{
	if ((m_iDXLevel < 90) && (m_iMaxRagdollCountDX8 >= 0))
		m_iCurrentMaxRagdollCount = m_iMaxRagdollCountDX8;
	else
		m_iCurrentMaxRagdollCount = m_iMaxRagdollCount;

	s_RagdollLRU.SetMaxRagdollCount(m_iCurrentMaxRagdollCount);
}

//=========================================================
// [INPUT] Actualiza la cantidad m�xima de mu�ecos de trapo.
//=========================================================
void CRagdollManager::InputSetMaxRagdollCount(inputdata_t &inputdata)
{
	m_iMaxRagdollCount = inputdata.value.Int();
	UpdateCurrentMaxRagDollCount();
}

//=========================================================
// [INPUT] Actualiza la cantidad m�xima de mu�ecos de trapo.
// En uso de DirectX 8 <- Gamers viviendo en una cueva.
//=========================================================
void CRagdollManager::InputSetMaxRagdollCountDX8(inputdata_t &inputdata)
{
	m_iMaxRagdollCountDX8 = inputdata.value.Int();
	UpdateCurrentMaxRagDollCount();
}

//=========================================================
// �Debo guardar los mu�ecos importantes/vitales?
//=========================================================
bool RagdollManager_SaveImportant(CAI_BaseNPC *pNPC)
{
	#ifdef HL2_DLL
		CRagdollManager *pEnt =	(CRagdollManager *)gEntList.FindEntityByClassname(NULL, "game_ragdoll_manager");

		if (pEnt == NULL)
			return false;

		if (pEnt->m_bSaveImportant)
		{
			if (pNPC->Classify() == CLASS_PLAYER_ALLY || pNPC->Classify() == CLASS_PLAYER_ALLY_VITAL)
				return true;
		}
	#endif

	return false;
}

//=========================================================
// Escribe en la pantalla informaci�n de desarrollo. (debug)
//=========================================================
int CRagdollManager::DrawDebugTextOverlays() 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print max ragdoll count
		Q_snprintf(tempstr,sizeof(tempstr),"max ragdoll count: %f", m_iCurrentMaxRagdollCount);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}

	return text_offset;
}

