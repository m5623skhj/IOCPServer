#include "PreCompile.h"
#include "IOCPServer.h"
#include "ServerCommon.h"
#include "IOCPSession.h"

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
	if (ServerOptionParsing(optionFileName) == false)
	{
		PrintError("ServerOptionParsing");
		return false;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		PrintError("WSAStartup");
		return false;
	}

	listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);
	if (listenSocket == INVALID_SOCKET)
	{
		PrintError("socket");
		return false;
	}

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (::bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		PrintError("bind");
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		PrintError("listen");
		return false;
	}

	if (SetSocketOption() == false)
	{
		return false;
	}

	RunThreads();

	return true;
}

void IOCPServer::StopServer()
{
	closesocket(listenSocket);

	delete[] workerOnList;
}

#pragma region thread
void IOCPServer::Accepter()
{
	SOCKET enteredClientSocket;
	SOCKADDR_IN enteredClientAddr;
	int addrSize = sizeof(enteredClientAddr);
	DWORD error = 0;
	WCHAR enteredIP[IP_SIZE];
	UNREFERENCED_PARAMETER(enteredIP);

	while (true)
	{
		enteredClientSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&enteredClientAddr), &addrSize);
		if (enteredClientSocket == INVALID_SOCKET)
		{
			error = GetLastError();
			if (error == WSAEINTR)
			{
				break;
			}
			else
			{
				PrintError("Accepter() / accept", error);
				continue;
			}
		}

		if (MakeNewSession(enteredClientSocket) == false)
		{
			closesocket(enteredClientSocket);
			continue;
		}

		InterlockedIncrement(&sessionCount);
	}
}

void IOCPServer::Worker(BYTE inThreadId)
{
	char postResult;
	int retval;
	DWORD transferred;
	IOCPSession* session;
	LPOVERLAPPED overlapped;

	while (1)
	{
		postResult = -1;
		transferred = 0;
		session = nullptr;
		overlapped = nullptr;


	}
}

void IOCPServer::RunThreads()
{
	workerOnList = new bool[numOfWorkerThread];

	for (BYTE i = 0; i < numOfWorkerThread; ++i)
	{
		workerOnList[i] = false;
		workerThreads.emplace_back([this, i]() { this->Worker(i); });
	}

	do
	{
		bool completed = true;
		for (int i = 0; i < numOfWorkerThread; ++i)
		{
			if (workerOnList[i] == false)
			{
				completed = false;
				break;
			}
		}

		if (completed == true)
		{
			break;
		}

		Sleep(1000);
	} while (true);

	accepterThread = std::thread([this]() { this->Accepter(); });
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

#pragma region session
bool IOCPServer::MakeNewSession(SOCKET enteredClientSocket)
{
	auto newSession = GetNewSession(enteredClientSocket);
	if (newSession == nullptr)
	{
		return false;
	}
	InterlockedIncrement(&newSession->ioCount);

	do
	{
		{
			std::lock_guard<std::mutex> lock(sessionMapLock);
			if (sessionMap.emplace(newSession->sessionId, newSession).second == false)
			{
				IOCountDecrement(*newSession);

				PrintError("RIOTestServer::MakeNewSession.emplace", GetLastError());
				break;
			}
		}

		RecvPost(*newSession);
		IOCountDecrement(*newSession);

		return true;
	} while (false);

	ReleaseSession(*newSession);
	return false;
}

std::shared_ptr<IOCPSession> IOCPServer::GetNewSession(SOCKET enteredClientSocket)
{
	SessionId newSessionId = InterlockedIncrement(&nextSessionId);
	if (newSessionId == INVALID_SESSION_ID)
	{
		return nullptr;
	}

	return make_shared<IOCPSession>(enteredClientSocket, newSessionId);
}

bool IOCPServer::ReleaseSession(OUT IOCPSession& releaseSession)
{
	return true;
}

void IOCPServer::IOCountDecrement(OUT IOCPSession& session)
{
	if (InterlockedDecrement(&session.ioCount) == 0)
	{
		ReleaseSession(session);
	}
}
#pragma endregion session

#pragma region io
IO_POST_ERROR IOCPServer::RecvPost(OUT IOCPSession& session)
{
	return IO_POST_ERROR::SUCCESS;
}

IO_POST_ERROR IOCPServer::SendPost(OUT IOCPSession& session)
{
	return IO_POST_ERROR::SUCCESS;
}
#pragma endregion io

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
