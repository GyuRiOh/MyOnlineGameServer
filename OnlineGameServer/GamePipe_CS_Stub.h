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
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//rotation을 변경
			player->SetRotation(rotation);

			//플레이어가 일어나게 만듬
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

			//좌표를 변경.필요시 섹터맵 Update
			//바뀐 섹터의 크리스탈, 몬스터 생성 패킷 날리기
			if(player->UpdateSector(posX, posY))
				server_->UpdateSector(player);
			
			//섹터 단위로 접속중인 사용자에게 Move패킷을 뿌림
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResMoveCharacter(clientID, posX, posY, rotation, vkey, hkey, idSet);

			return true;
		}

		bool ReqStopCharacter(INT64 clientID, float posX, float posY, USHORT rotation, NetSessionID sessionID)
		{
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//rotation을 변경
			player->SetRotation(rotation);

			//좌표를 변경.필요시 섹터맵 Update
			//바뀐 섹터의 크리스탈, 몬스터 생성 패킷 날리기
			if (player->UpdateSector(posX, posY))
				server_->UpdateSector(player);

			//섹터 단위로 접속중인 사용자에게 Stop패킷을 뿌림
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResStopCharacter(clientID, posX, posY, rotation, idSet);

			return true;
		}

		bool ReqAttack1(INT64 clientID, NetSessionID sessionID)
		{
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			std::vector<Monster*> damagedMonster;

			//맞은 이가 있는지 주변을 탐색
			//섹터 9개 전체에 데미지 들어가게 해보자
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

			//데미지 준 몬스터 가운데 
			//죽은 몬스터가 있다면 Exp를 올림
			
			//섹터 돌면서 데미지 패킷 쏘기
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
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			std::vector<Monster*> damagedMonster;

			//맞은 이가 있는지 주변을 탐색
			//섹터 9개 전체에 데미지 들어가게 해보자
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

			//섹터 돌면서 데미지 패킷 쏘기
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
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//크리스탈 포인터
			Crystal* pickedCrystal = nullptr;

			//섹터 단위로 접속중인 사용자에게 ResPick패킷을 뿌림
			//자신 제외
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResPick(clientID, idSet);

			//해당 클라이언트 주변에 획득 가능한 크리스탈 확인
			//먹을 수 있는 크리스탈이 있으면 해당 크리스탈을 획득
			//본 주변 패킷을 클라이언트 모두에게 뿌림 (행위 플레이어 포함)
			server_->objectManager_.ForeachSector(player->GetCurSector(),
				[this, &pickedCrystal, player](BaseObject* obj)
				{
					if ((pickedCrystal == nullptr) && (obj->GetType() == eCRYSTAL_TYPE))
					{
						//플레이어는 크리스탈 획득
						//크리스탈의 포인터를 받음
						pickedCrystal = static_cast<Crystal*>(obj);

						//플레이어 크리스탈에 더하기
						player->AddCrystal(pickedCrystal->GetCrystalAmount());

						//크리스탈은 사라짐
						pickedCrystal->Kill();
						
					}
				});

			if (pickedCrystal)
			{
				//DB에 크리스탈 획득 반영하기
				server_->DBSave(player, eGameDB, L"UPDATE gamedb.character SET crystal = %d WHERE accountno = %d",
					player->GetCrystal(),
					player->GetAccountNumber());

				//크리스탈 획득 로그 남기기
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

				//크리스탈 오브젝트 삭제 패킷 송신
				NetSessionIDSet* crystalRemoveIDSet = NetSessionIDSet::Alloc();
				server_->GetSessionIDSet_AroundSector(pickedCrystal->GetCurSector(), crystalRemoveIDSet);
				server_->proxy_->ResRemoveObject(
					pickedCrystal->GetClientID(),
					crystalRemoveIDSet);

				//자신 포함, 크리스탈 획득 패킷 송신
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
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));

			//앉은 캐릭터는 일정 시간마다 HP가 증가해야함
			//앉기 해제는 이 상태에서 Move패킷이 올 때
			player->Sit();
			 
			//섹터 단위로 접속중인 사용자에게 패킷을 뿌림
			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			server_->GetSessionIDSet_AroundSector_WithoutID(sessionID, player->GetCurSector(), idSet);
			server_->proxy_->ResSit(clientID, idSet);

			return true;

		}

		bool ReqPlayerRestart(NetSessionID sessionID)
		{ 
			//세션ID로 player검색
			PipePlayer* player = static_cast<PipePlayer*>(server_->FindUser(sessionID));
			
			//플레이어가 죽은 상태가 아니면 패킷 무시하기
			if (!player->isDead())
				return true;

			//섹터맵 업데이트 및
			//지정된 섹터에 업데이트 패킷 날리기
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
