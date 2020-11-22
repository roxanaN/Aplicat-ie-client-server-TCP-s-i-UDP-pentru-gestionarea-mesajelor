#include <iostream>
#include <algorithm> 
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

void usage(char *file) {
	fprintf(stderr, "Usage: %s ID_client server_address server_port\n", file);
	exit(0);
}

// Functie care formeaza mesajul pentru subscribe / unsubscribe
string showMessage(char* input) {
	string res = "";

	// Retinem tipul actiunii (subscribe / unsubscribe)
	char* substr = strtok(input, " ");
	res += substr;

	res += " ";

	// Retinem topicul
	substr = strtok(NULL, " \n");
	res += substr;

	return res;
}

int main(int argc, char *argv[]) {
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN], id[DIM], ip[DIM], port[DIM];
	fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

	if (argc != 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;

	// Trimitem informatiile referitoare la client
	memset(buffer, 0, BUFLEN);
	memcpy(buffer, "INFO", strlen("INFO") + 1);
	n = send(sockfd, buffer, BUFLEN, 0);
	DIE(n < 0, "send");

	// ID-ul clientului
	memset(id, 0, DIM);
	memcpy(id, argv[1], strlen(argv[1]) + 1);
	n = send(sockfd, id, DIM, 0);
	DIE(n < 0, "send");

	// Adresa IP
	memset(ip, 0, DIM);
	memcpy(ip, argv[2], strlen(argv[2]) + 1);
	n = send(sockfd, ip, DIM, 0);
	DIE(n < 0, "send");

	// Portul
	memset(port, 0, DIM);
	memcpy(port, argv[3], strlen(argv[3]) + 1);
	n = send(sockfd, port, DIM, 0);
	DIE(n < 0, "send");

	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (int i = 0; i <= fdmax; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				// Daca primim mesaje pe socket-ul de tcp
				if (i == sockfd) {
					memset(buffer, 0, BUFLEN);
					ret = recv(sockfd, buffer, BUFLEN, 0);
					DIE(ret < 0, "recv");

					// Serverul a inchis conexiunea
					if (ret == 0) {
						close(sockfd);
						return 0;
					}

					// Afisam mesajul primit
					cout << buffer << endl;					
				} else {
					// se citeste de la tastatura
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// Daca se citeste "exit", se inchide conexiunea
					if (strncmp(buffer, "exit", 4) == 0) {
						// Se trimite un mesaj catre server
						n = send(sockfd, NULL, 0, 0);
						DIE(n < 0, "send");

						close(sockfd);
						￼return 0;
					}

					// se trimite mesaj la server (subscribe / unsubscribe)
					n = send(sockfd, buffer, BUFLEN, 0);
					DIE(n < 0, "send");

					cout << showMessage(buffer) << endl;
				}
			}
		}
	}

	close(sockfd);
	return 0;
}