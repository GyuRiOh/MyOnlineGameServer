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
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));
			if (!player)
				CrashDump::Crash();

			//�ߺ��α��� üũ - �¶��� ��
			if (!server_->onlineMap_.Insert(sessionID, accountNo))
			{
				SystemLogger::GetInstance()->LogText(L"AuthPipe", LEVEL_ERROR,
					L"Login - Duplicated. %d", accountNo);

				NetSessionID abnormalPlayer;
				server_->onlineMap_.Find(&abnormalPlayer, accountNo);

				//�ߺ� �α��� �߻� ��
				//���� ����, �� ���� �Ѵ� ���� ����
				server_->DisconnectAbnormalPlayer(abnormalPlayer);
				return false;
			}
			
			//Redis���� ���� Ű �а� Ȯ��
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

			//DB���� �г��� �о����
			WCHAR nickName[20] = {0};
			size_t ret = 0;
			MYSQL_ROW nickNameData = server_->AccountDBQuery(L"SELECT usernick FROM account WHERE accountno = %d", accountNo);
			mbstowcs_s(&ret, nickName, 20, nickNameData[0], 20);
			server_->AccountDBFreeResult();

			//Player ���� DB���� �а� ����
			//DB�� ����� ������ �ٷ� GamePipe�� ����
			//DB�� ����� ������, ���� ���� ��ȯ
			
			MYSQL_ROW characterRow = server_->GameDBQuery(L"SELECT * FROM gamedb.character WHERE accountno = %d", accountNo);
			if (!characterRow)
			{
				//ĳ���Ͱ� �������� ����
				//���� AuthPipe�� �ӹ���
				player->InitializePlayerData(accountNo, nickName);
				server_->proxy_->ResLogin(2, player->GetAccountNumber(), sessionID);
			}
			else
			{
				//ĳ���Ͱ� ������
			//���� ��ü�ؼ� �÷��̾� ���� ������
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

				//DB���� ���� ����� �Ҵ������ϱ�
				server_->GameDBFreeResult();

				//�ٷ� GamePipe�� �̵�
				server_->MovePipe(static_cast<NetUser*>(player), GAME_PIPE_ID);

				//�α��� �Ϸ� ��Ŷ ����
				server_->proxy_->ResLogin(1, player->GetAccountNumber(), sessionID);

			}
					
			return true;
		}

		bool ReqCharacterSelect(BYTE type, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//type�� 1-5�� �ϳ��� ��� ����
			//������ ����� ��� ����
			if (type >= 1 && type <= 5)
			{
				//ĳ����Ÿ�� ���� �ʱ�ȭ
				player->InitializeCharacterType(type);

				//ĳ���� ���� �α� �����
				WCHAR logTime[256] = { 0 };
				GetDateTime(logTime);

				server_->DBSave(eLogDB,
					L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, NULL, NULL, NULL, NULL)",
					logTime,
					player->GetAccountNumber(),
					3,
					32,
					type);

				//�ű� ĳ���� ���� DB�� ����
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
	
				//���� �������� �̵�
				server_->MovePipe(static_cast<NetUser*>(player), GAME_PIPE_ID);

				//GAME MODE ��Ŷ ����
				server_->proxy_->ResCharacterSelect(1, sessionID);

				return true;
			}
			else
			{
				//���� ��� �ݼ�
				//���� ���� ����
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
