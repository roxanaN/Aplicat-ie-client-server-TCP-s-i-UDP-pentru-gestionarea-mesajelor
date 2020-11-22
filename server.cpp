#include <iostream>
#include <algorithm> 
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <vector>
#include <iterator> 
#include <unordered_map>
#include <math.h> 
#include <unordered_set>
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;

vector<char*> getArgs(char* input) {
	vector<char*> res;
	char* substr = strtok(input, " \n");
	
	while(substr) {
		res.push_back(substr);
		substr = strtok(NULL, " \n");
	}

	return res;
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	int tcpSockfd, udpSockfd;
	int newsockfd, portno;
	char buffer[BUFLEN], ip[DIM], id[DIM], port[DIM];
	struct sockaddr_in tcp_addr, udp_addr, cli_addr;
	int n, i, tcpRet, udpRet;
	socklen_t clilen;

	// socketii ai caror clienti au sf = 1
	unordered_set<int> SFs;
	// Socketi activi
	unordered_set<int> tcpSockets;
	// O mapare socket client - id client
	unordered_map<int, string> clients;
	// O mapare socket client - Mesajele pe care le-a primit, cat timp era deconectat
	unordered_map<int, vector<string>> sfMessages;
	// // O mapare topic - id clienti abonati
	unordered_map<string, unordered_set<int>> topics;

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	if (argc != 2) {
		usage(argv[0]);
	}

	// Se goleste multimea de descriptori de citire (read_fds)
	FD_ZERO(&read_fds);
	// Si multimea temporara (tmp_fds)
	FD_ZERO(&tmp_fds);

	tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpSockfd < 0, "socket");

	udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udpSockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(portno);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

	memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	tcpRet = bind(tcpSockfd, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(tcpRet < 0, "bind");

	udpRet = bind(udpSockfd, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(udpRet < 0, "bind");

	tcpRet = listen(tcpSockfd, MAX_CLIENTS);
	DIE(tcpRet < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(tcpSockfd, &read_fds);
	FD_SET(udpSockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = max(tcpSockfd, udpSockfd);

	while (1) {
		tmp_fds = read_fds; 
		tcpRet = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(tcpRet < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcpSockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcpSockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
				 	
					// Adaugam socket ul la socketii activi
					tcpSockets.insert(newsockfd);
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);

					if (newsockfd > fdmax) {
						fdmax = newsockfd;
					}
				} else if (i == udpSockfd) {
					// Daca s-a primit mesaj pe socket-ul udp
					clilen = sizeof(cli_addr);
					memset(buffer, 0, BUFLEN);
					memset((char *) &cli_addr, 0, clilen);

					n = recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr *) &cli_addr, &clilen);
					DIE(n < 0, "recv");

					char topic[50];
					memcpy(topic, buffer, 50);

					// Formam mesajul:
					// Adaugam IP-ul, portul si topicul
					string message = "";
					string IP(inet_ntoa(cli_addr.sin_addr));
					string PORT(to_string(ntohs(cli_addr.sin_port)));
					message += IP;
					message += ":";
					message += PORT;
					message += " - ";
					message += topic;
					message += " - ";

					// Adaugam continutul
					int type = buffer[IDX_TYPE];
					switch(type) {
						case INT: {
							int sign = buffer[IDX_TYPE + 1];
							int payload;

							memcpy(&payload, buffer + IDX_TYPE + 2, sizeof(payload));
							payload = ntohl(payload);

							if (sign) {
								payload = 0 - payload;
							}

							message += "INT - ";
							message += to_string(payload);

							break;
						}

						case SHORT_REAL: {
							uint16_t payload;
							memcpy(&payload, buffer + IDX_TYPE + 1, sizeof(payload));
							payload = ntohs(payload) / 100.00;

							message += "SHORT_REAL - ";
							message += to_string(payload);

							break;
						}

						case FLOAT: {
							int sign = buffer[IDX_TYPE + 1];
							int payload;
							uint8_t power = buffer[IDX_TYPE + 6];
							memcpy(&payload, buffer + IDX_TYPE + 2, sizeof(payload));
							
							if (sign) {
								payload = 0 - (double) payload / pow(10, power);
							} else {
								payload = (double) payload / pow(10, power);
							}

							message += "FLOAT - ";
							message += to_string(payload);

							break;
						}

						case STRING: {							
							char payload[MAXLEN];
							memcpy(payload, buffer + IDX_TYPE + 1, MAXLEN);
							
							message += "STRING - ";
							message += payload;

							break;
						}

						default:
							break;
					}

					unordered_set<int> subscribers = topics[topic];
					// Daca exista subscriberi
					if (subscribers.size()) {
						// Trimit mesajul clientilor abonati
						memset(buffer, 0, BUFLEN);
						memcpy(buffer, message.c_str(), message.size() + 1);
						for (auto s : subscribers) {
							// Clientul e deconectat
							if (tcpSockets.find(s) == tcpSockets.end()) {
								// Daca clientul are SF activ
								if (SFs.find(s) != SFs.end()) {
									// Retinem mesajele pe care trebuie sa le primeasca
									sfMessages[s].push_back(message);
								}
							} else {
								// Clientul e conectat si ii trimitem mesajul
								n = send(s, buffer, BUFLEN, 0);
								DIE(n < 0, "send");
							}
						}
					}
				} else if (i == 0) {
					// Daca se citeste de la tastatura
					string command;
					getline(cin, command);

					if (command == "exit") {
						// Se inchide conexiunea cu toti clientii
						for (auto s : tcpSockets) {
							send(s, NULL, 0, 0);
							close(s);
						}

						close(tcpSockfd);
						close(udpSockfd);

						return 0;				
					}
				} else {
					// Daca un client are mesaje de primit, le trimitem
					if (sfMessages[i].size()) {
						for (auto m : sfMessages[i]) {
							memset(buffer, 0, BUFLEN);
							memcpy(buffer, m.c_str(), m.size() + 1);
							n = send(i, buffer, BUFLEN, 0);
							DIE(n < 0, "send");
						}

						sfMessages[i].clear();
					} else {
						// S-au primit date pe unul din socketii de client,
						// Asa ca serverul trebuie sa le receptioneze
						memset(buffer, 0, BUFLEN);
						n = recv(i, buffer, BUFLEN, 0);
						DIE(n < 0, "recv");

						// Daca se deconecteaza
						if (n == 0) {
							// Eliminam socketul din lista de socketi activi
							tcpSockets.erase(i);
							close(i);
							cout << "Client " <<  clients[i] << " disconnected\n";

							// se scoate din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
						} else {
							// Daca am primit datele clientului conectat
							if (!strncmp(buffer, "INFO", 4)) {
								// primesc ID-ul clientului
								n = recv(i, id, DIM, 0);
								DIE(n < 0, "recv");
								clients[i] = id;

								// primesc adresa IP
								n = recv(i, ip, DIM, 0);
								DIE(n < 0, "recv");

								// Primesc datele despre port
								n = recv(i, port, DIM, 0);
								DIE(n < 0, "recv");

								// Primesc mesajul
								memset(buffer, 0, BUFLEN);
								n = recv(i, buffer, BUFLEN, 0);
								DIE(n < 0, "recv");
							}

							// Parsez mesajul pentru a accesa rapid topicul si sf-ul
							vector <char*> args = getArgs(buffer);
							string topic(args[1]);

							// Daca mesajul e de tip SUBSCRIBE
							if (!strncmp(buffer, "subscribe", 9)) {
								// Adaug socketul clientului in lista topicului
								printf("New client %s connected from %s:%s.\n", id, ip, port);
								topics[topic].insert(i);

								// Retin socket-ul care are store&forward activ
								int sf = atoi(args[2]);
								if (sf) {
									SFs.insert(i);
								}
							} else {
								// Stergem socket-ul din lista topicului dat ca argument
								auto it = find(topics[topic].begin(), topics[topic].end(), i);
								if (it != topics[topic].end()) {
									topics[topic].erase(it);
								}
							}
						}
					}
				}
			}
		}
	}

	close(tcpSockfd);
	close(udpSockfd);

	return 0;
}