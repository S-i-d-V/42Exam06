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
		tmp = clients->next;
		close(clients->fd);
		free(clients);
		clients = tmp;
	}
}

void	fatalError(t_client *clients, int serverSocket) {
	write(2, "Fatal error\n", 12);
	if (serverSocket != -1)
		close(serverSocket);
	if (clients)
		freeClients(clients);
	exit(1);
}

int initSocket(t_client *clients, struct sockaddr_in *servaddr, char *arg){
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == -1)
		fatalError(clients, serverSocket);
	bzero(servaddr, sizeof(*servaddr)); 
	if (atoi(arg) <= 0)
		fatalError(clients, serverSocket);
	servaddr->sin_family = AF_INET; 
	servaddr->sin_addr.s_addr = htonl(2130706433);
	servaddr->sin_port = htons(atoi(arg));
	if ((bind(serverSocket, (const struct sockaddr *)servaddr, sizeof(*servaddr))) != 0)
		fatalError(clients, serverSocket);
	if (listen(serverSocket, 10) != 0)
		fatalError(clients, serverSocket);
	
	return (serverSocket);
}

void	initFds(t_client *clients, int serverSocket, fd_set *setRead, int *maxFd){
	t_client *tmpClients = clients;
	
	FD_ZERO(setRead);
	*maxFd = serverSocket;
	while (tmpClients != NULL) {
		FD_SET(tmpClients->fd, setRead);
		if (*maxFd < tmpClients->fd)
			*maxFd = tmpClients->fd;
		tmpClients = tmpClients->next;
	}
	FD_SET(serverSocket, setRead);
}

int		addClient(t_client **clients, int serverSocket, int fd) {
	static int lastId = -1;
	t_client*	new;
	t_client*	tmp;

	if ((new = malloc(sizeof(t_client))) == NULL)
		fatalError(*clients, serverSocket);
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

int		deleteClient(t_client **clients, int serverSocket, int fd) {
	t_client*	prev;
	t_client*	tmp;
	int			id = -1;

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

void	sendToClients(t_client *clients, int serverSocket, int fd, char* toSend) {
	t_client *tmpClients = clients;

	while (tmpClients != NULL) {
		if (tmpClients->fd != fd)
			if (send(tmpClients->fd, toSend, strlen(toSend), 0) < 0)
				fatalError(clients, serverSocket);
		tmpClients = tmpClients->next;
	}
}

int main(int ac, char** av) {
	int					serverSocket = -1;
	socklen_t			socketLen;
	struct sockaddr_in	servaddr, cli;
	t_client*			clients = NULL;
	t_client*			tmpClients = NULL;
	int 				clientFd;
	int					clientId;
	int					maxFd;
	fd_set				setRead;
	char				recvBuffer[4096 * 42];
	char				sendBuffer[4096 * 42];
	ssize_t				recvSize;

	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	if ((clients = malloc(sizeof(t_client))) == NULL)
		fatalError(clients, serverSocket);
	clients = NULL;
	serverSocket = initSocket(clients, &servaddr, av[1]);
	socketLen = sizeof(cli);
	while (1) {
		initFds(clients, serverSocket, &setRead, &maxFd);

		if (select(maxFd + 1, &setRead, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(serverSocket, &setRead)) {
				clientFd = accept(serverSocket, (struct sockaddr *)&cli, &socketLen);
				if (clientFd >= 0) {
					clientId = addClient(&clients, serverSocket, clientFd);
					sprintf(sendBuffer, "server: client %d just arrived\n", clientId);
					sendToClients(clients, serverSocket, clientFd, sendBuffer);
				}
			}
			else {
				tmpClients = clients;
				while (tmpClients != NULL) {
					clientFd = tmpClients->fd;
					clientId = tmpClients->id;
					tmpClients = tmpClients->next;
					if (FD_ISSET(clientFd, &setRead)) {
						recvSize = recv(clientFd, recvBuffer, 4096 * 42, 0);
						if (recvSize == 0) {
							clientId = deleteClient(&clients, serverSocket, clientFd);
							if (clientId != -1){
								sprintf(sendBuffer, "server: client %d just left\n", clientId);
								sendToClients(clients, serverSocket, clientFd, sendBuffer);
							}
						}
						else if (recvSize > 0) {
							sprintf(sendBuffer, "client %d: %s", clientId, recvBuffer);
							sendToClients(clients, serverSocket, clientFd, sendBuffer);
							bzero(&recvBuffer, 4096 * 42);
						}
					}
				}
			}
		}
	}
}
