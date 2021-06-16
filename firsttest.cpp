#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_FDS 500
#define DATA_BUFFER 5000

using namespace std;
int create_tcp_server_socket() {
	struct sockaddr_in saddr;
	int fd, ret_val;

	/* Step1: create a TCP socket */
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		fprintf(stderr, "socket failed [%s]\n", strerror(errno));
		return -1;
	}
	printf("Created a socket with fd: %d\n", fd);

	/* Initialize the socket address structure */
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(7003);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* Step2: bind the socket to port 7000 on the local host */
	ret_val = bind(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val != 0) {
		fprintf(stderr, "bind failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}

	/* Step3: listen for incoming connections */
	ret_val = listen(fd, 5);
	if (ret_val != 0) {
		fprintf(stderr, "listen failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}
	fcntl(fd, F_SETFL, O_NONBLOCK);
	return fd;
}

int main () {
	struct pollfd pollfds[MAX_FDS];// = new struct pollfd;
	int server_fd, ret_val, nb_fds=0;
	socklen_t addrlen;
	struct sockaddr_storage client_saddr;
	char buf[DATA_BUFFER];
	string data;

	/* Get the socket server fd */
	server_fd = create_tcp_server_socket();
	if (server_fd == -1) {
		fprintf(stderr, "Failed to create a server\n");
		return 1;
	}

	pollfds[0].fd = server_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	nb_fds = 1;
	while (1) {
		// monitor readfds for readiness for reading

	printf("-----------------------------------------------------------");
	printf("\nUsing poll() to listen for incoming events\n");
	if (poll (pollfds, nb_fds, -1) == -1)
		printf("Error poll");

		// Some sockets are ready. Examine readfds
		for (int fd = 0; fd < (nb_fds + 1); fd++) {
			if ((pollfds + fd) -> fd <= 0) // file desc == 0 is not expected, as these are socket fds and not stdin
				continue;

			if (((pollfds + fd) -> revents & POLLIN) == POLLIN) {  // fd is ready for reading
				if ((pollfds + fd) -> fd == server_fd) {  // request for new connection
					addrlen = sizeof (struct sockaddr_storage);
					int fd_new;
					if ((fd_new = accept (server_fd, (struct sockaddr *) &client_saddr, &addrlen)) == -1)
						printf ("Error accept");
					// add fd_new to pollfds
					cout << "New connection acceped, fd: " << fd_new << endl;
					(pollfds + nb_fds) -> fd = fd_new;
					(pollfds + nb_fds) -> events = POLLIN;
					(pollfds + nb_fds) -> revents = 0;
					if (nb_fds < MAX_FDS)
						nb_fds++;
				}
				else  // data from an existing connection, receive it
				{
					ret_val = recv ((pollfds + fd) -> fd, buf, DATA_BUFFER, 0);
					if (ret_val == -1)
						printf("Error recv");
					else if (ret_val == 0) {
						// connection closed by client
						fprintf (stderr, "Socket %d closed by client\n", (pollfds + fd) -> fd);
						if (close ((pollfds + fd) -> fd) == -1)
							printf("Error close");
						(pollfds + fd) -> fd *= -1; // make it negative so that it is ignored in future
					}
					else
					{
						cout << "buf= " << buf << endl;
						data.assign(buf);
						if (strlen(buf) > 1)
							for (int fd = 0; fd < (nb_fds + 1); fd++){
								send((pollfds + fd)->fd, buf, strlen(buf), 0);
							}
						memset(buf, 0, sizeof(buf));
					}
				}
			}
		}
	} // while (1)

return 0;
}
