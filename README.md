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

int   addClient(t_client *clients, int serverSocket, int fd);
int   deleteClient(t_client *clients, int serverSocket, int fd);

void  sendToClients(t_client *clients, int serverSocket, int fd, char *toSend);
int   initSocket(t_client *clients, struct sockaddr_in servaddr, char *arg);
void  initFds(t_client *clients, int serverSocket, fd_set *setRead, int *maxFd);

int   main(int ac, char **av){
  ...
  while (1) {
    initFds(t_client *clients, int serverSocket, fd_set *setRead, int *maxFd);
    if (select(maxFd + 1, &setRead, NULL, NULL, NULL){
        if (FD_ISSET(serverSocket, &setRead)){
            if (clientFd >= 0({
                ...
            }
        }
        else{
            while (tmpClients != NULL){
                if (FD_ISSET(serverSocket &setRead)){
                    if (recvSize == 0){
                        if (clientId != -1){
                            ...
                        }
                    }
                    else if (recvSize > 0){
                        ...
                    }
                }
            }
        }
    }
}
```
