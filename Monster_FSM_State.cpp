
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
		//�ֺ� �÷��̾� ������ �˾ƾ���
		//��ó���� player�� �߰��ߴٸ�
		//���� ����
		PipePlayer* player = originalPipe_->GetNearestPlayer(monster_->GetTile());
		if (player)
		{
			ChasingState* state = reinterpret_cast<ChasingState*>(SizedMemoryPool::GetInstance()->Alloc(sizeof(ChasingState)));
			new (state) ChasingState(monster_, originalPipe_, player);
			state->OnMove(context);
			context->ChangeState(state);
			return;
		}

		//�������� ������
		if ((rand() % 100) > 20)
			return;

		//�̵� ���� ���� �߸���
		TileAround tileAround;
		TileAround::GetTileAround(monster_->GetTileX(), monster_->GetTileY(), tileAround);

		//�ش� �������� ���� �ش��ϴ� Ÿ�� ����
		TilePos destPos = tileAround.around_[rand() % tileAround.count_];
		while (GetMap()[destPos.tileY][destPos.tileX] == 0)
		{
			destPos = tileAround.around_[rand() % tileAround.count_];
		}

		//Ÿ�� �� ���� ������Ʈ
		monster_->UpdateTileAndSector(destPos);
	}
};

class ChasingState : public State
{
public:
	ChasingState(Monster* monster, MyNetwork::GamePipe* pipe, PipePlayer* player) : player_(player), clientID_(player->GetClientID()), State(monster, pipe) {};

	virtual void OnMove(MyNetwork::MonsterContext* context) override
	{
		//�÷��̾ ���ų� ���� ���¸�
		//�⺻ ���·� ����
		if (player_->isFatal() || player_->GetClientID() != clientID_)
		{
			IdleState* state = reinterpret_cast<IdleState*>(SizedMemoryPool::GetInstance()->Alloc(sizeof(IdleState)));
			new (state) IdleState(monster_, originalPipe_);
			state->OnMove(context);
			context->ChangeState(state);
			return;
		}

		//Ÿ�� ��ǥ ���
		const int monsterX = monster_->GetTileX();
		const int monsterY = monster_->GetTileY();

		const int playerX = player_->GetTileX();
		const int playerY = player_->GetTileY();

		//���� ���
		double slope = (monsterY - playerY) / (monsterX - playerX);

		//�ش� ���⸦ ���� ��ǥ ���
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

		//Ÿ�� �� ���� ������Ʈ
		monster_->UpdateTileAndSector(destPos);
	}

private:
	//�̻��� �߻� �� ����ϴ� ������ ������. Y�� ������
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
