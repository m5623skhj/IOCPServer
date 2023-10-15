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

	iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfWorkerThread);
	if (iocpHandle == NULL)
	{
		PrintError("IOCP handle was null");
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
	SessionId completionKey;
	LPOVERLAPPED overlapped;

	while (1)
	{
		postResult = -1;
		transferred = 0;
		completionKey = INVALID_SESSION_ID;
		overlapped = nullptr;

		if (GetQueuedCompletionStatus(iocpHandle, &transferred, static_cast<PULONG_PTR>(&completionKey), &overlapped, INFINITE) == false)
		{
			GetQueuedCompletionStatusFailed(overlapped, completionKey, transferred);
			continue;
		}

		GetQueuedCompletionStatusSuccess(overlapped, completionKey, transferred);
	}

	NetBuffer::ChunkFreeForcibly();
}

void IOCPServer::GetQueuedCompletionStatusSuccess(LPOVERLAPPED overlapped, SessionId sessionId, DWORD transferred)
{
	if (overlapped == NULL)
	{
		PrintError("GetQueuedCompletionStatusSuccess() overlapped is NULL");
		g_Dump.Crash();
	}

	auto session = GetSession(sessionId);
	if (session == nullptr)
	{
		return;
	}

	if (transferred == 0 || session->ioCancel)
	{
		IOCountDecrement(*session);
		return;
	}

	IOCompletedProcess(overlapped, *session, transferred);
}

void IOCPServer::GetQueuedCompletionStatusFailed(LPOVERLAPPED overlapped, SessionId sessionId, DWORD transferred)
{
	if (overlapped == NULL)
	{
		PrintError("GetQueuedCompletionStatusFailed() overlapped is NULL");
		g_Dump.Crash();
	}

	auto session = GetSession(sessionId);
	if (session == nullptr)
	{
		return;
	}

	if (transferred == 0 || session->ioCancel)
	{
		IOCountDecrement(*session);
	}

	int error = GetLastError();
	if (error != ERROR_NETNAME_DELETED && error != ERROR_OPERATION_ABORTED)
	{
		PrintError("GetQueuedCompletionStatusSuccess() error :", error);
	}
}

std::shared_ptr<IOCPSession> IOCPServer::GetSession(SessionId sessionId)
{
	lock_guard<mutex> lock(sessionMapLock);
	auto iter = sessionMap.find(sessionId);
	if (iter == sessionMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

bool IOCPServer::IsClosedSession(DWORD transferred, IOCPSession& session)
{
	if (transferred == 0 || session.ioCancel == true)
	{
		IOCountDecrement(session);
		return false;
	}

	return true;
}

void IOCPServer::IOCompletedProcess(LPOVERLAPPED overlapped, IOCPSession& session, DWORD transferred)
{
	IO_POST_ERROR retval = IO_POST_ERROR::INVALID_OPERATION_TYPE;
	if (overlapped == &session.recvIOData.overlapped)
	{
		retval = IORecvPart(session, transferred);
	}
	else if (overlapped == &session.sendIOData.overlapped)
	{
		retval = IOSendPart(session);
	}
	else if (overlapped == &session.postQueueOverlapped)
	{
		retval = IOSendPostPart(session);
	}

	if (retval == IO_POST_ERROR::IS_DELETED_SESSION)
	{
		return;
	}

	IOCountDecrement(session);
}

IO_POST_ERROR IOCPServer::IORecvPart(IOCPSession& session, DWORD transferred)
{
	session.recvIOData.ringBuffer.MoveWritePos(transferred);
	int restSize = session.recvIOData.ringBuffer.GetUseSize();
	bool isPacketError = false;

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		session.recvIOData.ringBuffer.Peek((char*)buffer.m_pSerializeBuffer, df_HEADER_SIZE);
		buffer.m_iRead = 0;

		WORD payloadLength;
		buffer >> payloadLength;
		if (restSize < payloadLength + df_HEADER_SIZE)
		{
			if (payloadLength > dfDEFAULTSIZE)
			{
				HandlePacketError(buffer, "Invalid payload length " + payloadLength);
				isPacketError = true;
				break;
			}
		}

		session.recvIOData.ringBuffer.RemoveData(df_HEADER_SIZE);
		int dequeuedSize = session.recvIOData.ringBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
		buffer.m_iWrite += dequeuedSize;
		if (PacketDecode(buffer) == false)
		{
			HandlePacketError(buffer, "Decode was failed");
			isPacketError = true;
			break;
		}

		restSize -= dequeuedSize + df_HEADER_SIZE;
		session.OnReceived(buffer);
		NetBuffer::Free(&buffer);
	}

	IO_POST_ERROR retval = RecvPost(session);
	if (isPacketError == true)
	{
		Disconnect(session.sessionId);
	}

	return retval;
}

IO_POST_ERROR IOCPServer::IOSendPart(IOCPSession& session)
{
	IO_POST_ERROR retval;

	int bufferCount = session.sendIOData.bufferCount;
	for (int i = 0; i < bufferCount; ++i)
	{
		NetBuffer::Free(session.storedBuffer[i]);
	}

	session.sendIOData.bufferCount = 0;

	if (session.isSendAndDisconnect == false)
	{
		InterlockedExchange(&session.sendIOData.ioMode, NONSENDING);
		retval = SendPost(session);
		InterlockedExchange(&session.nowPostQueueing, NONSENDING);
	}
	else
	{
		shutdown(session.socket, SD_BOTH);
		retval = IO_POST_ERROR::SUCCESS;
	}

	return retval;
}

IO_POST_ERROR IOCPServer::IOSendPostPart(IOCPSession& session)
{
	IO_POST_ERROR retval = SendPost(session);
	InterlockedExchange(&session.nowPostQueueing, NONSENDING);

	return retval;
}

bool IOCPServer::PacketDecode(OUT NetBuffer& buffer)
{
	if (buffer.Decode() == false)
	{
		return false;
	}

	return true;
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
			lock_guard<std::mutex> lock(sessionMapLock);
			if (sessionMap.emplace(newSession->sessionId, newSession).second == false)
			{
				IOCountDecrement(*newSession);

				PrintError("RIOTestServer::MakeNewSession.emplace", GetLastError());
				break;
			}
		}
		newSession->Initialize();

		RecvPost(*newSession);
		IOCountDecrement(*newSession);

		newSession->OnClientEntered();

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
	closesocket(releaseSession.socket);
	InterlockedDecrement(&sessionCount);

	lock_guard<mutex> lock(sessionMapLock);
	sessionMap.erase(releaseSession.sessionId);

	return true;
}

void IOCPServer::IOCountDecrement(OUT IOCPSession& session)
{
	if (InterlockedDecrement(&session.ioCount) == 0)
	{
		ReleaseSession(session);
	}
}

void IOCPServer::SendPacket(IOCPSession& session, OUT NetBuffer& packet)
{
	if (packet.m_bIsEncoded == false)
	{
		packet.m_iWriteLast = packet.m_iWrite;
		packet.m_iWrite = 0;
		packet.m_iRead = 0;
		packet.Encode();
	}

	session.sendIOData.sendQ.Enqueue(&packet);
	SendPost(session);
}

void IOCPServer::HandlePacketError(NetBuffer& willDeallocateBuffer, const std::string& printErrorString)
{
	PrintError(printErrorString);
	NetBuffer::Free(&willDeallocateBuffer);
}

void IOCPServer::Disconnect(SessionId sessionId)
{
	lock_guard<mutex> lock(sessionMapLock);
	auto session = sessionMap.find(sessionId);
	if (session == sessionMap.end())
	{
		return;
	}

	shutdown(session->second->socket, SD_BOTH);
}
#pragma endregion session

#pragma region io
IO_POST_ERROR IOCPServer::RecvPost(OUT IOCPSession& session)
{
	int brokenSize = session.recvIOData.ringBuffer.GetNotBrokenPutSize();
	int restSize = session.recvIOData.ringBuffer.GetFreeSize() - brokenSize;
	int bufferCount = 1;
	DWORD flag = 0;

	WSABUF buffer[2];
	buffer[0].buf = session.recvIOData.ringBuffer.GetWriteBufferPtr();
	buffer[0].len = brokenSize;
	if (restSize > 0)
	{
		buffer[1].buf = session.recvIOData.ringBuffer.GetBufferPtr();
		buffer[1].len = restSize;
		++bufferCount;
	}

	InterlockedIncrement(&session.ioCount);
	if (WSARecv(session.socket, buffer, bufferCount, NULL, &flag, &session.recvIOData.overlapped, 0)
		== SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			IOCountDecrement(session);
			PrintError("WSARecv error", error);
			
			return IO_POST_ERROR::FAILED_RECV_POST;
		}
	}

	return IO_POST_ERROR::SUCCESS;
}

IO_POST_ERROR IOCPServer::SendPost(OUT IOCPSession& session)
{
	while (1)
	{
		if (InterlockedCompareExchange(&session.sendIOData.ioMode, SENDING, NONSENDING))
		{
			return IO_POST_ERROR::SUCCESS;
		}

		int useSize = session.sendIOData.sendQ.GetRestSize();
		if (useSize == 0)
		{
			InterlockedExchange(&session.sendIOData.ioMode, NONSENDING);
			if (session.sendIOData.sendQ.GetRestSize() > 0)
			{
				continue;
			}

			return IO_POST_ERROR::SUCCESS;
		}

		WSABUF buffer[ONE_SEND_WSABUF_MAX];
		if (ONE_SEND_WSABUF_MAX < useSize)
		{
			useSize = ONE_SEND_WSABUF_MAX;
		}
		session.sendIOData.bufferCount += useSize;

		InterlockedIncrement(&session.ioCount);
		if (WSASend(session.socket, buffer, useSize, NULL, 0, &session.sendIOData.overlapped, 0)
			== SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != ERROR_IO_PENDING)
			{
				PrintError("SendPost", error);
				IOCountDecrement(session);
				return IO_POST_ERROR::IS_DELETED_SESSION;
			}

			return IO_POST_ERROR::FAILED_SEND_POST;
		}
	}
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
