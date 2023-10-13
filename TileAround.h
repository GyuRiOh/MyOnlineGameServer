#pragma once
#include "TilePos.h"
struct TileAround
{
	char count_ = 0;
	TilePos around_[9];

	static void GetTileAround(int tileXPos, int tileYPos, TileAround& tileAround)
	{
		int tileX = tileXPos;
		int tileY = tileYPos;

		tileX--;
		tileY--;

		tileAround.count_ = 0;

		for (int iCntY = 0; iCntY < 3; iCntY++)
		{

			if (tileY + iCntY < 0 || tileY + iCntY >= MAP_TILE_Y_MAX)
				continue;

			for (int iCntX = 0; iCntX < 3; iCntX++)
			{
				if (tileX + iCntX < 0 || tileX + iCntX >= MAP_TILE_X_MAX)
					continue;

				tileAround.around_[tileAround.count_].tileX = tileX + iCntX;
				tileAround.around_[tileAround.count_].tileY = tileY + iCntY;
				tileAround.count_++;
			}
		}

	}
};
