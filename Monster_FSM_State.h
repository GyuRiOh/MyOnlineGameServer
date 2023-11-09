#pragma once

class State;
class Monster;

namespace MyNetwork
{
	class PipePlayer;
	class GamePipe;

	class MonsterContext
	{
	public:
		MonsterContext(Monster* monster);
		MonsterContext(Monster* monster, GamePipe* pipe);
		void Initialize(GamePipe* pipe);
		void ChangeState(int stateCode, PipePlayer* player);
		void Move(GamePipe* pipe);

		GamePipe* GetPipe() const noexcept { return originalPipe_; }
	private:
		State* currentState_;
		GamePipe* originalPipe_;
		Monster* monster_;
	};



}
