#pragma once
#include "JobQueue.h"

class MessageStruct
{
public:
	int64 _roomId;
	string _message;
	int32 _userPk;
};

class MessageExporter : public JobQueue
{
public:
	void Push(MessageStruct messageStruct);
	void SaveAll();

private:
	LockQueue<MessageStruct> _lockQueue;

};

extern shared_ptr<MessageExporter> GExporter;