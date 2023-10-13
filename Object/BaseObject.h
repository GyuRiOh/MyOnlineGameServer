#ifndef __BASE_OBJECT__
#define __BASE_OBJECT__

#include "../NetRoot/Common/SizedMemoryPool.h"
#include "../Sector/Sector.h"
#include "TilePos.h"

enum ObjectType
{
	eMONSTER_TYPE = 1,
	eCRYSTAL_TYPE
};

class BaseObject
{
	struct ObjectData
	{
		TilePos pos_;
		SectorPos oldSector_;
		SectorPos curSector_;

		__int64 clientID_ = 0;
		int type_ = 0;
		bool destroyFlag_ = false;
	};

public:
	const BaseObject& operator= (const BaseObject& object) noexcept { data_ = object.data_; return *this;  }
	BaseObject(BaseObject&& object) noexcept : data_(object.data_) { object.data_ = nullptr; }
	BaseObject(const BaseObject& object) noexcept : data_(object.data_) { }
	BaseObject(float X, float Y, int objectType, __int64 id) noexcept : data_(nullptr) 
	{
		data_ = reinterpret_cast<ObjectData*>(server_baby::SizedMemoryPool::GetInstance()->Alloc(sizeof(ObjectData)));
		data_->clientID_ = id;
		data_->type_ = objectType;
		data_->destroyFlag_ = false;
		data_->pos_.SetPos(X, Y);

		data_->curSector_.SetSector(data_->pos_.tileX, data_->pos_.tileY);
	}
	
	virtual ~BaseObject() noexcept
	{
		if(data_)
			server_baby::SizedMemoryPool::GetInstance()->Free(data_);
	};

	virtual void OnUpdate(void) = 0;

	int GetType() noexcept { return data_->type_; }
	__int64 GetClientID() const noexcept { return data_->clientID_; }
	float GetClientPosX() const noexcept { return data_->pos_.clientX; }
	float GetClientPosY() const noexcept { return data_->pos_.clientY; }
	int GetTileX() const noexcept { return data_->pos_.tileX; }
	int GetTileY() const noexcept { return data_->pos_.tileY; }
	TilePos GetTile() const noexcept { return data_->pos_; }
	bool isDestroyReserved() const noexcept { return data_->destroyFlag_; }	
	bool isSectorChanged() const noexcept { return !(data_->curSector_ == data_->oldSector_); }
	SectorPos GetOldSector() const noexcept { return data_->oldSector_; }
	SectorPos GetCurSector() const noexcept { return data_->curSector_; }

	void ReserveDestroy() noexcept { data_->destroyFlag_ = true; }

	void UpdateTileAndSector(TilePos& tile) noexcept 
	{
		data_->pos_.SetTile(tile);
		data_->oldSector_ = data_->curSector_;
		data_->curSector_.SetSector(data_->pos_.tileX, data_->pos_.tileY);
	}

protected:
	ObjectData* data_;
};

#endif