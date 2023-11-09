
#ifndef TILE_POS
#define TILE_POS
#include "CommonProtocol.h"

struct TilePos
{
	float clientX = 0;
	float clientY = 0;
	int tileX = 0;
	int tileY = 0;

	TilePos() : tileX(0), tileY(0), clientX(0), clientY(0) {}

	TilePos(float X, float Y)
	{
		clientX = X;
		clientY = Y;
		tileX = static_cast<int>(clientX * 2);
		tileY = static_cast<int>(clientY * 2);
	}

	TilePos(const TilePos& pos)
	{
		clientX = pos.clientX;
		clientY = pos.clientY;
		tileX = pos.tileX;
		tileY = pos.tileY;
	}

	const TilePos operator= (const TilePos& other)
	{
		clientX = other.clientX;
		clientY = other.clientY;
		tileX = other.tileX;
		tileY = other.tileY;

		return *this;
	}

	void SetPos(float clientX, float clientY)
	{
		this->clientX = clientX;
		this->clientY = clientY;
		tileX = static_cast<int>(clientX * 2);
		tileY = static_cast<int>(clientY * 2);
	}

	void SetTile(TilePos& tile)
	{
		this->tileX = tile.tileX;
		this->tileY = tile.tileY;
		clientX = static_cast<float>(tileX / 2);
		clientY = static_cast<float>(tileY / 2);
	}

	void UpdateCilentPosByTilePos(int tileX, int tileY)
	{
		this->tileX = tileX;
		this->tileY = tileY;
		clientX = static_cast<float>(tileX / 2);
		clientY = static_cast<float>(tileY / 2);
	}

	double GetDistance(TilePos other)
	{
		return sqrt(pow(tileX-other.tileX, 2) + pow(tileY-other.tileY, 2));
	}

	static int GetInnerProduct(TilePos pos1, TilePos pos2)
	{
		return (pos1.tileX + pos2.tileX, pos1.tileY + pos2.tileY);
	}
};

#endif