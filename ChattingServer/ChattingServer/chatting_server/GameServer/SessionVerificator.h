#pragma once
#include "GameSession.h"
#include "Room.h"
#include "User.h"
template<typename T>
class SessionVerificator
{
public:
	static SessionVerificator& GetInstance() 
	{
		static SessionVerificator* instance = new SessionVerificator();
		return *instance;
	}

	
	bool IsLogined(shared_ptr<GameSession> session,T& pkt);
	
	bool IsMaster(shared_ptr<GameSession> session, T& pkt);
	
	bool IsLobby(shared_ptr<GameSession> session, T& pkt);

	bool isGameStarted(shared_ptr<GameSession> session, T& pkt);
private:
	SessionVerificator() {};
};


template<typename T>
inline bool SessionVerificator<T>::IsLogined(shared_ptr<GameSession> session, T& pkt)
{
	if (session->user == nullptr)
	{
		//로그인 실패
		pkt.set_code(2);
		return false;
	}
		
	else
	{
		//로그인 성공
		pkt.set_code(1);
		return true;
	}
		
	
}

template<typename T>
inline bool SessionVerificator<T>::IsMaster(shared_ptr<GameSession> session, T& pkt)
{
	if (session->user->userId == session->_room.lock()->masterUserId)
	{
		//현재 방장
		pkt.set_code(4);
		return true;
	}
		
	else
	{
		//현재 방장 아님
		pkt.set_code(3);
		return false;
	}
		
	
}

template<typename T>
inline bool SessionVerificator<T>::IsLobby(shared_ptr<GameSession> session, T& pkt)
{
	if (session->_room.lock() == GRoom)
	{
		//현재 로비에 있음
		pkt.set_code(5);
		return true;
	}
	else
	{
		//현재 로비가 아닌 다른 방에 속해있음
		pkt.set_code(6);
		return false;
	}
		
}

template<typename T>
inline bool SessionVerificator<T>::isGameStarted(shared_ptr<GameSession> session, T& pkt)
{
	if (session->_room.lock()->isStarted)
	{
		pkt.set_code(12);
		return true;
	}
	else
	{
		pkt.set_code(13);
		return false;
	}
	return false;
}
