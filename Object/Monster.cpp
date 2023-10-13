#include "Monster.h"

float dx[3] = { -0.1 , 0, 0.1 };


Monster::Monster(float X, float Y, int objectType, __int64 id) noexcept : 
	HP_(200), rotation_(180), BaseObject(X, Y, objectType, id)
{
	
}

const Monster& Monster::operator=(const Monster& object) noexcept
{
	BaseObject::operator=(object);
	rotation_ = object.rotation_;
	HP_ = object.HP_;

	return *this;
}

Monster::Monster(Monster&& other) noexcept : BaseObject(std::move(other))
{
	rotation_ = other.rotation_;
	HP_ = other.HP_;
}

Monster::~Monster() noexcept
{

}

void Monster::OnUpdate() noexcept
{	

}

void Monster::GetDamaged(int damage) noexcept
{
	HP_ -= damage;
	if (HP_ <= 0)
		ReserveDestroy();
}
