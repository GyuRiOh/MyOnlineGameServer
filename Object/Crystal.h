#pragma once
#include "BaseObject.h"

class Crystal final : public BaseObject
{
public:
	Crystal(float X, float Y, int objectType, BYTE type, __int64 id) noexcept;

	const Crystal& operator= (const Crystal& object) noexcept 
	{
		BaseObject::operator=(object);
		return *this;
	}

	Crystal(const Crystal& other) noexcept : type_(other.type_), amount_(other.amount_), BaseObject(other) {}
	Crystal(Crystal&& other) noexcept;
	~Crystal() noexcept override;

	BYTE GetCrystalType() const noexcept { return type_; }
	BYTE GetCrystalAmount() const noexcept { return amount_; }

	void OnUpdate() noexcept override;
private:
	BYTE type_;
	BYTE amount_;
};