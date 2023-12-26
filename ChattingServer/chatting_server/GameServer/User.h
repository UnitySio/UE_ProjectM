#pragma once

class User
{
public:
	int32					userPk = 0;
	uint64					userId = 0;
	string					name;
	Protocol::UserType	type = Protocol::USER_TYPE_NONE;
	GameSessionRef			ownerSession; // Cycle
};

