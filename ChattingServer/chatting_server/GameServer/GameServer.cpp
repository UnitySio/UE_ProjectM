#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include "Protocol.pb.h"
#include "Job.h"
#include "Room.h"
#include "User.h"
#include "DBConnectionPool.h"
#include "DBBind.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
	//

	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=ServerDb;Trusted_Connection=Yes;"));

	// Create Table

	// TODO: FK INDEX 등록하기
	
	
	{
		auto query = L"										\
			DROP TABLE IF EXISTS [dbo].[account];			\
			CREATE TABLE [dbo].[account]					\
			(												\
				[id] INT NOT NULL PRIMARY KEY IDENTITY,		\
				[user_id] NVARCHAR(50) NULL,				\
				[pw] NVARCHAR(50) NULL,						\
				[name] NVARCHAR(50) NULL,					\
				[phone_number] NVARCHAR(50) NULL,			\
				[create_date] DATETIME NULL,				\
				[user_type] INT NULL,						\
				[login_session] VARCHAR(50) NULL			\
			);												\
			DROP TABLE IF EXISTS [dbo].[room]				\
			CREATE TABLE [dbo].[room]						\
			(												\
				[room_id] INT NOT NULL PRIMARY KEY,			\
				[name] NVARCHAR(50) NULL,					\
				[pw] NVARCHAR(50) NULL,						\
				[master_user_id] INT NULL 					\
			);												\
			DROP TABLE IF EXISTS [dbo].[chat_log]			\
			CREATE TABLE [dbo].[chat_log]					\
				(											\
					[chat_log_id] INT NOT NULL PRIMARY KEY IDENTITY, \
					[room_id] INT NULL,						\
					[message] NVARCHAR(100) NULL,			\
					[user_pk] INT NULL,						\
					[create_date] DATETIME NULL				\
				);											\
			";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}
	
	

	/*
	
	
	// Add Data
	for (int32 i = 0; i < 3; i++)
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		
		DBBind<3, 0> dbBind(*dbConn, L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)");

		int32 gold = 100;
		dbBind.BindParam(0, gold);
		WCHAR name[100] = L"루키스";
		dbBind.BindParam(1, name);
		TIMESTAMP_STRUCT ts = {2021, 6, 5};
		dbBind.BindParam(2, ts);

		ASSERT_CRASH(dbBind.Execute());

		GDBConnectionPool->Push(dbConn);
	}

	// Read
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();

		DBBind<1, 4> dbBind(*dbConn, L"SELECT id, gold, name, createDate FROM [dbo].[Gold] WHERE gold = (?)");
		
		int32 gold = 100;
		dbBind.BindParam(0, gold);

		int32 outId = 0;		
		int32 outGold = 0;
		WCHAR outName[100];
		TIMESTAMP_STRUCT outDate = {};
		dbBind.BindCol(0, OUT outId);
		dbBind.BindCol(1, OUT outGold);
		dbBind.BindCol(2, OUT outName);
		dbBind.BindCol(3, OUT outDate);

		ASSERT_CRASH(dbBind.Execute());

		wcout.imbue(locale("kor"));

		while (dbConn->Fetch())
		{
			wcout << "Id: " << outId << " Gold : " << outGold << " Name: " << outName << endl;
			wcout << "Date : " << outDate.year << "/" << outDate.month << "/" << outDate.day << endl;
		}

		GDBConnectionPool->Push(dbConn);
	}
	*/

	//

	ClientPacketHandler::Init();

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager 등
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

	// Main Thread
	DoWorkerJob(service);

	GThreadManager->Join();
}