#include "Monster.h"
#include "../Monster_FSM_State.h"

float dx[3] = { -0.1 , 0, 0.1 };
using namespace MyNetwork;

Monster::Monster(float X, float Y, int objectType, __int64 id) noexcept : 
	HP_(200), rotation_(180), BaseObject(X, Y, objectType, id), context_(this){}

const Monster& Monster::operator=(const Monster& object) noexcept
{
	BaseObject::operator=(object);
	rotation_ = object.rotation_;
	HP_ = object.HP_;

	return *this;
}

Monster::Monster(Monster&& other) noexcept : context_(this), BaseObject(std::move(other))
{
	rotation_ = other.rotation_;
	HP_ = other.HP_;
}

Monster::~Monster() noexcept{}
void Monster::OnUpdate() noexcept {}

void Monster::GetDamaged(int damage) noexcept
{
	HP_ -= damage;
}

void Monster::Move(MyNetwork::GamePipe* pipe)
{
	context_.Move(pipe);
}

