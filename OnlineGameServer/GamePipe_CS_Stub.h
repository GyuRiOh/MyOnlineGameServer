#pragma once
#include <Windows.h>
#include "../NetRoot/NetServer/NetSessionID.h"
#include "../NetRoot/NetServer/NetPacketSet.h"
#include "../NetRoot/Common/RPCBuffer.h"
#include "../NetRoot/NetServer/NetStub.h"
#include "GamePipe_SC_Proxy.h"
#include "GamePipe.h"
#include "../DBSaveJob.h"
#include "GameServer.h"

namespace server_baby
{
	class GamePipe_CS_Stub final : public NetStub
	{
	public:
		explicit GamePipe_CS_Stub(GamePipe* server) : server_(server) {}

		bool PacketProc(NetSessionID sessionID, NetDummyPacket* msg) override
		{
			WORD type;
			*msg >> type;
			switch (type)
			{
			case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER:
			{
				INT64 clientID;
				float posX;
				float posY;
				USHORT rotation;
				BYTE vkey;
				BYTE hkey;
				*msg >> clientID;
				*msg >> posX;
				*msg >> posY;
				*msg >> rotation;
				*msg >> vkey;
				*msg >> hkey;
				return ReqMoveCharacter(clientID, posX, posY, rotation, vkey, hkey, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_STOP_CHARACTER:
			{
				INT64 clientID;
				float posX;
				float posY;
				USHORT rotation;
				*msg >> clientID;
				*msg >> posX;
				*msg >> posY;
				*msg >> rotation;
				return ReqStopCharacter(clientID, posX, posY, rotation, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_ATTACK1:
			{
				INT64 clientID;
				*msg >> clientID;
				return ReqAttack1(clientID, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_ATTACK2:
			{
				INT64 clientID;
				*msg >> clientID;
				return ReqAttack2(clientID, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_PICK:
			{
				INT64 clientID;
				*msg >> clientID;
				return ReqPick(clientID, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_SIT:
			{
				INT64 clientID;
				*msg >> clientID;
				return ReqSit(clientID, sessionID);
			}
			case en_PACKET_CS_GAME_REQ_PLAYER_RESTART:
			{
				return ReqPlayerRestart(sessionID);
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

		
		bool ReqMoveCharacter(INT64 clientID, float posX, float posY, USHORT rotation, BYTE vkey, BYTE hkey, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//rotation�� ����
			player->SetRotation(rotation);

			//�÷��̾ �Ͼ�� ����
			if (player->StandUp())
			{
				int oldHP = player->GetOldHP();
				int newHP = player->GetHP();
				int sitTimeSec = player->GetSittingTimeSec();

				WCHAR logTime[256] = { 0 };
				GetDateTime(logTime);

				server_->LogDBQuery(
					L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, %d, NULL, NULL)",
					logTime,
					player->GetAccountNumber(),
					5,
					51,
					oldHP,
					newHP,
					sitTimeSec);

				server_->proxy_->ResPlayerHP(
					player->GetHP(),
					sessionID);
			}

			//��ǥ�� ����.�ʿ�� ���͸� Update
			//�ٲ� ������ ũ����Ż, ���� ���� ��Ŷ ������
			if(player->UpdateSector(posX, posY))
				server_->UpdateSector(player);
			
			//���� ������ �������� ����ڿ��� Move��Ŷ�� �Ѹ�
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResMoveCharacter(clientID, posX, posY, rotation, vkey, hkey, idSet);

			return true;
		}

		bool ReqStopCharacter(INT64 clientID, float posX, float posY, USHORT rotation, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//rotation�� ����
			player->SetRotation(rotation);

			//��ǥ�� ����.�ʿ�� ���͸� Update
			//�ٲ� ������ ũ����Ż, ���� ���� ��Ŷ ������
			if (player->UpdateSector(posX, posY))
				server_->UpdateSector(player);

			//���� ������ �������� ����ڿ��� Stop��Ŷ�� �Ѹ�
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResStopCharacter(clientID, posX, posY, rotation, idSet);

			return true;
		}

		bool ReqAttack1(INT64 clientID, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			std::vector<Monster*> damagedMonster;

			//���� �̰� �ִ��� �ֺ��� Ž��
			//���� 9�� ��ü�� ������ ���� �غ���
			server_->objectManager_.ForeachSector(
				player->GetCurSector(),
				[&damagedMonster](BaseObject* obj)
				{
					if (obj->GetType() == eMONSTER_TYPE)
					{
						
						Monster* monster = static_cast<Monster*>(obj);

						monster->GetDamaged(5);

						damagedMonster.emplace_back(monster);
					}
				}
			);

			//������ �� ���� ��� 
			//���� ���Ͱ� �ִٸ� Exp�� �ø�
			
			//���� ���鼭 ������ ��Ŷ ���
			for (auto monster : damagedMonster)
			{
				if (monster->isDead())
					player->AddExp(10);

				NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
				server_->GetSessionIDSet_AroundSector(monster->GetCurSector(), idSet);
				server_->proxy_->ResDamage(
					clientID,
					monster->GetClientID(),
					5,
					idSet
				);
			}

			return true;
		}

		bool ReqAttack2(INT64 clientID, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			std::vector<Monster*> damagedMonster;

			//���� �̰� �ִ��� �ֺ��� Ž��
			//���� 9�� ��ü�� ������ ���� �غ���
			server_->objectManager_.ForeachSectorAround(
				player->GetCurSector(),
				[&damagedMonster](BaseObject* obj)
				{
					if (obj->GetType() == eMONSTER_TYPE)
					{
						Monster* monster = static_cast<Monster*>(obj);
						monster->GetDamaged(20);
						damagedMonster.emplace_back(monster);
					}
				}
			);

			//���� ���鼭 ������ ��Ŷ ���
			for (auto monster : damagedMonster)
			{
				if (monster->isDead())
					player->AddExp(10);

				NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
				server_->GetSessionIDSet_AroundSector(monster->GetCurSector(), idSet);
				server_->proxy_->ResDamage(
					clientID,
					monster->GetClientID(),
					20,
					idSet
				);
			}

			return true;
		}

		bool ReqPick(INT64 clientID, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//ũ����Ż ������
			Crystal* pickedCrystal = nullptr;

			//���� ������ �������� ����ڿ��� ResPick��Ŷ�� �Ѹ�
			//�ڽ� ����
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResPick(clientID, idSet);

			//�ش� Ŭ���̾�Ʈ �ֺ��� ȹ�� ������ ũ����Ż Ȯ��
			//���� �� �ִ� ũ����Ż�� ������ �ش� ũ����Ż�� ȹ��
			//�� �ֺ� ��Ŷ�� Ŭ���̾�Ʈ ��ο��� �Ѹ� (���� �÷��̾� ����)
			server_->objectManager_.ForeachSector(player->GetCurSector(),
				[this, &pickedCrystal, player](BaseObject* obj)
				{
					if ((pickedCrystal == nullptr) && (obj->GetType() == eCRYSTAL_TYPE))
					{
						//�÷��̾�� ũ����Ż ȹ��
						//ũ����Ż�� �����͸� ����
						pickedCrystal = static_cast<Crystal*>(obj);

						//�÷��̾� ũ����Ż�� ���ϱ�
						player->AddCrystal(pickedCrystal->GetCrystalAmount());

						//ũ����Ż�� �����
						pickedCrystal->Kill();
						
					}
				});

			if (pickedCrystal)
			{
				//DB�� ũ����Ż ȹ�� �ݿ��ϱ�
				server_->DBSave(player, eGameDB, L"UPDATE gamedb.character SET crystal = %d WHERE accountno = %d",
					player->GetCrystal(),
					player->GetAccountNumber());

				//ũ����Ż ȹ�� �α� �����
				WCHAR logTime[256] = { 0 };
				GetDateTime(logTime);

				server_->LogDBQuery(
					L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, NULL, NULL, NULL)",
					logTime,
					player->GetAccountNumber(),
					4,
					41,
					pickedCrystal->GetCrystalAmount(),
					player->GetCrystal());

				//ũ����Ż ������Ʈ ���� ��Ŷ �۽�
				NetSessionIDSet* crystalRemoveIDSet = NetSessionIDSet::Alloc();
				server_->GetSessionIDSet_AroundSector(pickedCrystal->GetCurSector(), crystalRemoveIDSet);
				server_->proxy_->ResRemoveObject(
					pickedCrystal->GetClientID(),
					crystalRemoveIDSet);

				//�ڽ� ����, ũ����Ż ȹ�� ��Ŷ �۽�
				NetSessionIDSet* crystalAcquireIDSet = NetSessionIDSet::Alloc();
				server_->GetSessionIDSet_AroundSector(pickedCrystal->GetCurSector(), crystalAcquireIDSet);
				server_->proxy_->ResPickCrystal(
					clientID,
					pickedCrystal->GetClientID(),
					player->GetCrystal(),
					crystalAcquireIDSet);

			}
			
			return true;

		}

		bool ReqSit(INT64 clientID, NetSessionID sessionID)
		{
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//���� ĳ���ʹ� ���� �ð����� HP�� �����ؾ���
			//�ɱ� ������ �� ���¿��� Move��Ŷ�� �� ��
			player->Sit();
			 
			//���� ������ �������� ����ڿ��� ��Ŷ�� �Ѹ�
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResSit(clientID, idSet);

			return true;

		}

		bool ReqPlayerRestart(NetSessionID sessionID)
		{ 
			//����ID�� player�˻�
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));
			
			//�÷��̾ ���� ���°� �ƴϸ� ��Ŷ �����ϱ�
			if (!player->isDead())
				return true;

			//���͸� ������Ʈ ��
			//������ ���Ϳ� ������Ʈ ��Ŷ ������
			server_->RestartGame(player);

			return true;
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
		GamePipe* server_;
	};
}
