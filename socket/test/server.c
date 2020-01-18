#include <stdio.h>
#include <string.h>
#include "socket.h"

int main(int argc, char const* argv[])
{
	socket_t server, client;
	int rc, wc;
	char buff[1024];

	server = socket_new();
	if (!server)
	{
		perror("socket new failed.\n");
		return -1;
	}

	if (socket_bind(server, "127.0.0.1", 8081) < 0)
	{
		perror("socket bind failed.\n");
	}

	printf("server[%s:%d] startup\n",
		   socket_get_ip(server), socket_get_port(server));

	if (socket_listen(server, 3) < 0)
	{
		perror("socket listen failed.\n");
	}

	client = socket_accept(server);
	if (client != NULL) {
		printf("client[%s:%d] connected\n",
		   socket_get_ip(client), socket_get_port(client));
		client->recv_timeout = 3000;
		while (1) {
			if ((rc = socket_recv(client, buff, 1024)) > 0)
			{
				buff[rc] = 0;
				printf("recv: %s\n", buff);
				socket_send(client, "OK!", strlen("OK!"));
			}
			else {
				break;
			}
		}
	}


	socket_delete(client);
	socket_delete(server);
}
