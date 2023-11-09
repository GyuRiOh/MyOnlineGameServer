#pragma once

#include "../NetRoot/NetServer/NetPipe.h"
#include "UserMap.h"
#include "../NetRoot/Common/DBConnector.h"
#include <cpp_redis/cpp_redis>

namespace MyNetwork
{
	class AuthPipe_CS_Stub;
	class AuthPipe_SC_Proxy;
	class PipePlayer;

	class AuthPipe final : public NetPipe
	{
	public:
		explicit AuthPipe(NetRoot* server, unsigned int framePerSecond, unsigned int threadNum = 1) noexcept;
		virtual ~AuthPipe() noexcept { delete proxy_; delete redisClient_; }
		NetUser* OnUserJoin(NetSessionID NetSessionID) noexcept override;
		void OnUserLeave(NetUser* const user) noexcept override;
		void OnUserMoveIn(NetUser* const user) noexcept override;
		void OnUserMoveOut(NetUser* const user) noexcept override;
		void OnUserUpdate(NetUser* const user) noexcept override;
		void OnUpdate() noexcept override {}
		void OnMonitor(const LoopInfo* const info) noexcept override;
		void OnStart() noexcept override;
		void OnStop() noexcept override;

		void DBSave(BYTE dbType, const WCHAR* stringFormat, ...) noexcept;

		MYSQL_ROW GameDBQuery(const WCHAR* stringFormat, ...) noexcept;
		MYSQL_ROW LogDBQuery(const WCHAR* stringFormat, ...) noexcept;
		MYSQL_ROW AccountDBQuery(const WCHAR* stringFormat, ...) noexcept;
		void GameDBFreeResult();
		void LogDBFreeResult();
		void AccountDBFreeResult();

		bool DisconnectAbnormalPlayer(NetSessionID ID) noexcept { return Disconnect(ID);}

	public:
		cpp_redis::client* redisClient_;
		UserMap<NetSessionID> onlineMap_;		
		AuthPipe_SC_Proxy* proxy_;
		int framePerSec_;

	};
}