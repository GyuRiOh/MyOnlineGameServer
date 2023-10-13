#pragma once
#include <Windows.h>
#include "BaseObject.h"
#include "../NetRoot/NetServer/NetSessionID.h"

class Player final : public BaseObject 
{
	struct PlayerData
	{
		WCHAR nickName_[20];
		server_baby::NetSessionID clientID_;
		INT64 Exp_;
		int crystal_;
		int HP_;
		USHORT rotation_;
		BYTE characterType_;
	};

public:
	Player(float X, float Y, int objectType, __int64 objID, server_baby::NetSessionID clientID) noexcept;
	Player(Player&& other) noexcept;
	~Player();

	void OnUpdate() override;

private:
	PlayerData* data_;
};