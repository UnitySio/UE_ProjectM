#include "pch.h"
#include "ServerPacketHandler.h"
#include "ErrorController.h"
#include "ShootingGame.h"
//
PacketHandlerFunc GPacketHandler[UINT16_MAX];
string GSession;
int32 GRoomNum;
uint64 mynum;

ErrorController errorController = ErrorController::GetInstance();

static int32 GcurrentHp = 100;

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}


bool Handle_S_SIGNUP(PacketSessionRef& session, Protocol::S_SIGNUP& pkt)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	if (pkt.success())
	{
		cout << "Success" << endl;
	}
	else
	{
		
		cout <<"Fail" << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_CREATE_ROOM(PacketSessionRef& session, Protocol::S_CREATE_ROOM& pkt)
{
	if (pkt.success())
	{
		cout << "Success" << endl;
		GRoomNum = pkt.roomcode();
	}
	else
	{
		cout << "Fail" << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
{
	if (pkt.success())
	{
		cout << "Success" << endl;
		GRoomNum = pkt.roomcode();
	}
	else
	{
		cout << "Fail" << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_READY(PacketSessionRef& session, Protocol::S_READY& pkt)
{
	if (pkt.success())
	{
		cout << "Success" << endl;
		cout << pkt.userid() << "is ready" << endl;
	}
	else
	{
		cout << "Fail" << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_START(PacketSessionRef& session, Protocol::S_START& pkt)
{
	if (pkt.success())
	{
		cout << "Success" << endl;
		cout << "Game Started " << endl;
		isGameStarted = true;
		cout << "Your player num is " << pkt.yournum() << endl;
		mynum = pkt.yournum();
		GcurrentHp = 100;
		const google::protobuf::RepeatedPtrField<Protocol::Player> players = pkt.playerinfo();
		for (int32 i = 0; i < players.size(); i++)
		{
			Protocol::Player player = players.Get(i);
			cout << i + 1 << "st player id : " << player.id() << endl;
			cout << i + 1 << "st player hp : " << player.hp() << endl;
			cout << i + 1 << "st player x : " << player.locationx() << endl;
			cout << i + 1 << "st player y : " << player.locationy() << endl;

		}
	}
	else
	{
		cout << "Fail" << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (pkt.success())
	{
		Protocol::Player player = pkt.playerinfo();
		cout << "player id is " << player.id() << endl;
		cout << "player hp is " << player.hp() << endl;
		cout << "player x is " << player.locationx() << endl;
		cout << "player y is " << player.locationy() << endl;
	}
	else
	{
		cout << "Fail " << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_ATTACK(PacketSessionRef& session, Protocol::S_ATTACK& pkt)
{
	if (pkt.success())
	{
		Protocol::Player player = pkt.playerinfo();
		cout << "player id is " << player.id() << endl;
		cout << "player hp is " << player.hp() << endl;
		cout << "player x is " << player.locationx() << endl;
		cout << "player y is " << player.locationy() << endl;
		if (pkt.playerattacktype() == Protocol::PlayerAttackType::PLAYER_ATTACK_LEFT)
		{
			cout << "Left fire" << endl;
		}
		else
		{
			cout << "Right fire" << endl;
		}
	}
	else
	{
		cout << "Fail " << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_ATTACKED(PacketSessionRef& session, Protocol::S_ATTACKED& pkt)
{
	if (pkt.success())
	{
		cout << pkt.attackplayerid() << "hit " << pkt.targetplayerid() << endl;
		if (pkt.targetplayerid() == mynum)
			GcurrentHp -= 10;
	}
	else
	{
		cout << "Fail " << endl;
		errorController.CheckError(pkt.code());
	}
	return false;
}

bool Handle_S_LOSE(PacketSessionRef& session, Protocol::S_LOSE& pkt)
{
	if (pkt.success())
	{
		cout << pkt.playerid() << "is lose" << endl;
	}
	else
	{
		cout << "Fail " << endl;
		errorController.CheckError(pkt.code());
	}
	isGameStarted = false;
	return false;
}

bool Handle_S_WIN(PacketSessionRef& session, Protocol::S_WIN& pkt)
{
	if (pkt.success())
	{
		cout << pkt.playerid() << "is win" << endl;
	}
	else
	{
		cout << "Fail " << endl;
		errorController.CheckError(pkt.code());
	}
	isGameStarted = false;
	return false;
}
