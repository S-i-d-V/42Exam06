#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

//Utils
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
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

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

//Global
int sockfd;

int g_id;
int max_fd;

//Me
int main(int ac, char **av){
    int connfd, len;
	struct sockaddr_in servaddr, cli; 

    //Check args
    if (ac != 2){
         write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
         exit(1);
     }

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		write(2, "Fatal error\n", strlen("Fatal error\n"));
        exit(1);
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT
    int port = atoi(av[1]);
    if (port <= 0){
        write(2, "Fatal error\n", strlen("Fatal error\n"));
        exit(1);
    }
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port); 

    // Binding newly created socket to given IP and verification
    //If socket cannot bind
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		write(2, "Fatal error\n", strlen("Fatal error\n"));
        exit(1);
	}
    //If cannot listen
	if (listen(sockfd, 10) != 0) {
		write(2, "Fatal error\n", strlen("Fatal error\n"));
        exit(1);
	}
	len = sizeof(cli);

    while (1) {

    }
	//connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	//if (connfd < 0) { 
    //    //printf("server acccept failed...\n"); 
    //    write(2, "Fatal error\n", strlen("Fatal error\n"));
    //    exit(1);
    //} 
}
