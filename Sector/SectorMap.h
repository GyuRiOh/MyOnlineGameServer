#pragma once
#include <unordered_map>
#include <functional>
#include "Sector.h"
#include "../NetRoot/Common/Crash.h"
#include "../NetRoot/NetServer/NetSessionID.h"
#include "../Object/BaseObject.h"

template<class Object>
class SectorMap
{
public:
	explicit SectorMap() {}
	~SectorMap() {}

	void AddTo(INT64 clientID, SectorPos sector, Object object)
	{
		sectorMap_[sector.yPos_][sector.xPos_].emplace(clientID, object);
	}

	bool RemoveFrom(INT64 clientID, SectorPos sector)
	{
		if (sector.xPos_ != X_DEFAULT)
		{
			if (sectorMap_[sector.yPos_][sector.xPos_].erase(clientID) != 1)
				ErrorQuit(L"RemoveFromCurSector - Not Erased");

			return true;
		}
		else
			return false;
	}

	void Update(INT64 clientID, SectorPos oldSector, SectorPos curSector, Object object)
	{
		if (oldSector.xPos_ != X_DEFAULT)
		{
			if (curSector.xPos_ == oldSector.xPos_ &&
				curSector.yPos_ == oldSector.yPos_)
				return;

			RemoveFrom(clientID, oldSector);
		}
		
		AddTo(clientID, curSector, object);
	}

	void GetSectorAround(SectorPos sector, SectorAround& sectorAround)
	{
		short sectorX = sector.xPos_;
		short sectorY = sector.yPos_;

		sectorX--;
		sectorY--;

		sectorAround.count_ = 0;

		for (int iCntY = 0; iCntY < 3; iCntY++)
		{

			if (sectorY + iCntY < 0 || sectorY + iCntY >= SECTOR_Y_MAX)
				continue;

			for (int iCntX = 0; iCntX < 3; iCntX++)
			{
				if (sectorX + iCntX < 0 || sectorX + iCntX >= SECTOR_X_MAX)
					continue;

				sectorAround.around_[sectorAround.count_].xPos_ = sectorX + iCntX;
				sectorAround.around_[sectorAround.count_].yPos_ = sectorY + iCntY;
				sectorAround.count_++;
			}
		}

	}

	void ForeachSector(SectorPos sector, std::function<void(Object&)> func)
	{
		for (auto& obj : sectorMap_[sector.yPos_][sector.xPos_])
			func(obj.second);
	}

	void ForeachSectorAround(SectorPos sector, std::function<void(Object&)> func)
	{
		short sectorX = sector.xPos_;
		short sectorY = sector.yPos_;

		sectorX--;
		sectorY--;

		for (int iCntY = 0; iCntY < 3; iCntY++)
		{

			if (sectorY + iCntY < 0 || sectorY + iCntY >= SECTOR_Y_MAX)
				continue;

			for (int iCntX = 0; iCntX < 3; iCntX++)
			{
				if (sectorX + iCntX < 0 || sectorX + iCntX >= SECTOR_X_MAX)
					continue;

				ForeachSector(SectorPos(sectorX + iCntX, sectorY + iCntY), func);
			}
		}
	}

	void ForeachIntersectionSectorAround(SectorPos sector1, SectorPos sector2, std::function<void(Object&)> func)
	{
		SectorAround sectorAround1;
		SectorAround sectorAround2;

		GetSectorAround(sector1, sectorAround1);
		GetSectorAround(sector2, sectorAround2);

		for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
		{
			for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					ForeachSector(sectorAround1.around_[sector1Cnt], func);
					break;
				}
				else
					continue;
			}
		}
	}

	void ForeachComplementSectorAround_BasedFrom(SectorPos baseSector, SectorPos otherSector, std::function<void(Object&)> func)
	{
		SectorAround baseAround;
		SectorAround otherAround;

		GetSectorAround(baseSector, baseAround);
		GetSectorAround(otherSector, otherAround);

		for (int baseCnt = 0; baseCnt < baseAround.count_; baseCnt++)
		{
			bool complementFlag = true;
			for (int otherCnt = 0; otherCnt < otherAround.count_; otherCnt++)
			{
				if (baseAround.around_[baseCnt] == otherAround.around_[otherCnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(baseAround.around_[baseCnt], func);
		}

	}

	void ForeachComplementSectorAround(SectorPos sector1, SectorPos sector2, std::function<void(Object&)> func)
	{
		SectorAround sectorAround1;
		SectorAround sectorAround2;

		GetSectorAround(sector1, sectorAround1);
		GetSectorAround(sector2, sectorAround2);

		for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
		{
			bool complementFlag = true;
			for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(sectorAround1.around_[sector1Cnt], func);		
		}

		for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
		{
			bool complementFlag = true;
			for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(sectorAround2.around_[sector2Cnt], func);
		}

	}

	void ForeachSectorAll(std::function<void(Object&)> func)
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				ForeachSector(SectorPos(iCntX, iCntY), func);
			}
		}
	}

	void Destroy(std::function<bool(Object&)> func)
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				for (auto iter = sectorMap_[iCntY][iCntX].begin(); iter != sectorMap_[iCntY][iCntX].end();)
				{
					if (func((*iter).second))
						iter = sectorMap_[iCntY][iCntX].erase(iter);
					else
						++iter;
				}
			}
		}
	}

	void Clear()
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				sectorMap_[iCntY][iCntX].clear();
			}
		}
	}

	void ErrorQuit(const WCHAR* msg)
	{
		MyNetwork::SystemLogger::GetInstance()->Console(L"SectorMap", MyNetwork::LEVEL_SYSTEM, msg);
		MyNetwork::SystemLogger::GetInstance()->LogText(L"SectorMap", MyNetwork::LEVEL_SYSTEM, msg);

		MyNetwork::CrashDump::Crash();
	}

	//Key: SessionID, Value : Player*
	std::unordered_map<INT64, Object> sectorMap_[SECTOR_Y_MAX][SECTOR_X_MAX];

};

template<>
class SectorMap<BaseObject*>
{
public:
	explicit SectorMap() {}
	~SectorMap() {}

	void AddTo(INT64 clientID, SectorPos sector, BaseObject*& object)
	{
		sectorMap_[sector.yPos_][sector.xPos_].emplace(clientID, object);
	}

	bool RemoveFrom(INT64 clientID, SectorPos sector)
	{
		if (sector.xPos_ != X_DEFAULT)
		{
			if (sectorMap_[sector.yPos_][sector.xPos_].erase(clientID) != 1)
				ErrorQuit(L"RemoveFromCurSector - Not Erased");

			return true;
		}
		else
			return false;
	}

	void Update(INT64 clientID, SectorPos oldSector, SectorPos curSector, BaseObject* object)
	{
		if (oldSector.xPos_ != X_DEFAULT)
		{
			if (curSector.xPos_ == oldSector.xPos_ &&
				curSector.yPos_ == oldSector.yPos_)
				return;

			RemoveFrom(clientID, oldSector);
		}

		AddTo(clientID, curSector, object);
	}

	void GetSectorAround(SectorPos sector, SectorAround& sectorAround)
	{
		short sectorX = sector.xPos_;
		short sectorY = sector.yPos_;

		sectorX--;
		sectorY--;

		sectorAround.count_ = 0;

		for (int iCntY = 0; iCntY < 3; iCntY++)
		{

			if (sectorY + iCntY < 0 || sectorY + iCntY >= SECTOR_Y_MAX)
				continue;

			for (int iCntX = 0; iCntX < 3; iCntX++)
			{
				if (sectorX + iCntX < 0 || sectorX + iCntX >= SECTOR_X_MAX)
					continue;

				sectorAround.around_[sectorAround.count_].xPos_ = sectorX + iCntX;
				sectorAround.around_[sectorAround.count_].yPos_ = sectorY + iCntY;
				sectorAround.count_++;
			}
		}

	}


	void ForeachSector(SectorPos sector, std::function<void(BaseObject*&)> func)
	{
		for (auto& obj : sectorMap_[sector.yPos_][sector.xPos_])
		{
			if (obj.second->isDestroyed())
				continue;

			func(obj.second);
		}
	}

	void ForeachSectorAround(SectorPos sector, std::function<void(BaseObject*&)> func)
	{
		short sectorX = sector.xPos_;
		short sectorY = sector.yPos_;

		sectorX--;
		sectorY--;

		for (int iCntY = 0; iCntY < 3; iCntY++)
		{

			if (sectorY + iCntY < 0 || sectorY + iCntY >= SECTOR_Y_MAX)
				continue;

			for (int iCntX = 0; iCntX < 3; iCntX++)
			{
				if (sectorX + iCntX < 0 || sectorX + iCntX >= SECTOR_X_MAX)
					continue;

				ForeachSector(SectorPos(sectorX + iCntX, sectorY + iCntY), func);
			}
		}
	}

	void ForeachIntersectionSectorAround(SectorPos sector1, SectorPos sector2, std::function<void(BaseObject*&)> func)
	{
		SectorAround sectorAround1;
		SectorAround sectorAround2;

		GetSectorAround(sector1, sectorAround1);
		GetSectorAround(sector2, sectorAround2);

		for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
		{
			for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					ForeachSector(sectorAround1.around_[sector1Cnt], func);
					break;
				}
				else
					continue;
			}
		}
	}

	void ForeachComplementSectorAround_BasedFrom(SectorPos baseSector, SectorPos otherSector, std::function<void(BaseObject*&)> func)
	{
		SectorAround baseAround;
		SectorAround otherAround;

		GetSectorAround(baseSector, baseAround);
		GetSectorAround(otherSector, otherAround);

		for (int baseCnt = 0; baseCnt < baseAround.count_; baseCnt++)
		{
			bool complementFlag = true;
			for (int otherCnt = 0; otherCnt < otherAround.count_; otherCnt++)
			{
				if (baseAround.around_[baseCnt] == otherAround.around_[otherCnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(baseAround.around_[baseCnt], func);
		}

	}

	void ForeachComplementSectorAround(SectorPos sector1, SectorPos sector2, std::function<void(BaseObject*&)> func)
	{
		SectorAround sectorAround1;
		SectorAround sectorAround2;

		GetSectorAround(sector1, sectorAround1);
		GetSectorAround(sector2, sectorAround2);

		for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
		{
			bool complementFlag = true;
			for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(sectorAround1.around_[sector1Cnt], func);
		}

		for (int sector2Cnt = 0; sector2Cnt < sectorAround2.count_; sector2Cnt++)
		{
			bool complementFlag = true;
			for (int sector1Cnt = 0; sector1Cnt < sectorAround1.count_; sector1Cnt++)
			{
				if (sectorAround1.around_[sector1Cnt] == sectorAround2.around_[sector2Cnt])
				{
					complementFlag = false;
					break;
				}
				else
					continue;
			}

			if (complementFlag)
				ForeachSector(sectorAround2.around_[sector2Cnt], func);
		}

	}

	void ForeachSectorAll(std::function<void(BaseObject*&)> func)
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				ForeachSector(SectorPos(iCntX, iCntY), func);
			}
		}
	}

	void Destroy(std::function<bool(BaseObject*&)> func)
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				for (auto iter = sectorMap_[iCntY][iCntX].begin(); iter != sectorMap_[iCntY][iCntX].end();)
				{
					if (func((*iter).second))
						iter = sectorMap_[iCntY][iCntX].erase(iter);
					else
						++iter;
				}
			}
		}
	}

	void Clear()
	{
		for (int iCntY = 0; iCntY < SECTOR_Y_MAX; iCntY++)
		{
			for (int iCntX = 0; iCntX < SECTOR_X_MAX; iCntX++)
			{
				sectorMap_[iCntY][iCntX].clear();
			}
		}
	}

	void ErrorQuit(const WCHAR* msg)
	{
		MyNetwork::SystemLogger::GetInstance()->Console(L"SectorMap", MyNetwork::LEVEL_SYSTEM, msg);
		MyNetwork::SystemLogger::GetInstance()->LogText(L"SectorMap", MyNetwork::LEVEL_SYSTEM, msg);

		MyNetwork::CrashDump::Crash();
	}

	//Key: SessionID, Value : Player*
	std::unordered_map<INT64, BaseObject*> sectorMap_[SECTOR_Y_MAX][SECTOR_X_MAX];

};
