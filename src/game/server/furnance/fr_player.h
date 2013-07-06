#ifndef FR_PLAYER_H

#define FR_PLAYER_H
#pragma once

#include "in_player.h"

class CFR_Player : public CIN_Player
{
	DECLARE_CLASS(CFR_Player, CIN_Player);
	DECLARE_DATADESC();

public:
	CFR_Player();
	~CFR_Player();

	// FUNCIONES DE DEVOLUCIÓN DE DATOS

	// Devuelve si la regenación de salud será automatica.
	bool AutoHealthRegeneration() { return false; }
		
	void SetNectar(int iNectar) { m_iNectar = iNectar; m_HL2Local.m_iNectar = iNectar; }
	// Devuelve la cantidad de nectar en el cuerpo.
	int GetNectar() { return m_iNectar; }
	// Devuelve la cantidad máxima de nectar.
	int GetMaxNectar() { return m_iMaxNectar; }
	// Establece la cantidad máxima de nectar.
	void SetMaxNectar(int iMax) { m_iMaxNectar = iMax; }

	// FUNCIONES PRINCIPALES
	virtual void Spawn();
	virtual void Precache();
	virtual void PostThink();

	virtual void NectarThink();

	// FUNCIONES RELACIONADAS A HL2
	virtual void SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime);
	virtual void CheatImpulseCommands(int iImpulse);

	//  FUNCIONES RELACIONADAS AL DAÑO/SALUD
	virtual void HealthRegeneration();
	virtual int TakeNectar(float flNectar);

	// FUNCIONES DE UTILIDAD
	virtual PlayerGender Gender();

private:
	int m_iMaxNectar;
	int m_iNectar;
	int m_iEmptyNectarSeconds;
};

inline CFR_Player *GetFRPlayer(CBasePlayer *pEntity)
{
	if ( !pEntity )
		return NULL;

	#if _DEBUG
		return dynamic_cast<CFR_Player *>(pEntity);
	#else
		return static_cast<CFR_Player *>(pEntity);
	#endif

}

inline CFR_Player *ToFRPlayer(CBaseEntity *pEntity)
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CFR_Player *>(pEntity);
}

#endif