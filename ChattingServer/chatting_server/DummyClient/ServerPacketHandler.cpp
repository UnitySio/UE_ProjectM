#include "pch.h"
#include "ServerPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];
string GSession;
int32 GRoomNum;
// 직접 컨텐츠 작업자

static void stringToUTF16(const string& value, WCHAR* target)
{

	const char* str = value.c_str();
	int32 nLen = ::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), NULL, NULL);

	::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), target, nLen);
	target[nLen] = L'\0';
}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

bool Handle_S_SIGNUP(PacketSessionRef& session, Protocol::S_SIGNUP& pkt)
{
	std::wcout.imbue(std::locale("kor"));
	if (pkt.success())
	{
		wcout << L"회원가입에 성공하셨습니다." << endl;
	}
	else
	{
		if (pkt.reason() == 1)
		{
			wcout << L"중복 회원가입입니다." << endl;
		}
		else
		{
			wcout << L"알 수 없는 이유로 회원가입에 실패하셨습니다." << endl;
		}
		
		
		
	}
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	std::wcout.imbue(std::locale("kor"));
	if (pkt.success() == false)
	{
		if (pkt.reason() == 1)
		{
			wcout << L"아이디 또는 비밀번호가 일치하지 않습니다." << endl;
		}
		else if (pkt.reason() == 2)
		{
			wcout << L"중복 로그인입니다." << endl;
		}
		else
		{
			wcout << L"알 수 없는 이유로 로그인에 실패하였습니다." << endl;
		}
	}
	else
	{
		wcout << L"로그인에 성공하였습니다. 유저 정보는," << endl;
		cout << pkt.user().name() << endl;
		cout << pkt.user().id() << endl;
		cout << pkt.user().usertype() << endl;
		cout << pkt.session() << endl;
		wcout << L"입니다." << endl;
		GSession = pkt.session();
		Protocol::C_ENTER_GAME enterGamePkt;
		enterGamePkt.set_playerindex(pkt.user().id());
		enterGamePkt.set_loginsession(GSession);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		session->Send(sendBuffer);
		
	}


	//if (pkt.success() == false)
	//	return true;

	//if (pkt.players().size() == 0)
	//{
	//	// 캐릭터 생성창
	//}

	//// 입장 UI 버튼 눌러서 게임 입장
	//Protocol::C_ENTER_GAME enterGamePkt;
	//enterGamePkt.set_playerindex(0); // 첫번째 캐릭터로 입장
	//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
	//session->Send(sendBuffer);

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	
	if (pkt.success())
	{
		wcout << L"로그인 성공" << endl;
		
	}
	else
	{
		wcout << L"로그인 실패" << endl;
	}
	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	cout << pkt.playerid() << " : " << pkt.msg() << endl;
	return true;
}

bool Handle_S_CREATE_ROOM(PacketSessionRef& session, Protocol::S_CREATE_ROOM& pkt)

{
	std::wcout.imbue(std::locale("kor"));
	if (pkt.success())
	{
		GRoomNum = pkt.roomnumber();
		wcout << L"성공 " << endl;
	}
	else
	{
		wcout << L"로그인 하지 않았거나, 현재 방을 파지 않았습니다." << endl;
	}
	return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
{
	std::wcout.imbue(std::locale("kor"));
	if (pkt.success())
	{
		GRoomNum = pkt.roomnumber();
		wcout << L"성공 " << endl;
	}
	else
	{
		wcout << L"실패" << endl;
	}
	return false;
}

bool Handle_S_BAN_OTHER_USER(PacketSessionRef& session, Protocol::S_BAN_OTHER_USER& pkt)
{
	std::wcout.imbue(std::locale("kor"));
	if (pkt.success())
	{
		wcout << L"성공 " << endl;
	}
	else
	{
		wcout << L"실패" << endl;
	}
	return false;
	
}

bool Handle_S_IS_BANNED(PacketSessionRef& session, Protocol::S_IS_BANNED& pkt)
{
	std::wcout.imbue(std::locale("kor"));
	wcout << L"당신은" << pkt.roomnumber() << L"번 방에서 강퇴되었습니다." << endl;
	GRoomNum = 0;
	return false;
}
