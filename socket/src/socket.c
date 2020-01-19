#include "socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#ifdef __linux
#include <arpa/inet.h>
#include <unistd.h>

#undef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#undef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#undef TIMEVAL
#define TIMEVAL struct timeval
#endif

enum
{
	__SKT_TYPE_NONE = 0,
	__SKT_TYPE_CLIENT,
	__SKT_TYPE_SERVER,
	__SKT_TYPE_SERVER_CLIENT,
};

int socket_errno()
{
	#if defined(__linux)
	return errno;
	#else
	return WSAGetLastError();
	#endif
}
const char* socket_errmsg(int err)
{
	#if defined(__linux)
	return strerror(errno);
	#else
	static char buffer[256];

	memset(buffer, 0, 256);
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err, 0, buffer, 256, NULL) == 0)
	{
		// Failed in translating the error.
		return buffer;
	}

	//wcstombs(buffer, wideStr, 500); // no test

	return buffer;
	#endif
}
static void error_dump(const char* func, int line)
{
	int err = socket_errno();
	fprintf(stderr, "%s: %d, %d, %s\n", func, line, err, socket_errmsg(err));
}

#ifndef DEBUG 
#define error_dump(...)
#endif

// winsock init
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
static int winsock_inited;
static int winsock_count;
static int winsock_init()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		error_dump(__func__, __LINE__);
		return 0;
	}

	return 1;
}
static int close(SOCKET s)
{
	int ret = closesocket(s);
	if (winsock_count > 0)
	{
		if (--winsock_count == 0)
		{
			WSACleanup();
			winsock_inited = 0;
		}
	}
	return ret;
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
			winsock_inited = 0;
			return NULL;
		}

		winsock_inited = 1;
	}
	#endif
	socket_t sk;

	// malloc socket memory
	if ((sk = (socket_t)malloc(sizeof(struct socket))) == NULL)
	{
		// TODO
		return NULL;
	}

	sk->type = __SKT_TYPE_NONE;
	sk->recv_timeout = -1;
	sk->accept_timeout = -1;
	sk->socket = 0;

	// get window socket
	if ((sk->socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		socket_delete(sk);
		error_dump(__func__, __LINE__);
		return NULL;
	}

	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)
	++winsock_count;
	#endif

	return sk;
}

void socket_delete(socket_t sk)
{
	if (sk)// && sk->type != __SKT_TYPE_SERVER_CLIENT) // TODO should we close this type of socket?
	{
		close(sk->socket);
	}

	free(sk);
	sk = NULL;
}

int socket_bind(socket_t sock, const char* ip, uint16_t port)
{
	assert(sock != NULL);
	assert(ip != NULL);

	// TODO only a new socket can use to bind to an address
	if (sock->type != __SKT_TYPE_NONE)
	{
		return -1;
	}

	// in linux
	int option = 1;
	if (setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, (const void*)&option, sizeof(option)) < 0)
	{
		error_dump(__func__, __LINE__);
		return -1;
	}

	sock->addr.sin_addr.s_addr = inet_addr(ip);
	sock->addr.sin_family = AF_INET;
	sock->addr.sin_port = htons(port);

	if (bind(sock->socket, (struct sockaddr*) & sock->addr, sizeof(sock->addr)) == SOCKET_ERROR)
	{
		error_dump(__func__, __LINE__);
		return -1;
	}

	sock->type = __SKT_TYPE_SERVER;

	return 1;
}

int socket_connect(socket_t sock, const char* ip, uint16_t port)
{
	assert(sock != NULL);
	assert(ip != NULL);

	// TODO only a new socket can be use to connect to an address
	if (sock->type != __SKT_TYPE_NONE)
	{
		return -1;
	}

	sock->addr.sin_addr.s_addr = inet_addr(ip);
	sock->addr.sin_family = AF_INET;
	sock->addr.sin_port = htons(port);

	if (connect(sock->socket, (struct sockaddr*) & sock->addr,
		sizeof(sock->addr)) < 0)
	{
		error_dump(__func__, __LINE__);
		return -1;
	}

	sock->type = __SKT_TYPE_CLIENT;

	return 1;
}

int socket_send(socket_t sock, const uint8_t* data, size_t size)
{
	assert(sock != NULL);
	assert(data != NULL);

	// TODO a new socket or a server socket cannot use this api.
	if (sock->type == __SKT_TYPE_NONE || sock->type == __SKT_TYPE_SERVER)
	{
		return -1;
	}

	if (send(sock->socket, data, size, 0) < 0)
	{
		error_dump(__func__, __LINE__);
		return -1;
	}

	return 1;
}

int socket_recv(socket_t sock, uint8_t* buff, size_t size)
{
	int rc = 0;

	assert(sock != NULL);
	assert(buff != NULL);

	// TODO a new socket or a server socket cannot use this api.
	if (sock->type == __SKT_TYPE_NONE || sock->type == __SKT_TYPE_SERVER)
	{
		return -1;
	}

	// block recv
	if (sock->recv_timeout < 0)
	{
		__read:
		{
			if ((rc = recv(sock->socket, buff, size, 0)) == SOCKET_ERROR)
			{
				error_dump(__func__, __LINE__);
				return -1;
			}
		}
	}
	else
	{
		fd_set fds;
		TIMEVAL tv;
		FD_ZERO(&fds);

		tv.tv_sec = sock->recv_timeout / 1000;
		tv.tv_usec = sock->recv_timeout % 1000 * 1000;
		FD_SET(sock->socket, &fds);

		// TODO select < 0(-1): error
		rc = select(sock->socket + 1, &fds, NULL, NULL, &tv);
		if (rc > 0 && FD_ISSET(sock->socket, &fds))
		{
			goto __read;
		}
		if (rc < 0)
		{
			error_dump(__func__, __LINE__);
			return -1;
		}
	}

	return rc;
}

int socket_listen(socket_t sock, int backlog)
{
	assert(sock != NULL);
	return listen(sock->socket, backlog);
}

socket_t socket_accept(socket_t sock)
{
	socket_t csk = NULL;
	struct socket cln;
	int c;

	assert(sock != NULL);

	if (sock->type != __SKT_TYPE_SERVER)
	{
		// TODO a socket which is no server should not use this api.
		return NULL;
	}

	if (sock->accept_timeout < 0)
	{
		__accept:
		{
			c = sizeof(struct sockaddr_in);
			cln.socket = accept(sock->socket, (struct sockaddr*) & cln.addr, &c);
			if (cln.socket == INVALID_SOCKET)
			{
				error_dump(__func__, __LINE__);
				return NULL;
			}

			// malloc socket memory
			if ((csk = (socket_t)malloc(sizeof(struct socket))) == NULL)
			{
				// TODO should we close socket?
				return NULL;
			}
			memcpy((void*)csk, (void*)&cln, sizeof(cln));
			csk->type = __SKT_TYPE_SERVER_CLIENT;
			csk->accept_timeout = -1;
			csk->recv_timeout = -1;
		}
	}
	else
	{
		fd_set fds;
		TIMEVAL tv;
		FD_ZERO(&fds);

		tv.tv_sec = sock->accept_timeout / 1000;
		tv.tv_usec = sock->accept_timeout % 1000 * 1000;
		FD_SET(sock->socket, &fds);

		// TODO select < 0(-1): error
		c = select(sock->socket + 1, &fds, NULL, NULL, &tv);
		if (c > 0 && FD_ISSET(sock->socket, &fds))
		{
			goto __accept;
		}
		if (c < 0)
		{
			error_dump(__func__, __LINE__);
			return NULL;
		}
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
