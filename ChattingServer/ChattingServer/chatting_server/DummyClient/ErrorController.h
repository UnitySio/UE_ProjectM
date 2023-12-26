#pragma once
class ErrorController
{
public:
	static ErrorController& GetInstance()
	{
		static ErrorController* instance = new ErrorController();
		return *instance;
	}
	bool CheckError(uint64 errorCode);

private:
	ErrorController() {};
};

