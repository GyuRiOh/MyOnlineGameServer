#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#include "../NetRoot/NetServer/NetSessionID.h"
#include "../NetRoot/Common/RPCBuffer.h"
#include "AuthPipe_SC_Proxy.h"
#include "AuthPipe.h"
#include "PipePlayer.h"
#include "../CommonProtocol.h"
#include "GameServer.h"
#include "../DBSaveJob.h"

namespace server_baby
{
	class AuthPipe_CS_Stub final : public NetStub
	{
	public:
		explicit AuthPipe_CS_Stub(AuthPipe* server) : server_(server){}

		bool PacketProc(NetSessionID sessionID, NetDummyPacket* msg) override
		{
			WORD type;
			*msg >> type;
			switch (type)
			{
			case en_PACKET_CS_GAME_REQ_LOGIN:
			{
				INT64 accountNo;
				int version;
				*msg >> accountNo;
				RPCBuffer sessionKeyBuf(64);
				msg->DeqData((char*)sessionKeyBuf.data, 64);
				char* sessionKey = (char*)sessionKeyBuf.Data();
				*msg >> version;
				return ReqLogin(accountNo, sessionKey, version, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
			{
				BYTE type;
				*msg >> type;
				return ReqCharacterSelect(type, sessionID);
			}				
			case en_PACKET_CS_GAME_REQ_ECHO:
			{
				INT64 accountNo;
				LONGLONG sendTick;
				*msg >> accountNo;
				*msg >> sendTick;

				return ReqEcho(accountNo, sendTick, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				return true;
			}
			return false;
		}

		bool ReqLogin(INT64 accountNo, char* sessionKey, int version, NetSessionID sessionID)
		{
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));
			if (!player)
				CrashDump::Crash();

			//중복로그인 체크 - 온라인 맵
			if (!server_->onlineMap_.Insert(sessionID, accountNo))
			{
				SystemLogger::GetInstance()->LogText(L"AuthPipe", LEVEL_ERROR,
					L"Login - Duplicated. %d", accountNo);

				NetSessionID abnormalPlayer;
				server_->onlineMap_.Find(&abnormalPlayer, accountNo);

				//중복 로그인 발생 시
				//기존 유저, 새 유저 둘다 연결 끊기
				server_->DisconnectAbnormalPlayer(abnormalPlayer);
				return false;
			}
			
			//Redis에서 세션 키 읽고 확인
			if (accountNo > 1000000)
			{
				bool redisResult = false;
				std::string accountNum = to_string(accountNo);

				server_->redisClient_->get
				(accountNum, [&redisResult, sessionKey](cpp_redis::reply& reply) {
					if (reply.as_string().compare(sessionKey) != 0)
					cout << "Redis Not Equal" << endl;
					else
						redisResult = true;
					});

				server_->redisClient_->sync_commit();

				if (!redisResult)
					return false;
			}	

			//DB에서 닉네임 읽어오기
			WCHAR nickName[20] = {0};
			size_t ret = 0;
			MYSQL_ROW nickNameData = server_->AccountDBQuery(L"SELECT usernick FROM account WHERE accountno = %d", accountNo);
			mbstowcs_s(&ret, nickName, 20, nickNameData[0], 20);
			server_->AccountDBFreeResult();

			//Player 정보 DB에서 읽고 세팅
			//DB에 결과가 있으면 바로 GamePipe로 보냄
			//DB에 결과가 없으면, 선택 모드로 전환
			
			MYSQL_ROW characterRow = server_->GameDBQuery(L"SELECT * FROM gamedb.character WHERE accountno = %d", accountNo);
			if (!characterRow)
			{
				//캐릭터가 존재하지 않음
				//아직 AuthPipe에 머무름
				player->InitializePlayerData(accountNo, nickName);
				server_->proxy_->ResLogin(2, player->GetAccountNumber(), sessionID);
			}
			else
			{
				//캐릭터가 존재함
			//정보 해체해서 플레이어 정보 세팅함
				BYTE characterType = atoi(characterRow[1]);
				float posX = atof(characterRow[2]);
				float posY = atof(characterRow[3]);
				int tileX = atoi(characterRow[4]);
				int tileY = atoi(characterRow[5]);
				USHORT rotation = atoi(characterRow[6]);
				int crystal = atoi(characterRow[7]);
				int hp = atoi(characterRow[8]);
				ULONGLONG exp = atoi(characterRow[9]);
				int level = atoi(characterRow[10]);
				int die = atoi(characterRow[11]);

				player->InitializePlayerData(
					accountNo,
					nickName,
					posX,
					posY,
					crystal,
					hp,
					exp,
					level,
					characterType);

				//DB에서 얻은 결과물 할당해제하기
				server_->GameDBFreeResult();

				//바로 GamePipe로 이동
				server_->MovePipe(static_cast<NetUser*>(player), GAME_PIPE_ID);

				//로그인 완료 패킷 전송
				server_->proxy_->ResLogin(1, player->GetAccountNumber(), sessionID);

			}
					
			return true;
		}

		bool ReqCharacterSelect(BYTE type, NetSessionID sessionID)
		{
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//type이 1-5중 하나인 경우 성공
			//범위를 벗어나는 경우 실패
			if (type >= 1 && type <= 5)
			{
				//캐릭터타입 정보 초기화
				player->InitializeCharacterType(type);

				//캐릭터 생성 로그 남기기
				WCHAR logTime[256] = { 0 };
				GetDateTime(logTime);

				server_->DBSave(eLogDB,
					L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, NULL, NULL, NULL, NULL)",
					logTime,
					player->GetAccountNumber(),
					3,
					32,
					type);

				//신규 캐릭터 정보 DB에 저장
				server_->DBSave(eGameDB, L"INSERT INTO gamedb.character VALUES (%d, %d, %f, %f, %d, %d, %d, %d, %d, %d, %d, %d)", 
					player->GetAccountNumber(),
					player->GetCharacterType(),
					player->GetClientPosX(),
					player->GetClientPosY(),
					player->GetTileX(),
					player->GetTileY(),
					player->GetRotation(),
					player->GetCrystal(),
					player->GetHP(),
					player->GetExp(),
					player->GetLevel(),
					player->isDead()	
				);
	
				//게임 파이프로 이동
				server_->MovePipe(static_cast<NetUser*>(player), GAME_PIPE_ID);

				//GAME MODE 패킷 전송
				server_->proxy_->ResCharacterSelect(1, sessionID);

				return true;
			}
			else
			{
				//실패 결과 반송
				//이후 연결 끊기
				server_->proxy_->ResCharacterSelect(0, sessionID);
				return false;
			}

		}

		bool ReqEcho(INT64 accountNo, LONGLONG sendTick, NetSessionID sessionID)
		{
			server_->proxy_->ResEcho(
				accountNo,
				sendTick,
				sessionID);

			return true;
		}

	private:
		AuthPipe* server_;
	};
}
