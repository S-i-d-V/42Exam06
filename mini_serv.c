#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>

//Struct
typedef struct	s_client {
	int					fd;
	int					id;
	struct s_client*	next;
}				t_client;

//Close sockets and free for all clients
void	freeClients(t_client *clients) {
	t_client*	tmp;

	while (clients != NULL) {
		tmp = clients->next;
		close(clients->fd);
		free(clients);
		clients = tmp;
	}
}

//Fatal
void	fatalError(t_client *clients, int serverSocket) {
	write(2, "Fatal error\n", 12);
	if (serverSocket != -1)
		close(serverSocket);
	if (clients)
		freeClients(clients);
	exit(1);
}

//Init socket
int initSocket(t_client *clients, struct sockaddr_in *servaddr, char *arg){
	// socket create and verification 
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == -1)
		fatalError(clients, serverSocket);
	// assign IP, PORT
	bzero(servaddr, sizeof(*servaddr)); 
	if (atoi(arg) <= 0)
		fatalError(clients, serverSocket);
	servaddr->sin_family = AF_INET; 
	servaddr->sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr->sin_port = htons(atoi(arg));
	// Binding newly created socket to given IP and verification 
	if ((bind(serverSocket, (const struct sockaddr *)servaddr, sizeof(*servaddr))) != 0)
		fatalError(clients, serverSocket);
	if (listen(serverSocket, 10) != 0)
		fatalError(clients, serverSocket);
	
	return (serverSocket);
}

//Init fds
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

//Add a new client to the linked list
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

//Remove a client from the linked list
int		deleteClient(t_client **clients, int serverSocket, int fd) {
	t_client*	prev;
	t_client*	tmp;
	int			id = -1;

	if (clients == NULL)
		fatalError(*clients, serverSocket);
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

//Send message
void	sendToClients(t_client *clients, int serverSocket, int fd, char* toSend) {
	t_client *tmpClients = clients;

	while (tmpClients != NULL) {
		if (tmpClients->fd != fd)
			if (send(tmpClients->fd, toSend, strlen(toSend), 0) < 0)
				fatalError(clients, serverSocket);
		tmpClients = tmpClients->next;
	}
}

//Main
int main(int ac, char** av) {
	//Socket
	int					serverSocket = -1;
	socklen_t			socketLen;
	struct sockaddr_in	servaddr, cli;

	//Client
	t_client*			clients = NULL;
	t_client*			tmpClients = NULL;
	int 				clientFd;
	int					clientId;

	//FD
	int					max_fd;
	fd_set				set_read;

	//Buffers
	char				recvBuffer[4096 * 42];
	char				sendBuffer[4096 * 42];
	ssize_t				recvSize;

	//Check args
	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	//Init clients
	if ((clients = malloc(sizeof(t_client))) == NULL)
		fatalError(clients, serverSocket);
	clients = NULL;
	
	//Init socket
	serverSocket = initSocket(clients, &servaddr, av[1]);

	//Loop
	socketLen = sizeof(cli);
	while (1) {
		//Init FD
		initFds(clients, serverSocket, &set_read, &max_fd);

		if (select(max_fd + 1, &set_read, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(serverSocket, &set_read)) {
				//I try to accept a new connexion
				clientFd = accept(serverSocket, (struct sockaddr *)&cli, &socketLen);
				//If the connexion succeed
				if (clientFd >= 0) {
					//Add a client
					clientId = addClient(&clients, serverSocket, clientFd);
					//Fill the sendBuffer with the formated connexion message
					sprintf(sendBuffer, "server: client %d just arrived\n", clientId);
					sendToClients(clients, serverSocket, clientFd, sendBuffer);
				}
			}
			else {
				//I use a temp copy of my clients
				tmpClients = clients;
				//I loop throught my clients
				while (tmpClients != NULL) {
					clientFd = tmpClients->fd;
					clientId = tmpClients->id;
					tmpClients = tmpClients->next;
					//If the FD is set
					if (FD_ISSET(clientFd, &set_read)) {
						//I try to receive octets send by the fd
						recvSize = recv(clientFd, recvBuffer, 4096 * 42, 0);
						//A client disconnect
						if (recvSize == 0) {
							clientId = deleteClient(&clients, serverSocket, clientFd);
							if (clientId != -1){
								//Fill the sendBuffer with the formated disconnexion message
								sprintf(sendBuffer, "server: client %d just left\n", clientId);
								sendToClients(clients, serverSocket, clientFd, sendBuffer);
							}
						}
						//Send received octets to clients
						else if (recvSize > 0) {
							//Fill the sendBuffer with the formated message
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
