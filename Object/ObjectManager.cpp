#include "ObjectManager.h"
#include "../NetRoot/Common/Crash.h"
#include <algorithm>
#include "../NetRoot/NetServer/NetSessionID.h"
#include "../NetRoot/Common/SizedMemoryPool.h"

const TilePos monsterRespawn[6] = {
	TilePos(15.0f, 85.0f),
	TilePos(17.5f, 34.0f),
	TilePos(83.5f, 79.0f),
	TilePos(81.5f, 18.5f),
	TilePos(106.5f, 92.5f),
	TilePos(156.5f, 60.0f)};

ObjectManager::ObjectManager() : monsterSize_(0), crystalSize_(0){}

Monster* ObjectManager::CreateMonster(INT64 id) noexcept
{	
	int random = rand() % 6;

	BaseObject* monster = reinterpret_cast<BaseObject*>(
		MyNetwork::SizedMemoryPool::GetInstance()->Alloc(sizeof(Monster)));
		
	new (monster) Monster(
		monsterRespawn[random].clientX,
		monsterRespawn[random].clientY,
		static_cast<int>(eMONSTER_TYPE), 
		static_cast<long long>(id));

	sectorMap_.AddTo(id, monster->GetCurSector(), monster);
	monsterSize_++;

	return static_cast<Monster*>(monster);
}

Crystal* ObjectManager::CreateCrystal(float X, float Y, BYTE type, INT64 id) noexcept
{
	BaseObject* crystal = reinterpret_cast<BaseObject*>(
		MyNetwork::SizedMemoryPool::GetInstance()->Alloc(sizeof(Crystal)));
		
	new (crystal) Crystal(X, Y, eCRYSTAL_TYPE, type, id);

	sectorMap_.AddTo(id, crystal->GetCurSector(), crystal);
	crystalSize_++;

	return static_cast<Crystal*>(crystal);
}

void ObjectManager::Update() noexcept
{
	sectorMap_.ForeachSectorAll([](BaseObject*& obj) {
		obj->OnUpdate();
		});

	sectorMap_.Destroy([this](BaseObject*& obj)
		{
			if (obj->isDestroyed())
			{
				switch (obj->GetType())
				{
				case eMONSTER_TYPE:
					monsterSize_--;
					break;
				case eCRYSTAL_TYPE:
					crystalSize_--;
					break;
				default:
					MyNetwork::CrashDump::Crash();
					break;
				}

				obj->~BaseObject();
				MyNetwork::SizedMemoryPool::GetInstance()->Free(obj);
				return true;
			}
			else
				return false;
		});
}

void ObjectManager::UpdateSectorMap(BaseObject* obj) noexcept
{
	sectorMap_.Update(obj->GetClientID(), obj->GetOldSector(), obj->GetCurSector(), obj);
}

void ObjectManager::ForeachSector(SectorPos sector, std::function<void(BaseObject*)> func) noexcept
{
	sectorMap_.ForeachSector(sector, func);
}

void ObjectManager::ForeachSectorAround(SectorPos sector, std::function<void(BaseObject*)> func) noexcept
{
	sectorMap_.ForeachSectorAround(sector, func);
}

void ObjectManager::ForeachAll(std::function<void(BaseObject*)> func) noexcept
{
	sectorMap_.ForeachSectorAll(func);
}

void ObjectManager::ForeachComplementSectorAround_BasedFrom(SectorPos base, SectorPos other, std::function<void(BaseObject*)> func) noexcept
{
	sectorMap_.ForeachComplementSectorAround_BasedFrom(base, other, func);
}

