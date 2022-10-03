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

void	initFds(t_client *clients, int serverSocket, fd_set *set_read, int *max_fd){
	t_client *tmpClients = clients;
	
	FD_ZERO(set_read);
	*max_fd = serverSocket;
	while (tmpClients != NULL) {
		FD_SET(tmpClients->fd, set_read);
		if (*max_fd < tmpClients->fd)
			*max_fd = tmpClients->fd;
		tmpClients = tmpClients->next;
	}
	FD_SET(serverSocket, set_read);
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

int		deleteClient(t_client **clients, int fd) {
	t_client *tmp = *clients;
    t_client *toDel = NULL;
    int id = -1;

    while (tmp->next){
        if (tmp->next->fd == fd){
            toDel = tmp->next;
            tmp->next = tmp->next->next;
            close(toDel->fd);
            id = toDel->id;
            free(toDel);
            break ;
        }
        tmp = tmp->next;
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
		fatalError(clients, serverSocket);
	clients = NULL;
	serverSocket = initSocket(clients, &servaddr, av[1]);
	socketLen = sizeof(cli);
	while (1) {
		initFds(clients, serverSocket, &set_read, &max_fd);
		if (select(max_fd + 1, &set_read, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(serverSocket, &set_read)) {
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
					if (FD_ISSET(clientFd, &set_read)) {
						recvSize = recv(clientFd, recvBuffer, 4096 * 42, 0);
						if (recvSize == 0) {
							clientId = deleteClient(&clients, clientFd);
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
