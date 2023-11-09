#pragma once
#include "Object/Monster.h"
#include "OnlineGameServer/GamePipe.h"

class State;

namespace MyNetwork
{
	class MonsterContext
	{
	public:
		MonsterContext(Monster* monster, GamePipe* pipe);
		void ChangeState(State* newState);
		void Move();
	private:
		State* currentState_;
		GamePipe* originalPipe_;
		Monster* monster_;
	};



}
