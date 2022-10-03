#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

//Given
int extract_message(char **buf, char **msg) {
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i]) {
		if ((*buf)[i] == '\n') {
			newbuf = malloc(sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == NULL)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

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
		tmp = (*clients).next;
		close((*clients).fd);
		free(clients);
		clients = tmp;
	}
}

//Fatal
void	fatalError(int serverSocket, t_client *clients){
	write(2, "Fatal error\n", 12);
	if (serverSocket != -1)
		close(serverSocket);
	if (clients)
		freeClients(clients);
	exit(1);
}

//Add a new client to the linked list
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

//Remove a client from the linked list
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

void	sendToClients(t_client *clients, char* toSend, int fd) {
	while (clients != NULL) {
		if ((*clients).fd != fd)
			send((*clients).fd, toSend, strlen(toSend), 0);
		clients = (*clients).next;
	}
}

//DEBUG
void	displayBuffer(char *buffer){
	int i = 0;

	printf("'");
	while (buffer[i]){
		if (buffer[i] == '\n')
			printf("\\n");
		else
			printf("%c", buffer[i]);
		i++;
	}
	printf("'\n");
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
	int		max_fd;
	fd_set	set_read;
	struct timeval	timeout;

	//Messages
	char*	msg;
	ssize_t	recvSize;

	//Check args
	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	//Init client
	if ((clients = malloc(sizeof(t_client))) == NULL)
		fatalError(serverSocket, clients);
	clients = NULL;

	// socket create and verification 
	serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
	if (serverSocket == -1)
		fatalError(serverSocket, clients);
	// assign IP, PORT
	bzero(&servaddr, sizeof(servaddr)); 
	if (atoi(av[1]) <= 0)
		fatalError(serverSocket, clients);
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
	// Binding newly created socket to given IP and verification 
	if ((bind(serverSocket, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatalError(serverSocket, clients);
	if (listen(serverSocket, 10) != 0)
		fatalError(serverSocket, clients);

	//Init buffer
	char	*recvBuffer;
	char	sendBuffer[4200];
	

	if ((recvBuffer = malloc(4096 * sizeof(char))) == NULL){
		free(clients);
		fatalError(serverSocket, clients);
	}

	//Loop
	socketLen = sizeof(cli);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	while (1) {
		//Init FD
		FD_ZERO(&set_read);
		max_fd = serverSocket;
		tmpClients = clients;
		while (tmpClients != NULL) {
			FD_SET(tmpClients->fd, &set_read);
			if (max_fd < tmpClients->fd)
				max_fd = tmpClients->fd;
			tmpClients = tmpClients->next;
		}
		FD_SET(serverSocket, &set_read);

		//I check if the max_fd + 1 is set
		if (select(max_fd + 1, &set_read, NULL, NULL, &timeout) > 0) {
			//If the FD is set
			if (FD_ISSET(serverSocket, &set_read)) {
				//I try to accept a new connexion
				clientFd = accept(serverSocket, (struct sockaddr *)&cli, &socketLen);
				//If the connexion succeed
				if (clientFd >= 0) {
					//Add a client
					clientId = AddClient(&clients, clientFd, serverSocket);
					if (max_fd < clientFd)
						max_fd = clientFd;

					//Server output
					printf("server: client %d just arrived\n", clientId);
					//Fill the sendBuffer with the formated connexion message
					sprintf(sendBuffer, "server: client %d just arrived\n", clientId);
					sendToClients(clients, sendBuffer, clientFd);
				}
			}
			//If the fd is not set
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
						recvSize = recv(clientFd, recvBuffer, 4096, 0);
						//A client disconnect
						if (recvSize == 0) {
							clientId = deleteClient(&clients, clientFd, serverSocket);
							if (clientId != -1){
								//Server output
								printf("server: client %d just left\n", clientId);
								//Fill the sendBuffer with the formated disconnexion message
								sprintf(sendBuffer, "server: client %d just left\n", clientId);
								sendToClients(clients, sendBuffer, clientFd);
							}
						}
						//Send received octets to clients
						else if (recvSize > 0) {
							//DEBUG
							//printf("Message :\n");
							//printf("recvBuffer :");
							displayBuffer(recvBuffer);

							msg = NULL;
							while (extract_message(&recvBuffer, &msg) != 0) {
								//Server output
								printf("client %d: %s", clientId, msg);
								//Fill the sendBuffer with the formated message
								sprintf(sendBuffer, "client %d: %s", clientId, msg);
								sendToClients(clients, sendBuffer, clientFd);

								//DEBUG
								//printf("sendBuffer :");
								//displayBuffer(sendBuffer);
							}

							//DEBUG
							printf("\n");
						}
					}
				}
			}
		}
	}
}
