#pragma once
#include "../NetRoot/NetServer/NetUser.h"
#include "../NetRoot/NetServer/NetSessionID.h"
#include "../Sector/Sector.h"
#include "TilePos.h"

constexpr int HP_RECOVERY = 1;
constexpr int HP_MAX = 5000;

namespace server_baby
{
	class PipePlayer : public NetUser
	{
	public:
		explicit PipePlayer(NetSessionID ID, NetRoot* server, NetPipeID curPipe) :
			accountNum_(NULL), clientID_(0), exp_(0), crystal_(0), sittingTime_(0), lastDBSaveTime_(GetTickCount64()),
			sitStart_(0), HP_(0), oldHP_(0), level_(0), rotation_(0), characterType_(0),
			oldSector_(), curSector_(), nickName_(), NetUser(ID, server, curPipe), isSitting_(false), isDead_(false)
		{ }

		~PipePlayer() {}

		INT64 GetAccountNumber() const noexcept { return accountNum_; }
		INT64 GetClientID() const noexcept { return clientID_; }
		const WCHAR* GetNickName() const noexcept { return nickName_; }
		SectorPos GetOldSector() const noexcept { return oldSector_; }
		SectorPos GetCurSector() const noexcept { return curSector_; }
		int GetCrystal() const noexcept { return crystal_; }
		int GetHP() const noexcept { return HP_; }
		int GetOldHP() const noexcept { return oldHP_; }
		bool isSitting() const noexcept { return isSitting_; }
		bool isDead() const noexcept { return isDead_; }
		BYTE GetCharacterType() const noexcept { return characterType_; }
		USHORT GetRotation() const noexcept { return rotation_; }
		float GetClientPosX() const noexcept { return tilePos_.clientX; }
		float GetClientPosY() const noexcept { return tilePos_.clientY; }
		int GetTileX() const noexcept { return tilePos_.tileX; }
		int GetTileY() const noexcept { return tilePos_.tileY; }
		INT64 GetExp() const noexcept { return exp_; }
		USHORT GetLevel() const noexcept { return level_; }
		ULONGLONG GetSittingTimeSec() const noexcept { return sittingTime_ / 1000; }
		ULONGLONG GetLastDBSaveTime() const noexcept { return lastDBSaveTime_; }

		void AddCrystal(unsigned int amount) noexcept { crystal_+= amount; }
		void SubCrystal(unsigned int num) noexcept { crystal_ -= num; }
		void Sit() noexcept { isSitting_ = true; sitStart_ = GetTickCount64(); }
		void SetRotation(USHORT rotation) noexcept { rotation_ = rotation; }
		void InitializeCharacterType(BYTE characterType) noexcept { characterType_ = characterType; }
		void InitializeInGameData(INT64 id) noexcept { clientID_ = id; rotation_ = 180; isDead_ = false; isSitting_ = false; sittingTime_ = 0;	sitStart_ = 0;}
		void ResetInGameData() noexcept { oldHP_ = 0; HP_ = HP_MAX / 2; }
		void UpdateLastDBSaveTime() noexcept { lastDBSaveTime_ = GetTickCount64(); }
		bool isSectorChanged() const noexcept { return !((curSector_.xPos_ == oldSector_.xPos_) && curSector_.yPos_ == oldSector_.yPos_); }

		void Damaged(int damage) noexcept { HP_ -= damage; }
		bool isFatal() const noexcept { return (HP_ <= 0); }
		void Kill() noexcept { isDead_ = true; }
		void Resurrect() noexcept;
		bool UpdateSector(float posX, float posY) noexcept;
		bool StandUp() noexcept;
		void AddExp(int exp) noexcept { exp_ += exp; }


		void InitializePlayerData(INT64 accountNo,  //DB에 이미 모든 데이터가 세팅된 경우
			const WCHAR* nickName,
			float posX,
			float posY,
			int crystal,
			int HP,
			int exp,
			int level,
			BYTE characterType) noexcept;
		
		void InitializePlayerData(INT64 accountNo,  //캐릭터를 새로 생성해야하는 경우, 일부만 초기화
			const WCHAR* nickName) noexcept;


	private: 
		WCHAR nickName_[20] = { 0 };
		INT64 accountNum_;
		INT64 clientID_;
		INT64 exp_;
		ULONGLONG sittingTime_;
		ULONGLONG sitStart_;
		ULONGLONG lastDBSaveTime_;
		int crystal_;
		int HP_;
		int oldHP_;
		USHORT rotation_;
		USHORT level_;
		BYTE characterType_;
		bool isSitting_;
		bool isDead_;

		TilePos tilePos_;
		SectorPos oldSector_;
		SectorPos curSector_;

	};

	inline bool PipePlayer::UpdateSector(float posX, float posY) noexcept
	{
		tilePos_.SetPos(posX, posY);

		oldSector_ = curSector_;
		curSector_.xPos_ = tilePos_.tileX / SECTOR_RATIO;
		curSector_.yPos_ = tilePos_.tileY / SECTOR_RATIO;

		return !(curSector_ == oldSector_);
	}

	inline void PipePlayer::Resurrect() noexcept
	{
		sittingTime_ = 0;
		sitStart_ = 0 ;
		rotation_ = 180;
		isDead_ = false;
		isSitting_ = false;	

		//좌표는 리스폰되는 초기위치로 설정...
		tilePos_.SetPos(47, 54);
		oldSector_ = curSector_;
		curSector_.SetSector(tilePos_.tileX, tilePos_.tileY);
	}

	inline bool PipePlayer::StandUp() noexcept
	{
		if (isSitting_)
		{
			//일어난 상태로 바꾸고, 앉아있던 시간 계산하기
			isSitting_ = false;
			sittingTime_ = (GetTickCount64() - sitStart_);
			sitStart_ = 0; 

			//앉아있던 시간 / 1000 * HP 회복수치 값을 HP에 더하기
			//현재의 회복수치는 5			
			oldHP_ = HP_;
			HP_ += (static_cast<int>(sittingTime_ / 1000) * 5);

			return true;
		}
		else
			return false;
	}

	inline void PipePlayer::InitializePlayerData(
		INT64 accountNo,  //DB에 이미 모든 데이터가 세팅된 경우
		const WCHAR* nickName,
		float posX,
		float posY,
		int crystal,
		int HP,
		int exp,
		int level,
		BYTE characterType) noexcept
	{
		accountNum_ = accountNo;
		wcscpy_s(nickName_, nickName);
		crystal_ = crystal;
		HP_ = HP;
		exp_ = exp;
		level_ = level;
		characterType_ = characterType;

		//좌표는 지정된 위치로
		tilePos_.SetPos(posX, posY);
		oldSector_ = SectorPos();
		curSector_.SetSector(tilePos_.tileX, tilePos_.tileY);
	}

	inline void PipePlayer::InitializePlayerData(INT64 accountNo,  //캐릭터를 새로 생성해야하는 경우, 일부만 초기화
		const WCHAR* nickName) noexcept
	{
		accountNum_ = accountNo;
		wcscpy_s(nickName_, nickName);
		crystal_ = 500;
		HP_ = HP_MAX / 2;
		exp_ = 0;
		level_ = 1;

		//좌표는 초기 리스폰 위치로
		tilePos_.SetPos(47.5f, 54.0f);
		oldSector_ = SectorPos();
		curSector_.SetSector(tilePos_.tileX, tilePos_.tileY);
	}

}
