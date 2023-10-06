#include "PreCompile.h"
#include "IOCPServer.h"
#include "ServerCommon.h"

using namespace std;

void PrintError(const string& errorFunctionName)
{
	cout << errorFunctionName << "() failed " << GetLastError() << endl;
}

void PrintError(const string& errorFunctionName, DWORD errorCode)
{
	cout << errorFunctionName << "() failed " << errorCode << endl;
}

IOCPServer::IOCPServer()
{

}	

bool IOCPServer::StartServer(const std::wstring& optionFileName)
{


	return true;
}

void IOCPServer::StopServer()
{

}

#pragma region thread
void IOCPServer::Accepter()
{

}

void IOCPServer::Worker(BYTE inThreadId)
{

}

void IOCPServer::RunThreads()
{

}

IO_POST_ERROR IOCPServer::RecvCompleted(IOCPSession& session, DWORD transferred)
{
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	return result;
}

IO_POST_ERROR IOCPServer::SendCompleted(IOCPSession& session)
{
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	return result;
}

WORD IOCPServer::GetPayloadLength(OUT NetBuffer& buffer, int restSize)
{
	WORD payloadLength = INVALID_PAYLOAD_LENGTH;

	return payloadLength;
}
#pragma endregion thread

#pragma region serverOption
bool IOCPServer::ServerOptionParsing(const std::wstring& optionFileName)
{
	WCHAR buffer[BUFFER_MAX];
	LoadParsingText(buffer, optionFileName.c_str(), BUFFER_MAX);

	if (g_Paser.GetValue_Byte(buffer, L"IOCP_SERVER", L"THREAD_COUNT", &numOfWorkerThread) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Byte(buffer, L"IOCP_SERVER", L"NAGLE_ON", (BYTE*)&nagleOn) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Short(buffer, L"IOCP_SERVER", L"PORT", (short*)&port) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Int(buffer, L"IOCP_SERVER", L"MAX_CLIENT_COUNT", (int*)&maxClientCount) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Byte(buffer, L"SERIALIZEBUF", L"PACKET_CODE", &NetBuffer::m_byHeaderCode) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Byte(buffer, L"SERIALIZEBUF", L"PACKET_KEY", &NetBuffer::m_byXORCode) == false)
	{
		return false;
	}

	return true;
}

bool IOCPServer::SetSocketOption()
{
	if (nagleOn == true)
	{
		if (setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nagleOn, sizeof(int)) == SOCKET_ERROR)
		{
			PrintError("setsockopt_nagle");
			return false;
		}
	}

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerOption, sizeof(lingerOption)) == SOCKET_ERROR)
	{
		PrintError("setsockopt_linger");
		return false;
	}

	return true;
}
#pragma endregion serverOption
