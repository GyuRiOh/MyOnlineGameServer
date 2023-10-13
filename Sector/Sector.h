#pragma once
#include "../CommonProtocol.h"

enum DefaultSector
{
	X_DEFAULT = 30000,
	Y_DEFAULT = 30000
};

struct SectorPos
{
	short xPos_;
	short yPos_;

	SectorPos() : xPos_(X_DEFAULT), yPos_(Y_DEFAULT) {}
	SectorPos(short x, short y) : xPos_(x), yPos_(y) {}
	~SectorPos() {}

	bool operator==(const SectorPos& sector)
	{
		if ((xPos_ == sector.xPos_) && (yPos_ == sector.yPos_))
			return true;
		else
			return false;
	}

	void SetSector(int tileX, int tileY)
	{
		xPos_ = tileX / SECTOR_RATIO;
		yPos_ = tileY / SECTOR_RATIO;
	}
};

struct SectorAround
{
	char count_ = 0;
	SectorPos around_[9];
};
