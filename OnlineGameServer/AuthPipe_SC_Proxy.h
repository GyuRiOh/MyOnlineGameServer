#include "../NetRoot/NetServer/NetPacket.h"
#include "../NetRoot/NetServer/NetSessionID.h"
#include "AuthPipe.h"
#include "../CommonProtocol.h"

namespace MyNetwork
{
	class AuthPipe_SC_Proxy final
	{
	public:
		explicit AuthPipe_SC_Proxy(AuthPipe* server) : server_(server){}

		void ResLogin(BYTE status, INT64 accountNo, NetSessionID NetSessionID)
		{
			//원격 함수 호출을 요청하는 Proxy의 경우, 
			//아무것도 하지 않아도
			//자동으로 함수 코드 전체가 만들어진다.

			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_LOGIN);
			*msg << status;
			*msg << accountNo;

			server_->AsyncSendPacket(NetSessionID, msg);
			NetPacket::Free(msg);
		}
		
		void ResCharacterSelect(BYTE status, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CHARACTER_SELECT);
			*msg << status;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResEcho(INT64 accountNo, LONGLONG sendTick, NetSessionID NetSessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_ECHO);
			*msg << accountNo;
			*msg << sendTick;

			server_->AsyncSendPacket(NetSessionID, msg);
			NetPacket::Free(msg);
		}


	private:
		AuthPipe* server_;
	};
}
