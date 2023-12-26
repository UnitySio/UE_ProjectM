#pragma once
class ShootingGame
{
public:
	ShootingGame() {
        StartGame();
	}

private:
    void StartGame();
	
    
};



class Player
{
private:
	int32 _hp = 100;
	int32 _location_x;
	int32 _location_y;
	int32 _user_id;
};

class Bullet
{
private:
	int32 _dmg = 10;
	int32 _location_x;
	int32 _location_y;
	int32 _playerType;
};

extern HANDLE semMainLogic;
extern HANDLE semInGame;

extern bool isGameStarted;
extern Player me, enemy;
extern Vector<Player> players;