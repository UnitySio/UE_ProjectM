#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "ShootingGame.h"

char sendData[] = "Hello World";

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{


		/*
		int32 menu = 0;
		while (true)
		{
			wcout << L"1: 회원가입, 2: 로그인" << endl;
			cin >> menu;
			switch (menu)
			{
				case 1:
				{
					Protocol::C_SIGNUP signupPkt;
					signupPkt.set_userid("test1");
					signupPkt.set_pw("test2");
					signupPkt.set_name("test3");
					signupPkt.set_phonenumber("0101234");
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(signupPkt);
					Send(sendBuffer);
					
					this_thread::sleep_for(1s);
					break;
				}
				default:
				{
					wcout << L"다시 입력하세요" << endl;
					break;
				}
			}
			
		}
		*/
		
		
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// TODO : packetId 대역 체크
		ServerPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		//cout << "Disconnected" << endl;
	}
};

int main()
{
	std::wcout.imbue(std::locale("kor"));
	ServerPacketHandler::Init();

	semMainLogic = CreateSemaphore(NULL, 1, 1, NULL);
	semInGame = CreateSemaphore(NULL, 0, 1, NULL);

	this_thread::sleep_for(1s);

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>, // TODO : SessionManager 등
		1000);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}
	
	GThreadManager->Launch([=]() {
		
		while (true)
		{
			if (!isGameStarted)
			{
				int32 menu = 0;
				cout << "Current login status : " << GSession << endl;
				wcout << L"현재 방 번호 : " << GRoomNum << endl;

				wcout << L"1: 로그인, 2:방만들기, 3: 방입장, 4: 준비, 5:시작" << endl;
				cin >> menu;
				switch (menu)
				{
				case 1:
				{
					Protocol::C_LOGIN loginPkt;
					string id, pw;
					wcout << L"ID를 입력하세요." << endl;
					cin >> id;
					wcout << L"PW를 입력하세요." << endl;
					cin >> pw;
					loginPkt.set_id(id);
					loginPkt.set_pw(pw);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
					service->Broadcast(sendBuffer);



					this_thread::sleep_for(1s);
					break;
				}
				case 2:
				{
					Protocol::C_CREATE_ROOM createRoomPkt;
					string roomName, pw;
					wcout << L"방 제목을 입력하세요." << endl;
					cin >> roomName;
					wcout << L"PW를 입력하세요." << endl;
					cin >> pw;
					createRoomPkt.set_roomname(roomName);
					createRoomPkt.set_pw(pw);

					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(createRoomPkt);
					service->Broadcast(sendBuffer);



					this_thread::sleep_for(1s);
					break;
				}
				case 3:
				{
					Protocol::C_ENTER_ROOM enterRoomPkt;
					int32 roomId;
					string pw;
					wcout << L"룸 아이디를 입력하세요." << endl;
					cin >> roomId;
					wcout << L"룸 패스워드를 입력하세요." << endl;
					cin >> pw;
					enterRoomPkt.set_roomcode(roomId);
					enterRoomPkt.set_pw(pw);

					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterRoomPkt);
					service->Broadcast(sendBuffer);



					this_thread::sleep_for(1s);
					break;
				}
				case 4:
				{
					Protocol::C_READY readyPkt;
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(readyPkt);
					service->Broadcast(sendBuffer);



					this_thread::sleep_for(1s);
					break;
				}
				case 5:
				{
					Protocol::C_START startPkt;
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(startPkt);
					service->Broadcast(sendBuffer);



					this_thread::sleep_for(1s);
					break;
				}
				default:
				{
					wcout << L"다시 입력하세요" << endl;
					this_thread::sleep_for(1s);
					break;
				}
				}

			}
			else
			{
				int32 menu = 0;
				wcout << L"현재 방 번호 : " << GRoomNum << endl;
				Protocol::C_MOVE movePkt;
				Protocol::C_ATTACK attackPkt;
				wcout << L"1: 위로 움직이기, 2: 아래로 움직이기,3: 왼쪽으로 움직이기,4: 오른쪽으로 움직이기" << endl;
				wcout << L"5. 오른쪽으로 발사, 6: 왼쪽으로 발사, 7:명중" << endl;
				cin >> menu;
				switch (menu)
				{
				case 1:
				{
					movePkt.set_playermovetype(Protocol::PlayerMoveType::PLAYER_MOVE_UP);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 2:
				{
					movePkt.set_playermovetype(Protocol::PlayerMoveType::PLAYER_MOVE_DOWN);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 3:
				{
					movePkt.set_playermovetype(Protocol::PlayerMoveType::PLAYER_MOVE_LEFT);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 4:
				{
					movePkt.set_playermovetype(Protocol::PlayerMoveType::PLAYER_MOVE_DOWN);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 5:
				{
					attackPkt.set_playerattacktype(Protocol::PlayerAttackType::PLAYER_ATTACK_LEFT);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(attackPkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 6:
				{
					attackPkt.set_playerattacktype(Protocol::PlayerAttackType::PLAYER_ATTACK_RIGHT);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(attackPkt);
					service->Broadcast(sendBuffer);
					break;
				}
				case 7:
				{
					wcout << L"명중한 플레이어 넘버를 입력하세요." << endl;
					uint64 playerNum;
					cin >> playerNum;
					Protocol::C_ATTACKED attackedPkt;
					attackedPkt.set_targetplayerid(playerNum);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(attackedPkt);
					service->Broadcast(sendBuffer);
					break;
				}
				default:
				{
					break;
				}

				this_thread::sleep_for(0.2s);
				}
			}
		}
			
	});

	
	
	


	GThreadManager->Join();
}