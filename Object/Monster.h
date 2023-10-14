#pragma once
#include "BaseObject.h"

class Monster final : public BaseObject
{
public:
	Monster(float X, float Y, int objectType, __int64 id) noexcept;	

	const Monster& operator= (const Monster& object) noexcept;
	Monster(const Monster& other) noexcept : HP_(other.HP_), rotation_(other.rotation_), BaseObject(other) {}
	Monster(Monster&& other) noexcept;
	~Monster() noexcept override;

	void OnUpdate() noexcept override;
	void GetDamaged(int damage) noexcept;
	USHORT GetRotation() const noexcept { return rotation_; }

	bool isDead() { return (isDestroyReserved()); }
private:
	int HP_;
	USHORT rotation_;
};