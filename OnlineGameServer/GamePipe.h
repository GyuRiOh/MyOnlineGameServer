#pragma once

#include "../NetRoot/NetServer/NetPipe.h"
#include "../Object/ObjectManager.h"
#include "../Sector/SectorMap.h"
#include "../OnlineGameServer/UserMap.h"
#include "../OnlineGameServer/PipePlayer.h"
#include "../NetRoot/Common/Queue.h"
#include "../CommonProtocol.h"
#include <thread>

namespace MyNetwork
{
	class PipePlayer;
	class GamePipe_CS_Stub;
	class GamePipe_SC_Proxy;

	class GamePipe final : public NetPipe
	{
	public:
		explicit GamePipe(NetRoot* server, unsigned int framePerSecond, unsigned int threadNum = 1) noexcept ;
		virtual ~GamePipe() noexcept;

		NetUser* OnUserJoin(const NetSessionID sessionID) noexcept override;
		void OnUserLeave(NetUser* const user) noexcept override;
		void OnUserMoveIn(NetUser* const user) noexcept override;
		void OnUserMoveOut(NetUser* const user) noexcept override;
		void OnUserUpdate(NetUser* const user) noexcept override;
		void OnUpdate() noexcept override;
		void OnMonitor(const LoopInfo* const info) noexcept override;
		void OnStart() noexcept override;
		void OnStop() noexcept override; 

		void GetSessionIDSet_AroundSector(SectorPos sector, NetSessionIDSet* const set) noexcept;
		void GetSessionIDSet_AroundSector_WithoutID(NetSessionID exceptionID, SectorPos sector, NetSessionIDSet* const set) noexcept;
		UINT64 GetClientIDStamp() noexcept { return (clientIDStamp_++); }
		const std::vector<vector<unsigned char>>& GetMap();

		void UpdateSector(PipePlayer* player) noexcept ;
		void UpdateSector(BaseObject* object) noexcept;
		void StartGame(PipePlayer* player) noexcept ;
		void RestartGame(PipePlayer* player) noexcept;
		void AttackPlayer() noexcept;
		void DBSave(PipePlayer* player, BYTE dbType, const WCHAR* stringFormat, ...) noexcept;

		PipePlayer* GetNearestPlayer(TilePos monsterPos);
	private:
		void GenerateNewMonster() noexcept;
		void MoveMonster() noexcept;
		void InitializeMap() noexcept;
		void RemoveMonsterAndCreateCrystal() noexcept;
	
		void CreateObjectInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept;
		void CreateOthreCharacterInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept;
		void CreateMyCharacterInSectorAround(PipePlayer* player) noexcept;
		void RemoveMyCharacterFromOldSector(PipePlayer* player) noexcept;
		void RemoveObjectFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept;
		void RemoveOtherCharacterFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept;

	public:
		std::vector<vector<BYTE>> map_;
		ObjectManager objectManager_;
		SectorMap<PipePlayer*> sectorMap_;

		GamePipe_SC_Proxy* proxy_;
		int framePerSec_;
	private:
		UINT64 clientIDStamp_;

	};


}