#include "pch.h"
#include "ClientPacketHandler.h"
#include "User.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "Room.h"
#include "SessionVerificator.h"
PacketHandlerFunc GPacketHandler[UINT16_MAX];
string GSession;

static int32 locationGenerator(int32 max)
{
	
	return rand() % max;
}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_C_SIGNUP(PacketSessionRef& session, Protocol::C_SIGNUP& pkt)
{
	return false;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_LOGIN loginPkt;

	static Atomic<uint64> idGenerator = 0;

	SessionVerificator<Protocol::S_LOGIN> sessionVerificator = SessionVerificator<Protocol::S_LOGIN>::GetInstance();
	if (sessionVerificator.IsLogined(gameSession,loginPkt))
	{
		loginPkt.set_success(false);
	}
	else
	{
		UserRef user = MakeShared<User>();
		user->userId = idGenerator++;
		user->ownerSession = gameSession;
		gameSession->user = user;
		gameSession->_room = GRoom;
		GSessionManager.Add(gameSession);

		loginPkt.set_success(true);
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(loginPkt);
	session->Send(sendBuffer);

	return false;
}

bool Handle_C_CREATE_ROOM(PacketSessionRef& session, Protocol::C_CREATE_ROOM& pkt)
{
	Protocol::S_CREATE_ROOM createRoomPkt;
	SessionVerificator<Protocol::S_CREATE_ROOM> sessionVerificator = SessionVerificator<Protocol::S_CREATE_ROOM>::GetInstance();
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);
	srand((uint32)time(NULL));
	static Atomic<uint64> roomIdGenerator = 1;
	if (!sessionVerificator.IsLogined(gameSession,createRoomPkt))
	{
		createRoomPkt.set_success(false);
	}
	else if (sessionVerificator.IsLobby(gameSession,createRoomPkt))
	{
		uint64 roomId = roomIdGenerator++;
		string roomName = pkt.roomname();
		string pw = pkt.pw();

		RoomRef room = MakeShared<Room>();
		room->roomId = roomId;
		room->roomName = roomName;
		room->pw = pw;
		room->masterUserId = gameSession->user->userId;

		gameSession->user->ownerSession.lock()->_room = room;
		GRoomMap->insert({ roomId,room });
		GRoom->DoAsync(&Room::Leave, gameSession->user);
		room->DoAsync(&Room::Enter, gameSession->user);

		createRoomPkt.set_success(true);
		createRoomPkt.set_roomcode(roomId);
	}
	else
	{
		createRoomPkt.set_success(false);
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(createRoomPkt);
	session->Send(sendBuffer);

	return false;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_ENTER_ROOM enterRoomPkt;
	SessionVerificator<Protocol::S_ENTER_ROOM> sessionVerificator = SessionVerificator<Protocol::S_ENTER_ROOM>::GetInstance();
	if (!sessionVerificator.IsLogined(gameSession, enterRoomPkt))
	{
		enterRoomPkt.set_success(false);
	}
	else if (GRoomMap->find(pkt.roomcode()) != GRoomMap->end())
	{
		RoomRef room = GRoomMap->find(pkt.roomcode())->second;

		if (room->isStarted == true)
		{
			enterRoomPkt.set_code(9);
			enterRoomPkt.set_success(false);
		}
		else if (room->_users.size() >= room->maxUser)
		{
			enterRoomPkt.set_success(false);
			enterRoomPkt.set_code(10);
		}
		else if (room->pw == pkt.pw())
		{
			gameSession->user->ownerSession.lock()->_room = room;
			GRoom->DoAsync(&Room::Leave, gameSession->user);
			//동시에 2명 이상의 유저가 입장할 수 있으므로 JobQueue에 맡기지 않고 바로 실행
			room->Enter(gameSession->user);
			enterRoomPkt.set_success(true);
			enterRoomPkt.set_roomcode(pkt.roomcode());

			
		}
		
		else
		{
			enterRoomPkt.set_code(7);
			enterRoomPkt.set_success(false);
		}
		
	}
	else
	{
		enterRoomPkt.set_code(8);
		enterRoomPkt.set_success(false);
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterRoomPkt);
	session->Send(sendBuffer);

	return false;
}

bool Handle_C_READY(PacketSessionRef& session, Protocol::C_READY& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);
	
	Protocol::S_READY readyPkt;
	SessionVerificator<Protocol::S_READY> sessionVerificator = SessionVerificator<Protocol::S_READY>::GetInstance();

	if (!sessionVerificator.IsLogined(gameSession,readyPkt))
	{
		readyPkt.set_success(false);
	}
	//로비라면
	else if(sessionVerificator.IsLobby(gameSession, readyPkt))
	{
		readyPkt.set_success(false);
	}
	//방장이라면
	else if (sessionVerificator.IsMaster(gameSession,readyPkt))
	{
		readyPkt.set_success(false);
	}
	else
	{
		readyPkt.set_success(true);
		readyPkt.set_userid(gameSession->user->userId);
		gameSession->user->isReady = !gameSession->user->isReady;
		auto readyBroadcastBuffer = ClientPacketHandler::MakeSendBuffer(readyPkt);
		gameSession->_room.lock()->DoAsync(&Room::Broadcast, readyBroadcastBuffer);
		return false;
		
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(readyPkt);
	session->Send(sendBuffer);
	return false;
}

bool Handle_C_START(PacketSessionRef& session, Protocol::C_START& pkt)
{
	srand((uint32)time(NULL));
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_START startPkt;
	SessionVerificator<Protocol::S_START> sessionVerificator = SessionVerificator<Protocol::S_START>::GetInstance();
	//로그인이 되어 있다면
	if (!sessionVerificator.IsLogined(gameSession,startPkt))
	{
		startPkt.set_success(false);
	}
	//로비라면
	else if (sessionVerificator.IsLobby(gameSession,startPkt))
	{
		startPkt.set_success(false);
	}
	//방장이라면
	else if(sessionVerificator.IsMaster(gameSession,startPkt))
	{
		gameSession->_room.lock()->isStarted = true;
		startPkt.set_success(true);
		/*
		auto readyBroadcastBuffer = ClientPacketHandler::MakeSendBuffer(startPkt);
		gameSession->_room.lock()->DoAsync(&Room::Broadcast, readyBroadcastBuffer);
		*/

		//순회

		RoomRef startRoom = gameSession->_room.lock();
		auto iter = startRoom->_users.find(1);
		auto innerIter = startRoom->_users.find(1);
		int32 cnt = 0;
		Map<uint64, int32> randomNumMapX;
		Map<uint64, int32> randomNumMapY;
		for (iter = startRoom->_users.begin(); iter != startRoom->_users.end(); iter++)
		{
			randomNumMapX.insert({ iter->first,rand() % 1024 });
			randomNumMapY.insert({ iter->first,rand() % 1024 });
		}
		for (iter = startRoom->_users.begin(); iter != startRoom->_users.end(); iter++)
		{
				
			Protocol::S_START startBroadcastPkt;
			startBroadcastPkt.set_success(true);
			for (innerIter = startRoom->_users.begin(); innerIter != startRoom->_users.end(); innerIter++)
			{
				Protocol::Player* player = startBroadcastPkt.add_playerinfo();
				UserRef user = innerIter->second;
				player->set_id(innerIter->first);
				player->set_hp(1000000);
				player->set_locationx(randomNumMapX.find(innerIter->first)->second);
				player->set_locationy(randomNumMapY.find(innerIter->first)->second);
				user->hp = 100;
				user->locationX = randomNumMapX.find(innerIter->first)->second;
				user->locationY = randomNumMapY.find(innerIter->first)->second;
				//해당 유저가 마스터라면
				if (user->userId == startRoom->masterUserId)
				{
					player->set_playertype(Protocol::PlayerType::PLAYER_MASTER);
				}
				else
				{
					if (user->isReady == false)
					{
						startPkt.set_success(false);
						startPkt.set_code(11);
						auto sendBuffer = ClientPacketHandler::MakeSendBuffer(startPkt);
						session->Send(sendBuffer);
						return false;
					}
					player->set_playertype(Protocol::PlayerType::PLAYER_NONE_MASTER);
				}
				
			}

			startBroadcastPkt.set_yournum(iter->first);
			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(startBroadcastPkt);
			iter->second->ownerSession.lock()->Send(sendBuffer);

		}



		return false;
	}
	else
	{
		startPkt.set_success(false);
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(startPkt);
	session->Send(sendBuffer);
	return false;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_MOVE movePkt;
	SessionVerificator<Protocol::S_MOVE> sessionVerificator = SessionVerificator<Protocol::S_MOVE>::GetInstance();

	//로그인이 되어 있다면
	if (!sessionVerificator.IsLogined(gameSession, movePkt))
	{
		movePkt.set_success(false);
	}
	//로비라면
	else if (sessionVerificator.IsLobby(gameSession, movePkt))
	{
		movePkt.set_success(false);
	}
	//게임이 시작되지 않았다면
	else if (!sessionVerificator.isGameStarted(gameSession, movePkt))
	{
		movePkt.set_success(false);
	}
	else
	{
		movePkt.set_success(true);
		UserRef user = gameSession->user;
		Protocol::PlayerMoveType moveType = pkt.playermovetype();
		if (moveType == Protocol::PlayerMoveType::PLAYER_MOVE_LEFT)
		{
			user->locationX -= 1;
		}
		else if (moveType == Protocol::PlayerMoveType::PLAYER_MOVE_RIGHT)
		{
			user->locationX += 1;
		}
		else if (moveType == Protocol::PlayerMoveType::PLAYER_MOVE_UP)
		{
			user->locationY += 1;
		}
		else
		{
			user->locationY -= 1;
		}

		Protocol::Player* player = new Protocol::Player;
		player->set_id(user->userId);
		player->set_hp(user->hp);
		player->set_locationx(user->locationX);
		player->set_locationy(user->locationY);

		movePkt.set_allocated_playerinfo(player);

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);
		gameSession->_room.lock()->DoAsync(&Room::Broadcast, sendBuffer);
		return false;
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);
	session->Send(sendBuffer);

	
	

	return false;
}

bool Handle_C_ATTACK(PacketSessionRef& session, Protocol::C_ATTACK& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_ATTACK attackPkt;
	SessionVerificator<Protocol::S_ATTACK> sessionVerificator = SessionVerificator<Protocol::S_ATTACK>::GetInstance();


	//로그인이 되어 있다면
	if (!sessionVerificator.IsLogined(gameSession, attackPkt))
	{
		attackPkt.set_success(false);
	}
	//로비라면
	else if (sessionVerificator.IsLobby(gameSession, attackPkt))
	{
		attackPkt.set_success(false);
	}
	//게임이 시작되지 않았다면
	else if (!sessionVerificator.isGameStarted(gameSession, attackPkt))
	{
		attackPkt.set_success(false);
	}
	else
	{
		attackPkt.set_success(true);
		if (pkt.playerattacktype() == Protocol::PlayerAttackType::PLAYER_ATTACK_LEFT)
		{
			attackPkt.set_playerattacktype(Protocol::PlayerAttackType::PLAYER_ATTACK_LEFT);
		}
		else
		{
			attackPkt.set_playerattacktype(Protocol::PlayerAttackType::PLAYER_ATTACK_RIGHT);
		}
		Protocol::Player* player = new Protocol::Player;
		UserRef user = gameSession->user;
		player->set_id(user->userId);
		player->set_hp(user->hp);
		player->set_locationx(user->locationX);
		player->set_locationy(user->locationY);
		attackPkt.set_allocated_playerinfo(player);
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(attackPkt);
	gameSession->_room.lock()->DoAsync(&Room::Broadcast, sendBuffer);

	return false;
}

bool Handle_C_ATTACKED(PacketSessionRef& session, Protocol::C_ATTACKED& pkt)
{
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_ATTACKED attackedPkt;
	SessionVerificator<Protocol::S_ATTACKED> sessionVerificator = SessionVerificator<Protocol::S_ATTACKED>::GetInstance();

	//로그인이 되어 있다면
	if (!sessionVerificator.IsLogined(gameSession, attackedPkt))
	{
		attackedPkt.set_success(false);
	}
	//로비라면
	else if (sessionVerificator.IsLobby(gameSession, attackedPkt))
	{
		attackedPkt.set_success(false);
	}
	//게임이 시작되지 않았다면
	else if (!sessionVerificator.isGameStarted(gameSession, attackedPkt))
	{
		attackedPkt.set_success(false);
	}
	else
	{
		RoomRef room = gameSession->_room.lock();
		
		if (room->_users.find(pkt.targetplayerid()) != room->_users.end())
		{
			UserRef targetUser = room->_users.find(pkt.targetplayerid())->second;
			targetUser->hp -= 50;
			if (targetUser->hp<= 0)
			{
				
				Protocol::S_LOSE losePkt;
				losePkt.set_playerid(pkt.targetplayerid());
				losePkt.set_success(true);
				auto losePktSendBuffer = ClientPacketHandler::MakeSendBuffer(losePkt);
				room->DoAsync(&Room::Broadcast, losePktSendBuffer);
				
				room->Leave(targetUser);
				targetUser->ownerSession.lock()->_room = GRoom;
				GRoom->DoAsync(&Room::Enter, targetUser);
			}
			if (room->_users.size() == 1)
			{
				Protocol::S_WIN winPkt;
				winPkt.set_playerid(gameSession->user->userId);
				winPkt.set_success(true);
				auto winPktSendBuffer = ClientPacketHandler::MakeSendBuffer(winPkt);
				session->Send(winPktSendBuffer);
				room->Leave(gameSession->user);
				GRoom->DoAsync(&Room::Enter, gameSession->user);
				
			}
			attackedPkt.set_success(true);
			attackedPkt.set_attackplayerid(gameSession->user->userId);
			attackedPkt.set_targetplayerid(pkt.targetplayerid());
			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(attackedPkt);
			room->DoAsync(&Room::Broadcast, sendBuffer);
			return false;
		}
		else
		{
			attackedPkt.set_success(false);
			attackedPkt.set_code(14);
		}
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(attackedPkt);
	session->Send(sendBuffer);

	return false;
}
