### Mini Serv (Exam Rank 06):

```
typedef struct  s_client {
  int fd;
  int id;
  struct s_client *next;
}               t_client;
```

```
void  freeClients(t_client *clients);
void  fatalError(t_client *clients, int serverSocket);

int   initSocket(t_client *clients, struct sockaddr_in *servaddr, char *arg);
void  initFds(t_client *clients, int serverSocket, fd_set *setRead, int *maxFd);

void  sendToClients(t_client *clients, int serverSocket, int fd, char *toSend);
void  checkMessage(t_client *clients, int serverSocket, t_client *sender, fd_set *setRead, char *toSend);

int   addClient(t_client *clients, int serverSocket, int fd);
int   deleteClient(t_client *clients, int serverSocket, int fd);

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

    ...
}
```
