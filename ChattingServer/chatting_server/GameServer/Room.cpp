#include "pch.h"
#include "Room.h"
#include "User.h"
#include "GameSession.h"

shared_ptr<Room> GRoom = make_shared<Room>();
map<uint64, RoomRef>* GRoomMap = new map<uint64, RoomRef>();
void Room::Enter(UserRef user)
{
	_users[user->userId] = user;
}

void Room::Leave(UserRef user)
{
	_users.erase(user->userId);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& p : _users)
	{
		p.second->ownerSession->Send(sendBuffer);
	}
}