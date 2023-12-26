#include "pch.h"
#include "MessageExporter.h"
#include "DBConnection.h"
#include "DBConnectionPool.h"
#include "DBBind.h"

static void stringToUTF16(const string& value, WCHAR* target)
{

	const char* str = value.c_str();
	int32 nLen = ::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), NULL, NULL);

	::MultiByteToWideChar(CP_ACP, 0, str, strlen(str), target, nLen);
	target[nLen] = L'\0';
}

shared_ptr<MessageExporter> GExporter = make_shared<MessageExporter>();
void MessageExporter::Push(MessageStruct messageStruct)
{
	_lockQueue.Push(messageStruct);
}

void MessageExporter::SaveAll()
{
	Vector<MessageStruct> messageArr;
	_lockQueue.PopAll(messageArr);

	if (messageArr.size() == 0)
		return;

	for(auto mStruct:messageArr)
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		auto query = L"INSERT INTO [dbo].[chat_log]([room_id], [message], [user_pk],[create_date]) VALUES(?,?,?,CURRENT_TIMESTAMP)";
		DBBind<3, 0> dbBind(*dbConn, query);
		WCHAR* message = new WCHAR[100];
		stringToUTF16(mStruct._message, message);
		dbBind.BindParam(0, mStruct._roomId);
		dbBind.BindParam(1, const_cast<WCHAR*>(message));
		dbBind.BindParam(2, mStruct._userPk);
		ASSERT_CRASH(dbBind.Execute());
		GDBConnectionPool->Push(dbConn);
		delete[] message;
	}

}
