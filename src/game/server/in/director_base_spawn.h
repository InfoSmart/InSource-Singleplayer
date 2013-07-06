#ifndef DIRECTOR_BASE_SPAWN_H
#define DIRECTOR_BASE_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

// Opciones de creación.
#define SF_NO_SPAWN_VIEWCONE	2

class CDirectorBaseSpawn : public CLogicalEntity
{
public:
	DECLARE_CLASS(CDirectorBaseSpawn, CLogicalEntity);
	DECLARE_DATADESC();

	CDirectorBaseSpawn();
	~CDirectorBaseSpawn();

	virtual void Spawn();
	virtual void Precache();

	virtual void Make() { }
	virtual bool MaySpawn() { return true; }

	void Enable();
	void Disable();

	virtual bool CanSpawn(CBaseEntity *pItem, Vector *pResult);
	virtual bool FindSpotInRadius(Vector *pResult, const Vector &vStartPos, CBaseEntity *pItem, float radius);

	void InputSpawn(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);

protected:
	bool Disabled;
	bool SpawnCount;
};

#endif //DIRECTOR_BASE_SPAWN_H