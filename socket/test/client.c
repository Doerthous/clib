#include <stdio.h>
#include <string.h>
#include "socket.h"

int main(int argc, char const* argv[])
{
	socket_t sk;
	int rc, wc;
	char buff[1024];

	sk = socket_new();
	if (!sk)
	{
		perror("socket new failed.\n");
		return -1;
	}

	if (socket_connect(sk, "127.0.0.1", 8081) < 0)
	{
		perror("socket connect failed.\n");
		socket_delete(sk);
		return -1;
	}



	sk->recv_timeout = 0;

	while (1) {
		if (socket_send(sk, (uint8_t*)"Hello world!", strlen("Hello world!")) < 0)
		{
			perror("socket send failed.\n");
			break;
		}
		rc = socket_recv(sk, buff, 1024);
		if (rc > 0)
		{
			buff[rc] = 0;
			printf("recv: %s\n", buff);
		}
		if (rc < 0) {
			perror("socket recv failed.\n");
			break;
		}
		getchar();
	}

	socket_delete(sk);
}
