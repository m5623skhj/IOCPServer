#pragma once

enum class IO_POST_ERROR : char
{
	SUCCESS = 0
	, IS_DELETED_SESSION
	, FAILED_RECV_POST
	, FAILED_SEND_POST
	, INVALID_OPERATION_TYPE
};

enum class PACKET_ID : unsigned int
{
	INVALID_PACKET = 0
	, PING
	, PONG
};