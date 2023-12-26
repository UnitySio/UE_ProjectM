#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"

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
	ServerPacketHandler::Init();

	this_thread::sleep_for(1s);

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>, // TODO : SessionManager 등
		1);

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

	/*Protocol::C_CHAT chatPkt;
	chatPkt.set_msg(u8"Hello World !");*/
	
	//SignUp
	std::wcout.imbue(std::locale("kor"));


	int32 menu = 0;
	while (true)
	{
		cout << "Current login status : " << GSession << endl;
		wcout << L"현재 방 번호 : " << GRoomNum << endl;
		
		wcout << L"1: 회원가입, 2: 로그인, 3:채팅, 4.:방파기, 5: 방입장하기, 6: 밴하기" << endl;
		cin >> menu;
		switch (menu)
		{
		case 1:
		{
			Protocol::C_SIGNUP signupPkt;
			string id, pw, name, phonenumber;
			wcout << L"ID를 입력하세요." << endl;
			cin >> id;
			wcout << L"PW를 입력하세요." << endl;
			cin >> pw;
			wcout << L"name를 입력하세요." << endl;
			cin >> name;
			wcout << L"phonenumber를 입력하세요." << endl;
			cin >> phonenumber;
			signupPkt.set_userid(id);
			signupPkt.set_pw(pw);
			signupPkt.set_name(name);
			signupPkt.set_phonenumber(phonenumber);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(signupPkt);
			service->Broadcast(sendBuffer);

			this_thread::sleep_for(1s);
			break;
		}
		case 2:
		{
			Protocol::C_LOGIN loginPkt;
			string id, pw;
			wcout << L"ID를 입력하세요." << endl;
			cin >> id;
			wcout << L"PW를 입력하세요." << endl;
			cin >> pw;
			loginPkt.set_userid(id);
			loginPkt.set_pw(pw);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
			service->Broadcast(sendBuffer);

			this_thread::sleep_for(1s);
			break;
		}
		case 3:
		{
			Protocol::C_CHAT chatPkt;
			string msg;
			wcout << L"메세지를 입력하세요. " << endl;
			cin >> msg;
			chatPkt.set_msg(msg);
			chatPkt.set_loginsession(GSession);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(chatPkt);
			service->Broadcast(sendBuffer);

			this_thread::sleep_for(1s);
			break;
		}
		case 4:
		{
			Protocol::C_CREATE_ROOM createRoomPkt;
			string name, pw;
			wcout << L"방 제목을 입력하세요. " << endl;
			cin >> name;
			wcout << L"방 비밀번호를 입력하세요. " << endl;
			cin >> pw;
			createRoomPkt.set_roomname(name);
			createRoomPkt.set_roompw(pw);
			createRoomPkt.set_loginsession(GSession);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(createRoomPkt);
			service->Broadcast(sendBuffer);

			this_thread::sleep_for(1s);
			break;
		}
		case 5:
		{
			Protocol::C_ENTER_ROOM enterRoomPkt;
			wcout << L"방 번호를 입력하세요" << endl;
			uint64 roomNum;
			cin >> roomNum;
			wcout<< L"방 비밀번호를 입력하세요" << endl;
			string pw;
			cin >> pw;
			enterRoomPkt.set_roomnumber(roomNum);
			enterRoomPkt.set_roompw(pw);
			enterRoomPkt.set_loginsession(GSession);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterRoomPkt);
			service->Broadcast(sendBuffer);

			this_thread::sleep_for(1s);
			break;
		}
		case 6:
		{
			if (GRoomNum == 0)
			{
				wcout << L"로비에서는 밴을 할 수 없습니다." << endl;
				break;
			}
				
			Protocol::C_BAN_OTHER_USER banPkt;
			wcout << L"밴하고 싶은 유저의 id를 입력하세요." << endl;
			int32 ban;
			cin >> ban;
			banPkt.set_loginsession(GSession);
			banPkt.set_roomnumber(GRoomNum);
			banPkt.set_targetusernum(ban);
			auto sendBuffer = ServerPacketHandler::MakeSendBuffer(banPkt);
			service->Broadcast(sendBuffer);

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

	//Login
	/*
	for (int32 i = 0; i < 5; i++)
	{
		Protocol::C_LOGIN loginPkt;
		loginPkt.set_userid("test1");
		loginPkt.set_pw("test2");
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
		service->Broadcast(sendBuffer);

		this_thread::sleep_for(1s);
	}
	*/
	
	


	GThreadManager->Join();
}