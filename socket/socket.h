
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") //Winsock Library

#elif defined(__linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKET int

#endif

#ifdef __cplusplus
extern "C"
{
	#endif

	typedef struct socket
	{
		struct sockaddr_in addr;
		SOCKET socket;
		int type;
	} *socket_t;

	socket_t socket_new();
	void socket_delete(socket_t sock);
	int socket_connect(socket_t client, const char* ip, uint16_t port);
	int socket_send(socket_t sock, const uint8_t* data, size_t size);
	size_t socket_recv(socket_t sock, uint8_t* buff, size_t size);
	int socket_bind(socket_t server, const char* ip, uint16_t port);
	int socket_listen(socket_t server, int backlog);
	socket_t socket_accept(socket_t server);
	//
	const char* socket_get_ip(socket_t sock);
	const uint16_t socket_get_port(socket_t sock);
	//
	int socket_errno();
	const char* socket_errmsg(int err);

	#ifdef __cplusplus
}
#endif
