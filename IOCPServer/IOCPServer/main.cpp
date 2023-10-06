#include "PreCompile.h"
#include "IOCPServer.h"

int main()
{
	IOCPServer::GetInst().StartServer(L"OptionFile/DBClientOptionFile.txt");

	while (true)
	{
		Sleep(1000);
	}

	return 0;
}