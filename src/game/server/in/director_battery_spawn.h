#ifndef DIRECTOR_BATTERY_SPAWN_H

#define DIRECTOR_BATTERY_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

class CDirectorBatterySpawn : public CDirectorBaseSpawn
{
public:
	DECLARE_CLASS(CDirectorBatterySpawn, CDirectorBaseSpawn);
	DECLARE_DATADESC();

	void Precache();
	void Make();
	bool MaySpawn();
};

#endif // DIRECTOR_BATTERY_SPAWN_H