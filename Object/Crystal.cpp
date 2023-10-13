#include "Crystal.h"

Crystal::Crystal(float X, float Y, int objectType, BYTE type, __int64 id) noexcept 
	: type_(type), amount_(100), BaseObject(X, Y, objectType, id)
{
	
}

Crystal::Crystal(Crystal&& other) noexcept : type_(other.type_), BaseObject(std::move(other))
{

}

Crystal::~Crystal() noexcept
{

}

void Crystal::OnUpdate() noexcept
{
}
