#ifndef DIRECTOR_HEALTHKIT_SPAWN_H

#define DIRECTOR_HEALTHKIT_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

class CDirectorHealthSpawn : public CDirectorBaseSpawn
{
public:
	DECLARE_CLASS(CDirectorHealthSpawn, CDirectorBaseSpawn);
	DECLARE_DATADESC();

	void Precache();
	void Make();
	bool MaySpawn();

	const char *SelectClass();
};

#endif // DIRECTOR_HEALTHKIT_SPAWN_H