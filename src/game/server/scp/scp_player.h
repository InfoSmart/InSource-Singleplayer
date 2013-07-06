#ifndef SCP_PLAYER_H

#define SCP_PLAYER_H
#pragma once

#include "in_player.h"

class CSCP_Player : public CIN_Player
{
public:
	DECLARE_CLASS(CSCP_Player, CIN_Player);

	CSCP_Player();
	~CSCP_Player();

	// FUNCIONES PRINCIPALES
	virtual void Precache();
	virtual void Spawn();

	// FUNCIONES DE UTILIDAD
	virtual PlayerGender Gender();
	virtual void CheatImpulseCommands(int iImpulse);

	DECLARE_DATADESC();
};

#endif //SCP_PLAYER_H