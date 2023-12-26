#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ClientPacketHandler.h"
#include "Room.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "User.h"

static void deleteSession(UserRef user)
{
	DBConnection * dbConn = GDBConnectionPool->Pop();
	auto query = L"UPDATE [dbo].[account] SET login_session = NULL WHERE id = (?)";
	DBBind<1, 0> dbBind(*dbConn, query);
	dbBind.BindParam(0, user->userPk);
	ASSERT_CRASH(dbBind.Execute());
	GDBConnectionPool->Push(dbConn);
}

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	if (_currentUser)
	{
		if (auto room = _room.lock())
		{
			room->DoAsync(&Room::Leave, _currentUser);
			deleteSession(_currentUser);
		}
			
	}

	_currentUser = nullptr;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}