#pragma once
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	// 싱글쓰레드 환경인마냥 코딩
	void Enter(UserRef user);
	void Leave(UserRef user);
	void Broadcast(SendBufferRef sendBuffer);


public:
	map<uint64, UserRef> _users;
	int32 roomId;
	wstring roomName;
	int32 masterId;
	wstring pw;
};

extern shared_ptr<Room> GRoom;
extern map<uint64, RoomRef>* GRoomMap;