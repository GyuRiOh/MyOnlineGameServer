
#ifndef __OBJECT__MANAGER__
#define __OBEJCT__MANAGER__

#include <vector>
#include <functional>
#include "../NetRoot/Common/Singleton.h"
#include "../Sector/SectorMap.h"
#include "BaseObject.h"
#include "Crystal.h"
#include "Monster.h"


class ObjectManager final
{
public:
	ObjectManager();
	~ObjectManager() {}

	Monster* CreateMonster(INT64 id) noexcept;
	Crystal* CreateCrystal(float X, float Y, BYTE type, INT64 id) noexcept;
	void Update() noexcept;
	void UpdateSectorMap(BaseObject* obj) noexcept ;

	size_t GetMonsterMapSize() { return monsterSize_; }

	//섹터 도는 함수 만들기
	void ForeachSector(SectorPos sector, std::function<void(BaseObject*)> func) noexcept;
	void ForeachSectorAround(SectorPos sector, std::function<void(BaseObject*)> func) noexcept;
	void ForeachAll(std::function<void(BaseObject*)> func) noexcept;
	void ForeachComplementSectorAround_BasedFrom(SectorPos base, SectorPos other, std::function<void(BaseObject*)> func) noexcept;	


private:
	SectorMap<BaseObject*> sectorMap_;

	int monsterSize_;
	int crystalSize_;
};




#endif