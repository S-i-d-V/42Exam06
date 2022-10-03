#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct	s_client {
	int					fd;
	int					id;
	struct s_client*	next;
}				t_client;

void	freeClients(t_client *clients) {
	t_client*	tmp;

	while (clients != NULL) {
		tmp = (*clients).next;
		close((*clients).fd);
		free(clients);
		clients = tmp;
	}
}

void	fatalError(int serverSocket, t_client *clients){
	write(2, "Fatal error\n", 12);
	if (serverSocket != -1)
		close(serverSocket);
	if (clients)
		freeClients(clients);
	exit(1);
}

int		AddClient(t_client **clients, int fd, int serverSocket) {
	static int lastId = -1;
	t_client*	new;
	t_client*	tmp;

	if ((new = malloc(sizeof(t_client))) == NULL || clients == NULL) {
		fatalError(serverSocket, *clients);
	}
	new->fd = fd;
	new->id = ++lastId;
	new->next = NULL;
	if (*clients == NULL)
		*clients = new;
	else {
		tmp = *clients;
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = new;
	}
	return (new->id);
}

int		deleteClient(t_client **clients, int fd, int serverSocket) {
	t_client*	prev;
	t_client*	tmp;
	int			id;

	if (clients == NULL)
		fatalError(serverSocket, *clients);
	id = -1;
	prev = NULL;
	tmp = *clients;
	if (tmp != NULL && tmp->fd == fd) {
		*clients = tmp->next;
		id = tmp->id;
		close((*tmp).fd);
		free(tmp);
	}
	else {
		while (tmp != NULL && tmp->fd != fd) {
			prev = tmp;
			tmp = tmp->next;
		}
		if (tmp != NULL) {
			prev->next = tmp->next;
			close(tmp->fd);
			id = tmp->id;
			free(tmp);
		}
	}
	return (id);
}

void	sendToClients(int serverSocket, t_client *clients, char* toSend, int fd) {
	t_client *tmpClients = clients;

	while (tmpClients != NULL) {
		if (tmpClients->fd != fd)
			if (send(tmpClients->fd, toSend, strlen(toSend), 0) < 0)
				fatalError(serverSocket, tmpClients);
		tmpClients = tmpClients->next;
	}
}

int initSocket(t_client *clients, struct sockaddr_in *servaddr, char *arg){
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
		fatalError(serverSocket, clients);
	bzero(servaddr, sizeof(*servaddr)); 
	if (atoi(arg) <= 0)
		fatalError(serverSocket, clients);
	servaddr->sin_family = AF_INET; 
	servaddr->sin_addr.s_addr = htonl(2130706433);
	servaddr->sin_port = htons(atoi(arg));
	if ((bind(serverSocket, (const struct sockaddr *)servaddr, sizeof(*servaddr))) != 0)
		fatalError(serverSocket, clients);
	if (listen(serverSocket, 10) != 0)
		fatalError(serverSocket, clients);
	
	return (serverSocket);
}

void	initFds(fd_set *set_read, int *max_fd, t_client *clients, int serverSocket){
	t_client *tmpClients;
	
	FD_ZERO(set_read);
	*max_fd = serverSocket;
	tmpClients = clients;
	while (tmpClients != NULL) {
		FD_SET(tmpClients->fd, set_read);
		if (*max_fd < tmpClients->fd)
			*max_fd = tmpClients->fd;
		tmpClients = tmpClients->next;
	}
	FD_SET(serverSocket, set_read);
}

int main(int ac, char** av) {
	int					serverSocket = -1;
	socklen_t			socketLen;
	struct sockaddr_in	servaddr, cli;
	t_client*			clients = NULL;
	t_client*			tmpClients = NULL;
	int 				clientFd;
	int					clientId;
	int					max_fd;
	fd_set				set_read;
	char				recvBuffer[4096 * 42];
	char				sendBuffer[4096 * 42];
	ssize_t				recvSize;

	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	if ((clients = malloc(sizeof(t_client))) == NULL)
		fatalError(serverSocket, clients);
	clients = NULL;
	serverSocket = initSocket(clients, &servaddr, av[1]);
	socketLen = sizeof(cli);
	while (1) {
		initFds(&set_read, &max_fd, clients, serverSocket);
		if (select(max_fd + 1, &set_read, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(serverSocket, &set_read)) {
				clientFd = accept(serverSocket, (struct sockaddr *)&cli, &socketLen);
				if (clientFd >= 0) {
					clientId = AddClient(&clients, clientFd, serverSocket);
					if (max_fd < clientFd)
						max_fd = clientFd;
					sprintf(sendBuffer, "server: client %d just arrived\n", clientId);
					sendToClients(serverSocket, clients, sendBuffer, clientFd);
				}
			}
			else {
				tmpClients = clients;
				while (tmpClients != NULL) {
					clientFd = tmpClients->fd;
					clientId = tmpClients->id;
					tmpClients = tmpClients->next;
					if (FD_ISSET(clientFd, &set_read)) {
						recvSize = recv(clientFd, recvBuffer, 4096 * 42, 0);
						if (recvSize == 0) {
							clientId = deleteClient(&clients, clientFd, serverSocket);
							if (clientId != -1){
								sprintf(sendBuffer, "server: client %d just left\n", clientId);
								sendToClients(serverSocket, clients, sendBuffer, clientFd);
							}
						}
						else if (recvSize > 0) {
							sprintf(sendBuffer, "client %d: %s", clientId, recvBuffer);
							sendToClients(serverSocket, clients, sendBuffer, clientFd);
							bzero(&recvBuffer, 4096 * 42);
						}
					}
				}
			}
		}
	}
}