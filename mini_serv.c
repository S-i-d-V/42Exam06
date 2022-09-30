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

//Global
//int		serverSocket;
int		max_fd;

//Fatal
void	fatalError(int serverSocket){
	write(2, "Fatal error\n", 12);
	close(serverSocket);
	exit(1);
}

//Close sockets and free for all clients
void	freeClients(t_client* clients) {
	t_client*	tmp;

	while (clients != NULL) {
		tmp = (*clients).next;
		close((*clients).fd);
		free(clients);
		clients = tmp;
	}
}

//Add a new client to the linked list
int		AddClient(t_client** clients, int fd, int serverSocket) {
	static int lastId = -1;
	t_client*	new;
	t_client*	tmp;

	if ((new = malloc(sizeof(t_client))) == NULL || clients == NULL) {
		if (clients != NULL)
			freeClients(*clients);
		fatalError(serverSocket);
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
int		deleteClient(t_client** clients, int fd, int serverSocket) {
	t_client*	prev;
	t_client*	tmp;
	int			id;

	if (clients == NULL)
		fatalError(serverSocket);
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

void	sendToClients(t_client* clients, char* toSend, int fd) {
	while (clients != NULL) {
		if ((*clients).fd != fd)
			send((*clients).fd, toSend, strlen(toSend), 0);
		clients = (*clients).next;
	}
}

//Main
int main(int ac, char** av) {
	int		serverSocket;

	int connfd, id;
	socklen_t	len;
	struct sockaddr_in servaddr, cli; 
	fd_set	set_read;
	struct timeval	timeout;
	char	*buff;
	char	str[4200];
	char*	msg;
	ssize_t	size;

	//Check args
	if (ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	// socket create and verification 
	serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
	if (serverSocket == -1){
		write(2, "Fatal error\n", 12); 
		exit(1);
	} 

	// assign IP, PORT
	bzero(&servaddr, sizeof(servaddr)); 
	if (atoi(av[1]) <= 0)
		fatalError(serverSocket);
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(serverSocket, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatalError(serverSocket);
	if (listen(serverSocket, 10) != 0)
		fatalError(serverSocket);

	//Init client
	t_client*	clients;
	t_client*	tmp;

	if ((clients = malloc(sizeof(t_client))) == NULL)
		fatalError(serverSocket);
	clients = NULL;
	tmp = NULL;

	//Init buffer
	if ((buff = malloc(4096)) == NULL){
		free(clients);
		fatalError(serverSocket);
	}

	//Loop
	len = sizeof(cli);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	while (1) {
		//Init FD
		FD_ZERO(&set_read);
		max_fd = serverSocket;
		tmp = clients;
		while (tmp != NULL) {
			FD_SET((*tmp).fd, &set_read);
			if (max_fd < (*tmp).fd)
				max_fd = (*tmp).fd;
			tmp = (*tmp).next;
		}
		FD_SET(serverSocket, &set_read);

		if (select(max_fd + 1, &set_read, NULL, NULL, &timeout) > 0) {
			//New connexion
			if (FD_ISSET(serverSocket, &set_read)) {
				connfd = accept(serverSocket, (struct sockaddr *)&cli, &len);
				//Connexion accepted
				if (connfd >= 0) {
					id = AddClient(&clients, connfd, serverSocket);
					if (max_fd < connfd)
						max_fd = connfd;
					sprintf(str, "server: client %d just arrived\n", id);
					sendToClients(clients, str, connfd);
				}
			}
			else {
				tmp = clients;
				while (tmp != NULL) {
					connfd = (*tmp).fd;
					id = (*tmp).id;
					tmp = (*tmp).next;
					if (FD_ISSET(connfd, &set_read)) {
						size = recv(connfd, buff, 4096, 0);
						//A client disconnect
						if (size == 0) {
							id = deleteClient(&clients, connfd, serverSocket);
							if (id != -1){
								sprintf(str, "server: client %d just left\n", id);
								sendToClients(clients, str, connfd);
							}
						}
						//Send received octets to clients
						else if (size > 0) {
							msg = NULL;
							while (extract_message(&buff, &msg) != 0) {
								sprintf(str, "client %d: %s", id, msg);
								sendToClients(clients, str, connfd);
							}
						}
					}
				}
			}
		}
	}
}
