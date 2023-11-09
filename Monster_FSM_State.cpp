
#include "Monster_FSM_State.h"
#include "OnlineGameServer/PipePlayer.h"
#include "OnlineGameServer/GamePipe.h"
#include "TileAround.h"

using namespace MyNetwork;

class IdleState;
class ChasingState;

class State
{
public:
	State(Monster* monster, MyNetwork::GamePipe* pipe) : monster_(monster), originalPipe_(pipe) {};
	virtual ~State() {}
	virtual void OnMove(MyNetwork::MonsterContext* constext) = 0;

protected:
	Monster* monster_ = nullptr;
	MyNetwork::GamePipe* originalPipe_ = nullptr;
};

class IdleState : public State
{
public:
	IdleState(Monster* monster, MyNetwork::GamePipe* pipe) : State(monster, pipe) {}

	virtual void OnMove(MyNetwork::MonsterContext* context) override
	{
		//주변 플레이어 정보를 알아야함
		//근처에서 player를 발견했다면
		//상태 전이
		PipePlayer* player = originalPipe_->GetNearestPlayer(monster_->GetTile());
		if (player)
		{
			ChasingState* state = reinterpret_cast<ChasingState*>(SizedMemoryPool::GetInstance()->Alloc(sizeof(ChasingState)));
			new (state) ChasingState(monster_, originalPipe_, player);
			state->OnMove(context);
			context->ChangeState(state);
			return;
		}

		//랜덤으로 움직임
		if ((rand() % 100) > 20)
			return;

		//이동 가능 범위 추리기
		TileAround tileAround;
		TileAround::GetTileAround(monster_->GetTileX(), monster_->GetTileY(), tileAround);

		//해당 범위에서 벽에 해당하는 타일 제외
		TilePos destPos = tileAround.around_[rand() % tileAround.count_];
		while (GetMap()[destPos.tileY][destPos.tileX] == 0)
		{
			destPos = tileAround.around_[rand() % tileAround.count_];
		}

		//타일 및 섹터 업데이트
		monster_->UpdateTileAndSector(destPos);
	}
};

class ChasingState : public State
{
public:
	ChasingState(Monster* monster, MyNetwork::GamePipe* pipe, PipePlayer* player) : player_(player), clientID_(player->GetClientID()), State(monster, pipe) {};

	virtual void OnMove(MyNetwork::MonsterContext* context) override
	{
		//플레이어가 없거나 죽은 상태면
		//기본 상태로 전이
		if (player_->isFatal() || player_->GetClientID() != clientID_)
		{
			IdleState* state = reinterpret_cast<IdleState*>(SizedMemoryPool::GetInstance()->Alloc(sizeof(IdleState)));
			new (state) IdleState(monster_, originalPipe_);
			state->OnMove(context);
			context->ChangeState(state);
			return;
		}

		//타일 좌표 얻기
		const int monsterX = monster_->GetTileX();
		const int monsterY = monster_->GetTileY();

		const int playerX = player_->GetTileX();
		const int playerY = player_->GetTileY();

		//기울기 계산
		double slope = (monsterY - playerY) / (monsterX - playerX);

		//해당 기울기를 가진 좌표 계산
		int destX = monsterX;
		int destY = monsterY;
		if (monsterX < playerX)
		{
			destX++;
			destY = LineEquation(destX, playerX, playerY, slope);
		}
		else if (monsterX > playerX)
		{
			destX--;
			destY = LineEquation(destX, playerX, playerY, slope);
		}
		else
		{
			if (monsterY < playerY)
				destY++;
			else if (monsterY > playerY)
				destY--;
		}

		TilePos destPos;
		destPos.UpdateCilentPosByTilePos(destX, destY);

		//타일 및 섹터 업데이트
		monster_->UpdateTileAndSector(destPos);
	}

private:
	//미사일 발사 시 사용하는 직선의 방정식. Y를 리턴함
	double LineEquation(int nowX, int startX, int startY, double slope)
	{
		return (slope) * (nowX - startX) + startY;
	}

	PipePlayer* player_;
	INT64 clientID_;
};

MyNetwork::MonsterContext::MonsterContext(Monster* monster, GamePipe* pipe) : monster_(monster), originalPipe_(pipe)
{
	IdleState* state = reinterpret_cast<IdleState*>(SizedMemoryPool::GetInstance()->Alloc(sizeof(IdleState)));
	new (state) IdleState(monster, pipe);
	currentState_ = state;
}


void MyNetwork::MonsterContext::ChangeState(State* newState)
{
	SizedMemoryPool::GetInstance()->Free(currentState_);
	currentState_ = newState;
}

void MyNetwork::MonsterContext::Move()
{
	currentState_->OnMove(this);
}
