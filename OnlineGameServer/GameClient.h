#pragma once
#include "../NetRoot/LanClient/LanClient.h"
#include "../CommonProtocol.h"

namespace server_baby
{
	class GameClient : public LanClient
	{
	protected:
		bool OnRecv(LanPacketSet* const packetList)
		{
			LanPacketSet::Free(packetList);
			return true;
		}

		void OnConnect()
		{
			LanPacket* packet = LanPacket::Alloc();

			*packet << (WORD)en_PACKET_SS_MONITOR_LOGIN;
			*packet << (int)eGAME_SERVER_NO;

			SendPacket(packet);
			LanPacket::Free(packet);

		}
	};
}
