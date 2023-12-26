#pragma once

class User
{
public:
	uint64					userId = 0;
	string					name;
	bool					isReady = false;
	int32					locationX;
	int32					locationY;
	int32					hp = 1000000;
	weak_ptr<GameSession>			ownerSession; // Cycle
};

