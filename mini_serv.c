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
	char				cache[4096 * 42];
	struct s_client		*next;
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
	t_client *tmp = clients;
	
	FD_ZERO(setRead);
	*maxFd = serverSocket;
	while (tmp != NULL) {
		FD_SET(tmp->fd, setRead);
		if (*maxFd < tmp->fd)
			*maxFd = tmp->fd;
		tmp = tmp->next;
	}
	FD_SET(serverSocket, setRead);
}

void	sendToClients(t_client *clients, int serverSocket, int fd, char* toSend) {
	t_client *tmp = clients;

	while (tmp != NULL) {
		if (tmp->fd != fd)
			if (send(tmp->fd, toSend, strlen(toSend), 0) < 0)
				fatalError(clients, serverSocket);
		tmp = tmp->next;
	}
}

void sendMessage(t_client *clients, int serverSocket, t_client *sender, fd_set *setRead, char *toSend){
    int i = 0;
    int clientMsgSize = strlen(sender->cache) - 1;
    char sendBuffer[4096 * 43];


    while(toSend[i]){
        sender->cache[strlen(sender->cache)] = toSend[i];
        clientMsgSize++;
        if (toSend[i] == '\n'){
            sprintf(sendBuffer, "client %d: %s", sender->id, sender->cache);
            sendToClients(clients, serverSocket, sender->fd, sendBuffer);
            bzero(&sender->cache, 4096 * 42);
        }
        i++;
    }
}

void		addClient(t_client **clients, int serverSocket, int fd) {
	static int lastId = -1;
	t_client*	new;
	t_client*	tmp = *clients;
    char        sendBuffer[128];

	if ((new = malloc(sizeof(t_client))) == NULL)
		fatalError(*clients, serverSocket);
	new->fd = fd;
	new->id = ++lastId;
	bzero(&new->cache, 4096 * 42);
	new->next = NULL;
	if (*clients == NULL)
		*clients = new;
	else {
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = new;
	}
    sprintf(sendBuffer, "server: client %d just arrived\n", new->id);
	sendToClients(*clients, serverSocket, fd, sendBuffer);
}

void		deleteClient(t_client **clients, int serverSocket, int fd) {
	t_client*	prev = NULL;
	t_client*	tmp = *clients;
	int			id = -1;
    char        sendBuffer[128];

	if (tmp != NULL && tmp->fd == fd) {
		*clients = tmp->next;
		id = tmp->id;
		close(tmp->fd);
		free(tmp);
	}
	else {
		while (tmp != NULL && tmp->fd != fd) {
			prev = tmp;
			tmp = tmp->next;
		}
		if (tmp != NULL) {
			prev->next = tmp->next;
			id = tmp->id;
			close(tmp->fd);
			free(tmp);
		}
	}
    if (id != -1){
		sprintf(sendBuffer, "server: client %d just left\n", id);
		sendToClients(*clients, serverSocket, fd, sendBuffer);
	}
}

int main(int ac, char** av) {
	int					serverSocket = -1;
	socklen_t			socketLen;
	struct sockaddr_in	servaddr, cli;
	t_client*			clients = NULL;
	t_client*			tmp = NULL;
	int 				clientFd;
	int					clientId;
	int					maxFd;
	fd_set				setRead;
	char				recvBuffer[4096 * 42];
	ssize_t				recvSize;

	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	serverSocket = initSocket(clients, &servaddr, av[1]);
	socketLen = sizeof(cli);
	while (1) {
		initFds(clients, serverSocket, &setRead, &maxFd);
		if (select(maxFd + 1, &setRead, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(serverSocket, &setRead)) {
				clientFd = accept(serverSocket, (struct sockaddr *)&cli, &socketLen);
				if (clientFd >= 0)
                    addClient(&clients, serverSocket, clientFd);
			}
			else {
				tmp = clients;
				while (tmp != NULL) {
					if (FD_ISSET(tmp->fd, &setRead)) {
						recvSize = recv(tmp->fd, recvBuffer, 4096 * 42, 0);
						if (recvSize == 0){
                            deleteClient(&clients, serverSocket, tmp->fd);
                            break;
                        }
						else if (recvSize > 0) {
							sendMessage(clients, serverSocket, tmp, &setRead, recvBuffer);
                            bzero(&recvBuffer, 4096 * 42);
						}
					}
                    tmp = tmp->next;
				}
			}
		}
	}
}
