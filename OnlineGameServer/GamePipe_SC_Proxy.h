#pragma once
#include "../NetRoot/NetServer/NetPacket.h"
#include "../NetRoot/NetServer/NetSessionID.h"
#include "GamePipe.h"
#include "../CommonProtocol.h"

namespace MyNetwork
{
	class GamePipe_SC_Proxy final
	{
	public:
		explicit GamePipe_SC_Proxy(GamePipe* server) : server_(server) {}
	
		void ResCreateMyCharacter(INT64 clientID, BYTE charaType, const WCHAR* nickName, float posX, float posY, USHORT rotation, int crystal, int HP, INT64 Exp, USHORT level, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER);
			*msg << clientID;
			*msg << charaType;
			msg->EnqData((char*)nickName, 40);
			*msg << posX;
			*msg << posY;
			*msg << rotation;
			*msg << crystal;
			*msg << HP;
			*msg << Exp;
			*msg << level;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResCreateOtherCharacter(INT64 clientID, BYTE charaType, const WCHAR* nickName, float posX, float posY, USHORT rotation, USHORT level, BYTE respawn, BYTE sit, BYTE die, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER);
			*msg << clientID;
			*msg << charaType;
			msg->EnqData((char*)nickName, 40);
			*msg << posX;
			*msg << posY;
			*msg << rotation;
			*msg << level;
			*msg << respawn;
			*msg << sit;
			*msg << die;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResCreateOtherCharacter(INT64 clientID, BYTE charaType, const WCHAR* nickName, float posX, float posY, USHORT rotation, USHORT level, BYTE respawn, BYTE sit, BYTE die, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER);
			*msg << clientID;
			*msg << charaType;
			msg->EnqData((char*)nickName, 40);
			*msg << posX;
			*msg << posY;
			*msg << rotation;
			*msg << level;
			*msg << respawn;
			*msg << sit;
			*msg << die;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResCreateMonsterCharacter(INT64 clientID, float posX, float posY, USHORT rotation, BYTE respawn, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER);
			*msg << clientID;
			*msg << posX;
			*msg << posY;
			*msg << rotation;
			*msg << respawn;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResRemoveObject(INT64 clientID, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_REMOVE_OBJECT);
			*msg << clientID;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResRemoveObject(INT64 clientID, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_REMOVE_OBJECT);
			*msg << clientID;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResMoveCharacter(INT64 clientID, float posX, float posY, USHORT rotation, BYTE vkey, BYTE hkey, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_MOVE_CHARACTER);
			*msg << clientID;
			*msg << posX;
			*msg << posY;
			*msg << rotation;
			*msg << vkey;
			*msg << hkey;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResStopCharacter(INT64 clientID, float posX, float posY, USHORT rotation, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_STOP_CHARACTER);
			*msg << clientID;
			*msg << posX;
			*msg << posY;
			*msg << rotation;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResMoveMonster(INT64 clientID, float posX, float posY, USHORT rotation, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_MOVE_MONSTER);
			*msg << clientID;
			*msg << posX;
			*msg << posY;
			*msg << rotation;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResAttack1(INT64 clientID, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_ATTACK1);
			*msg << clientID;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResAttack2(INT64 clientID, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_ATTACK2);
			*msg << clientID;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResMonsterAttack(INT64 clientID, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_MONSTER_ATTACK);
			*msg << clientID;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResDamage(INT64 attackClientID, INT64 targetClientID, int damage, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_DAMAGE);
			*msg << attackClientID;
			*msg << targetClientID;
			*msg << damage;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResMonsterDie(INT64 monsterClientID, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_MONSTER_DIE);
			*msg << monsterClientID;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}


		void ResCreateCrystal(INT64 crystalClientID, BYTE crystalType, float posX, float posY, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_CRISTAL);
			*msg << crystalClientID;
			*msg << crystalType;
			*msg << posX;
			*msg << posY;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResCreateCrystal(INT64 crystalClientID, BYTE crystalType, float posX, float posY, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_CREATE_CRISTAL);
			*msg << crystalClientID;
			*msg << crystalType;
			*msg << posX;
			*msg << posY;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResPick(INT64 clientID, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_PICK);
			*msg << clientID;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResSit(INT64 clientID, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_SIT);
			*msg << clientID;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResPickCrystal(INT64 clientID, INT64 crystalClientID, int amountCrystal, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_PICK_CRISTAL);
			*msg << clientID;
			*msg << crystalClientID;
			*msg << amountCrystal;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResPlayerHP(INT64 hp, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_PLAYER_HP);
			*msg << hp;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResPlayerDie(INT64 clientID, int minusCrystal, NetSessionIDSet* sessionIDset)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_PLAYER_DIE);
			*msg << clientID;
			*msg << minusCrystal;

			server_->AsyncSendPacket(sessionIDset, msg);
			NetPacket::Free(msg);
		}

		void ResPlayerRestart(NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << static_cast<unsigned short>(en_PACKET_CS_GAME_RES_PLAYER_RESTART);

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
		GamePipe* server_;
	};
}
