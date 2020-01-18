#include "socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#ifdef __linux
#include <arpa/inet.h>

#undef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#undef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

// error handle
static int __socket_errno;
static const char* __socket_err_msg[] = {
	"", // OK
	"socket create failed.",
	"this socket has used by a client or server, cannot bind to an address.",
	"this socket has used by a client or server, cannot connect to an address.",
	"this socket has used by a client or server, cannot accpet an address.",
	"The socket is not connect or has binded to a address",

/// window socket errors
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
	"winsock startup failed",
	"winsock socket create failed",
#endif
};
enum
{
	__SKT_OK = 0,
	__SKT_SOCKET_CREATE_FAILED,
	__SKT_BIND_USING_AN_USED_SOCKET,
	__SKT_CONNECT_USING_AN_USED_SOCKET,
	__SKT_ACCEPT_USING_AN_USED_SOCKET,
	__SKT_NONE_OR_SERVER_SOCKET,

///window socket errors
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
__SKT_WIN_WSA_STARTUP_FAILED,
__SKT_WIN_SOCKET_CREATE_FAILED,
#endif
__SKT_ERR_CNT,
};
enum
{
	__SKT_TYPE_NONE = 0,
	__SKT_TYPE_CLIENT,
	__SKT_TYPE_SERVER,
	__SKT_TYPE_SERVER_CLIENT,
};

#define SET_ERROR(__err) __socket_errno = __SKT_##__err;

int socket_errno()
{
	return __socket_errno;
}
const char* socket_errmsg(int err)
{
	assert(0 <= err && err <= __SKT_ERR_CNT);
	return __socket_err_msg[err];
}

// winsock init
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
static int winsock_inited;
static int winsock_count;
static int winsock_init()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		fprintf(stderr, "winsock startup failed: %d", WSAGetLastError());
		SET_ERROR(WIN_WSA_STARTUP_FAILED);
		return 0;
	}

	return 1;
}
#endif

// socket interfaces
socket_t socket_new()
{
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
		// window socket startup
	if (!winsock_inited)
	{
		if (!winsock_init())
		{
			return NULL;
		}

		winsock_inited = 1;
	}
	#endif
	socket_t sk;

	// malloc socket memory
	if ((sk = (socket_t)malloc(sizeof(struct socket))) == NULL)
	{
		//errno = 1;
		return NULL;
	}

	sk->type = __SKT_TYPE_NONE;

	// get window socket
	if ((sk->socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		socket_delete(sk);
		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
		fprintf(stderr, "could not create socket: %d", WSAGetLastError());
		#elif defined(__linux)
		fprintf(stderr, "could not create socket: %s(%d)", strerror(errno), errno);
		#endif
		SET_ERROR(SOCKET_CREATE_FAILED);
		return NULL;
	}

	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
	++winsock_count;
	#endif

	return sk;
}

void socket_delete(socket_t sk)
{
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
	if (sk && sk->type != __SKT_TYPE_SERVER_CLIENT)
	{
		closesocket(sk->socket);
		if (winsock_count > 0)
		{
			if (--winsock_count == 0)
			{
				WSACleanup();
				winsock_inited = 0;
			}
		}
	}
	#endif
	free(sk);
	sk = NULL;
}

int socket_bind(socket_t sock, const char* ip, uint16_t port)
{
	assert(sock != NULL);
	assert(ip != NULL);

	if (sock->type != __SKT_TYPE_NONE)
	{
		SET_ERROR(BIND_USING_AN_USED_SOCKET);
		return 0;
	}

	sock->addr.sin_addr.s_addr = inet_addr(ip);
	sock->addr.sin_family = AF_INET;
	sock->addr.sin_port = htons(port);

	if (bind(sock->socket, (struct sockaddr*) & sock->addr,
		sizeof(sock->addr)) == SOCKET_ERROR)
	{
		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
		fprintf(stderr, "bind failed: %d", WSAGetLastError());
		#elif defined(__linux)
		fprintf(stderr, "bind failed: %s(%d)", strerror(errno), errno);
		#endif
		return 0;
	}

	sock->type = __SKT_TYPE_SERVER;

	return 1;
}

int socket_connect(socket_t sock, const char* ip, uint16_t port)
{
	assert(sock != NULL);
	assert(ip != NULL);

	if (sock->type != __SKT_TYPE_NONE)
	{
		SET_ERROR(CONNECT_USING_AN_USED_SOCKET);
		return 0;
	}

	sock->addr.sin_addr.s_addr = inet_addr(ip);
	sock->addr.sin_family = AF_INET;
	sock->addr.sin_port = htons(port);

	if (connect(sock->socket, (struct sockaddr*) & sock->addr,
		sizeof(sock->addr)) < 0)
	{
		perror("connect error");
		return 0;
	}

	sock->type = __SKT_TYPE_CLIENT;

	return 1;
}

int socket_send(socket_t sock, const uint8_t* data, size_t size)
{
	assert(sock != NULL);
	assert(data != NULL);

	if (sock->type == __SKT_TYPE_NONE || sock->type == __SKT_TYPE_SERVER)
	{
		SET_ERROR(NONE_OR_SERVER_SOCKET);
		return 0;
	}

	if (send(sock->socket, data, size, 0) < 0)
	{
		perror("send error");
		return 0;
	}

	return 1;
}

size_t socket_recv(socket_t sock, uint8_t* buff, size_t size)
{
	int recv_size;

	assert(sock != NULL);
	assert(buff != NULL);

	if (sock->type == __SKT_TYPE_NONE || sock->type == __SKT_TYPE_SERVER)
	{
		SET_ERROR(NONE_OR_SERVER_SOCKET);
		return 0;
	}

	if ((recv_size = recv(sock->socket, buff, size, 0)) == SOCKET_ERROR)
	{
		perror("recv failed");
		return 0;
	}

	return (size_t)recv_size;
}

int socket_listen(socket_t sock, int backlog)
{
	assert(sock != NULL);
	return listen(sock->socket, backlog) == 0;
}

socket_t socket_accept(socket_t sock)
{
	struct sockaddr_in client;
	socket_t csk;
	int c;

	assert(sock != NULL);

	if (sock->type != __SKT_TYPE_SERVER)
	{
		SET_ERROR(ACCEPT_USING_AN_USED_SOCKET);
		return NULL;
	}

	// malloc socket memory
	if ((csk = (socket_t)malloc(sizeof(struct socket))) == NULL)
	{
		//errno = 1;
		return NULL;
	}
	csk->type = __SKT_TYPE_SERVER_CLIENT;

	c = sizeof(struct sockaddr_in);
	csk->socket = accept(sock->socket, (struct sockaddr*) & client, &c);
	if (csk->socket == INVALID_SOCKET)
	{
		socket_delete(csk);
		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
		fprintf(stderr, "accept failed: %d", WSAGetLastError());
		#elif defined(__linux)
		fprintf(stderr, "accept failed: %s(%d)", strerror(errno), errno);
		#endif
		return NULL;
	}

	return csk;
}

const char* socket_get_ip(socket_t sock)
{
	assert(sock != NULL);

	if (sock->type != __SKT_TYPE_NONE)
	{
		return inet_ntoa(sock->addr.sin_addr);
	}

	return "";
}

const uint16_t socket_get_port(socket_t sock)
{
	assert(sock != NULL);

	return ntohs(sock->addr.sin_port);
}
