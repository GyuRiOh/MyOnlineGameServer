#include "Player.h"
#include <utility>
#include "../NetRoot/Common/SizedMemoryPool.h"


Player::Player(float X, float Y, int objectType, __int64 objID, server_baby::NetSessionID clientID) : data_(nullptr), BaseObject(X, Y, objectType, objID)
{	
	//data 세팅
	data_ = reinterpret_cast<PlayerData*>
		(server_baby::SizedMemoryPool::GetInstance()->Alloc(sizeof(PlayerData)));
}

Player::Player(Player&& other) noexcept : BaseObject(std::move(other)) 
{	
	data_ = other.data_;
	other.data_ = nullptr;	
}

Player::~Player()
{
	server_baby::SizedMemoryPool::GetInstance()->Free(data_);
}

void Player::OnUpdate()
{
	//이동 처리
}