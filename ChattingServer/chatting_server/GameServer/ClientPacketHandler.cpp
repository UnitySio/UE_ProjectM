#include "pch.h"
#include "ClientPacketHandler.h"
#include "User.h"
#include "Room.h"
#include "GameSession.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "GameSessionManager.h"
#include "MessageExporter.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];
string GSession;
// 직접 컨텐츠 작업자

static void stringToUTF16(const string& value , WCHAR* target)
{
	
	const char* str = value.c_str();
	int32 nLen = ::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), NULL, NULL);
	
	::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), target, nLen);
	target[nLen] = L'\0';
}

static bool sessionCheck(string loginSession)
{
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		auto query = L"SELECT COUNT(*) FROM [dbo].[account] WHERE login_session = (?)";
		DBBind<1, 1> dbSessionCheckBind(*dbConn, query);
		WCHAR* str = new WCHAR[100];
		stringToUTF16(loginSession, str);
		dbSessionCheckBind.BindParam(0, const_cast<WCHAR*>(str));
		int32 outCnt = 0;
		dbSessionCheckBind.BindCol(0, outCnt);
		ASSERT_CRASH(dbSessionCheckBind.Execute());
		dbConn->Fetch();
		delete[] str;
		GDBConnectionPool->Push(dbConn);
		if (outCnt == 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	

}

static string UUIDGenerator() {

	UUID uuid;
	UuidCreate(&uuid);
	string guid;
	RPC_CSTR szUuid = NULL;
	if (::UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
	{
		guid = reinterpret_cast<char*>(szUuid);
		::RpcStringFreeA(&szUuid);
	}
	return guid;
}

static string UTF16ToString(const WCHAR* value)
{
	wstring ws(value);
	return string(ws.begin(), ws.end());
}

static bool isExistedRandomNumber(int32 rand)
{
	DBConnection* dbConn = GDBConnectionPool->Pop();
	int32 cnt = 0;
	auto query = L"SELECT COUNT(*) FROM [dbo].[room] WHERE room_id = (?)";
	DBBind<1, 1> dbBind(*dbConn, query);
	dbBind.BindParam(0, rand);
	dbBind.BindCol(0, cnt);
	ASSERT_CRASH(dbBind.Execute());
	dbConn->Fetch();
	GDBConnectionPool->Push(dbConn);
	return cnt;

}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

bool Handle_C_SIGNUP(PacketSessionRef& session, Protocol::C_SIGNUP& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	// TODO : Validation check

	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		auto query = L"								\
				SELECT COUNT(*)						\
				FROM [dbo].[account]				\
				WHERE user_id = (?) OR phone_number = (?)";

		DBBind<2, 1> dbUserCheckBind(*dbConn, query);
		WCHAR* outUserid = new WCHAR[100];
		stringToUTF16(pkt.userid(), outUserid);
		dbUserCheckBind.BindParam(0, const_cast<WCHAR*>(outUserid));
		WCHAR* outPhoneNumber = new WCHAR[100];
		stringToUTF16(pkt.phonenumber(), outPhoneNumber);
		dbUserCheckBind.BindParam(1, const_cast<WCHAR*>(outPhoneNumber));
		

		int32 outCnt = 0;
		dbUserCheckBind.BindCol(0, OUT outCnt);

		ASSERT_CRASH(dbUserCheckBind.Execute());

		//count는 오로지 1행
		dbConn->Fetch();
		Protocol::S_SIGNUP signupPkt;
		GDBConnectionPool->Push(dbConn);

		if (!outCnt)
		{
			dbConn = GDBConnectionPool->Pop();
			
			// TODO : pw 암호화

			query = L"INSERT INTO [dbo].[account]([user_id],[pw],[name],[phone_number],[create_date],[user_type]) VALUES(?,?,?,?,CURRENT_TIMESTAMP,?)";
			DBBind<5, 0> dbInsertUserBind(*dbConn, query);
			
			WCHAR* userid = new WCHAR[100];
			stringToUTF16(pkt.userid(), userid);
			dbInsertUserBind.BindParam(0, const_cast<WCHAR*>(userid));
			WCHAR* pw = new WCHAR[100];
			stringToUTF16(pkt.pw(), pw);
			dbInsertUserBind.BindParam(1, const_cast<WCHAR*>(pw) );
			WCHAR* name = new WCHAR[100];
			stringToUTF16(pkt.name(), name);
			dbInsertUserBind.BindParam(2, const_cast<WCHAR*>(name) );
			WCHAR* phonenumber = new WCHAR[100];
			stringToUTF16(pkt.phonenumber(), phonenumber);
			dbInsertUserBind.BindParam(3, const_cast<WCHAR*>(phonenumber));
			//NONE = 0 / ADMIN = 1
			int32 userType = 0;
			dbInsertUserBind.BindParam(4, userType);
			ASSERT_CRASH(dbInsertUserBind.Execute());

			signupPkt.set_success(true);
			GDBConnectionPool->Push(dbConn);
			delete[] userid;
			delete[] pw;
			delete[] name;
			delete[] phonenumber;
		}
		else
		{
			signupPkt.set_success(false);
			signupPkt.set_reason(1);
		}


		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(signupPkt);
		session->Send(sendBuffer);
		delete[] outUserid;
		delete[] outPhoneNumber;
		
	}

	
	

	

	return true;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	Protocol::S_LOGIN loginPkt;

	// DB에서 플레이 정보를 긁어온다
	// GameSession에 플레이 정보를 저장 (메모리)

	// ID 발급 (DB 아이디가 아니고, 인게임 아이디)
	static Atomic<uint64> idGenerator = 0;
	
		
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		auto query = L"SELECT id, name, user_type, create_date, login_session FROM [dbo].[account] WHERE user_id = (?) AND pw = (?)";

		DBBind<2, 5> loginBind(*dbConn, query);

		WCHAR* userId = new WCHAR[100];
		WCHAR* pw = new WCHAR[100];
		int32 outId = 0;
		WCHAR outName[100] = L"";
		int32 outUserType = 0;
		WCHAR outLoginSession[100] = L"";
		TIMESTAMP_STRUCT outDateTime = {};
		stringToUTF16(pkt.userid(), userId);
		stringToUTF16(pkt.pw(), pw);

		loginBind.BindParam(0, const_cast<WCHAR*>(userId));
		loginBind.BindParam(1, const_cast<WCHAR*>(pw));

		loginBind.BindCol(0, OUT outId);
		loginBind.BindCol(1, OUT outName);
		loginBind.BindCol(2, OUT outUserType);
		loginBind.BindCol(3, OUT outDateTime);
		loginBind.BindCol(4, OUT outLoginSession);

		ASSERT_CRASH(loginBind.Execute());

		//같은 아이디와 전화번호를 가진 유저는 한 명만 가입 가능
		dbConn->Fetch();
		Protocol::User* user = new Protocol::User;
		string outNameStringVer = UTF16ToString(outName);
		GDBConnectionPool->Push(dbConn);

		if (wcscmp(outLoginSession,L"") > 0)
		{
			loginPkt.set_success(false);
			loginPkt.set_reason(2);
		}

		// TODO : Redis와 같은 인메모리 데이터베이스로 세션정보를 저장
		else if (outId != 0)
		{
			loginPkt.set_success(true);

			user->set_id(idGenerator++);
			user->set_name(outNameStringVer);
			user->set_usertype(static_cast<Protocol::UserType>(outUserType));
			loginPkt.set_allocated_user(user);

			UserRef userRef = MakeShared<User>();
			userRef->userPk = outId;
			userRef->userId = user->id();
			userRef->type = user->usertype();
			userRef->name = user->name();
			userRef->ownerSession = gameSession;

			gameSession->user = userRef;
			GSessionManager.Add(gameSession);

			dbConn = GDBConnectionPool->Pop();
			query = L"UPDATE [dbo].[account] SET login_session = (?) WHERE id = (?)";
			DBBind<2, 0> changeLoginInfoBind(*dbConn, query);

			string uuid = UUIDGenerator();
			WCHAR* uuidWstr = new WCHAR[100];
			stringToUTF16(uuid, uuidWstr);
			changeLoginInfoBind.BindParam(0, const_cast<WCHAR*>(uuidWstr));
			changeLoginInfoBind.BindParam(1, outId);

			ASSERT_CRASH(changeLoginInfoBind.Execute());
			
			loginPkt.set_session(uuid);

			GDBConnectionPool->Push(dbConn);
		}
		else
		{
			loginPkt.set_success(false);
			loginPkt.set_reason(1);
		}


		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(loginPkt);
		session->Send(sendBuffer);

		delete[] userId;
		delete[] pw;
		
		return true;
	}
		

	
	
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	Protocol::S_ENTER_GAME enterGamePkt;
	uint64 index = pkt.playerindex();
	string loginSession = pkt.loginsession();

	if (sessionCheck(loginSession) == true)
	{
		enterGamePkt.set_success(true);

		gameSession->_currentUser = gameSession->user; // READ_ONLY?
		gameSession->_room = GRoom;

		GRoom->DoAsync(&Room::Enter, gameSession->_currentUser);

		/*
		Protocol::S_CHAT chatPkt;
		const WCHAR* ipAddress = new WCHAR[100];
		ipAddress = gameSession->_currentUser->ownerSession->GetAddress().GetIpAddress().c_str();
		string ipAddressStringVer = UTF16ToString(ipAddress);
		wcout << ipAddress << endl;
		chatPkt.set_msg(ipAddressStringVer);
		auto ipAddressSendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

		GRoom->DoAsync(&Room::Broadcast, ipAddressSendBuffer);
		*/
		
	}
	else
	{
		enterGamePkt.set_success(false);
	}

	

	
	
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterGamePkt);
	gameSession->_currentUser->ownerSession->Send(sendBuffer);

	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	string loginSession = pkt.loginsession();
	Protocol::S_CHAT chatPkt;
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (sessionCheck(loginSession))
	{
		std::cout << pkt.msg() << endl;

		
		chatPkt.set_playerid(gameSession->_currentUser->userId);
		chatPkt.set_msg(pkt.msg());
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

		gameSession->_currentUser->ownerSession->_room.lock()->DoAsync(&Room::Broadcast, sendBuffer);
		UserRef user = gameSession->_currentUser;
		MessageStruct mStruct = { user->ownerSession->_room.lock()->roomId,pkt.msg(),user->userPk };
		GExporter->DoAsync(&MessageExporter::Push, mStruct);
		GExporter->DoTimer(6400, &MessageExporter::SaveAll);
	}
	else
	{
		chatPkt.set_playerid(-1);
		chatPkt.set_msg("First, login plz.");
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);
		session->Send(sendBuffer);
	}

	

	return true;
}

bool Handle_C_CREATE_ROOM(PacketSessionRef& session, Protocol::C_CREATE_ROOM& pkt)
{
	string loginSession = pkt.loginsession();
	Protocol::S_CREATE_ROOM createRoomPkt;
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (sessionCheck(loginSession)&&gameSession->_currentUser->ownerSession->_room.lock() == GRoom)
	{
		srand((uint32)time(NULL));

		int32 roomId = rand() % 100000;
		//중복되지 않는 room id 를 받아낼때까지 무한 루프
		while (isExistedRandomNumber(roomId))
			roomId = rand() % 100000;
		

		DBConnection * dbConn = GDBConnectionPool->Pop();

		WCHAR* roomName = new WCHAR[100];
		WCHAR* roomPw = new WCHAR[100];
		stringToUTF16(pkt.roomname(), roomName);
		stringToUTF16(pkt.roompw(), roomPw);
		
		int32 masterUserId = gameSession->_currentUser->userPk;

		
		

		auto query = L"INSERT INTO [dbo].[room]([room_id],[name],[pw],[master_user_id]) VALUES(?,?,?,?)";
		DBBind<4, 0> dbBind(*dbConn, query);
		dbBind.BindParam(0, roomId);
		dbBind.BindParam(1, const_cast<WCHAR*>(roomName));
		dbBind.BindParam(2, const_cast<WCHAR*>(roomPw));
		dbBind.BindParam(3, masterUserId);
		ASSERT_CRASH(dbBind.Execute());
		createRoomPkt.set_success(true);
		createRoomPkt.set_roomnumber(roomId);
		GDBConnectionPool->Push(dbConn);

		RoomRef room = MakeShared<Room>();
		room->roomId = roomId;
		room->roomName = roomName;
		room->pw = roomPw;
		room->masterId = masterUserId;
		gameSession->_currentUser->ownerSession->_room = room;
		GRoomMap->insert({ roomId, room });
		GRoom->DoAsync(&Room::Leave, gameSession->_currentUser);
		room->DoAsync(&Room::Enter, gameSession->_currentUser);

		delete[] roomName;
		delete[] roomPw;
	}
	else
	{
		createRoomPkt.set_success(false);
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(createRoomPkt);
	gameSession->_currentUser->ownerSession->Send(sendBuffer);

	return false;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
	string loginSession = pkt.loginsession();
	Protocol::S_ENTER_ROOM enterRoomPkt;
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	
	//Room Number가 존재하거나, 
	if (sessionCheck(loginSession))
	{
		int32 roomId = pkt.roomnumber();
		WCHAR* roomPw = new WCHAR[100];
		stringToUTF16(pkt.roompw(), roomPw);

		DBConnection* dbConn = GDBConnectionPool->Pop();
		auto query = L"SELECT COUNT(*) FROM [dbo].[room] WHERE room_id = (?) AND pw = (?)";
		DBBind<2, 1> dbBind(*dbConn, query);
		dbBind.BindParam(0, roomId);
		dbBind.BindParam(1, const_cast<WCHAR*>(roomPw));
		int32 outCnt = 0;
		dbBind.BindCol(0, OUT outCnt);

		ASSERT_CRASH(dbBind.Execute());
		dbConn->Fetch();
		GDBConnectionPool->Push(dbConn);

		if (outCnt == 0)
		{
			enterRoomPkt.set_success(false);
		}
		else
		{
			// 찾은 경우
			if (GRoomMap->find(roomId) != GRoomMap->end())
			{
				RoomRef room = GRoomMap->find(roomId)->second;
				gameSession->_currentUser->ownerSession->_room = room;
				UserRef user = gameSession->_currentUser;
				room->DoAsync(&Room::Enter, user);
				GRoom->DoAsync(&Room::Leave, user);

			}
			// Room의 정보는 DB에 남아있으나, 서버 재부팅 등의 이유로 세션 자체가 사라진 경우
			else
			{
				DBConnection* dbConn = GDBConnectionPool->Pop();
				auto query = L"SELECT name,pw,master_user_id FROM [dbo].[room] WHERE room_id = (?)";
				DBBind<1, 4> dbBind(*dbConn, query);
				dbBind.BindParam(0,roomId);
				WCHAR name[100];
				WCHAR pw[100];
				int32 masterUserId = 0;
				dbBind.BindCol(0, name);
				dbBind.BindCol(1, pw);
				dbBind.BindCol(2, masterUserId);
				ASSERT_CRASH(dbBind.Execute());
				dbConn->Fetch();
				GDBConnectionPool->Push(dbConn);

				RoomRef room = MakeShared<Room>();
				room->roomId = roomId;
				room->roomName = name;
				room->pw = pw;
				room->masterId = masterUserId;
				GRoomMap->insert({ roomId,room });
				GRoom->DoAsync(&Room::Leave, gameSession->_currentUser);
				room->DoAsync(&Room::Enter, gameSession->_currentUser);
				

			}
			enterRoomPkt.set_success(true);
			enterRoomPkt.set_roomnumber(pkt.roomnumber());
		}

		
	}
	else
	{
		enterRoomPkt.set_success(false);
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterRoomPkt);
	gameSession->_currentUser->ownerSession->Send(sendBuffer);

	return false;
}

bool Handle_C_BAN_OTHER_USER(PacketSessionRef& session, Protocol::C_BAN_OTHER_USER& pkt)
{
	string loginSession = pkt.loginsession();
	Protocol::S_BAN_OTHER_USER banPkt;
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (sessionCheck(loginSession))
	{
		//현재 세션의 pk가 master id라면 강퇴 권한이 있음
		if (gameSession->_currentUser->userPk == gameSession->_currentUser->ownerSession->_room.lock()->masterId)
		{
			UserRef targetUser = gameSession->_currentUser->ownerSession->_room.lock()->_users[pkt.targetusernum()];
			gameSession->_currentUser->ownerSession->_room.lock()->DoAsync(&Room::Leave, targetUser);
			GRoom->DoAsync(&Room::Enter, targetUser);
			targetUser->ownerSession->_room = GRoom;
			banPkt.set_success(true);
			Protocol::S_IS_BANNED bannedPkt;
			bannedPkt.set_roomnumber(gameSession->_currentUser->ownerSession->_room.lock()->roomId);
			auto noticeBuffer = ClientPacketHandler::MakeSendBuffer(bannedPkt);
			targetUser->ownerSession->Send(noticeBuffer);
		}
		else
		{
			banPkt.set_success(false);
		}
	}
	else
	{
		banPkt.set_success(false);
	}
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(banPkt);
	gameSession->_currentUser->ownerSession->Send(sendBuffer);
	return false;
}
