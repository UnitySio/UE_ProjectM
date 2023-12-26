#include "pch.h"
#include "ErrorController.h"
bool ErrorController::CheckError(uint64 errorCode)
{	
	switch (errorCode)
	{
	case 0:
	{
		cout << "not error occured" << endl;
		break;
	}
	case 1:
	{
		cout << "Login Fail. Because Already Logined" << endl;
		break;
	}
	case 2:
	{
		cout << "First, Login plz." << endl;
		break;
	}
	case 3:
	{
		cout << "You are not master." << endl;
		break;
	}
	case 4:
	{
		cout << "You are master" << endl;
		break;
	}
	case 5:
	{
		cout << "You can't start because your session in lobby" << endl;
		break;
	}
	case 6:
	{
		cout << "Your session is not lobby" << endl;
		break;
	}
	case 7:
	{
		cout << "pw incorrect" << endl;
		break;
	}
	case 8:
	{
		cout << "Room Not found" << endl;
		break;
	}
	case 9:
	{
		cout << "This room Already Started" << endl;
		break;
	}
	case 10:
	{
		cout << "This Room Already big" << endl;
		break;
	}
	case 11:
	{
		cout << "All people don't ready" << endl;
		break;
	}
	case 12:
	{
		cout << "Already started" << endl;
		break;
	}
	case 13:
	{
		cout << "not started" << endl;
		break;
	}
	case 14:
	{
		cout << "target player is empty" << endl;
	}
	}
	return false;
			
}

