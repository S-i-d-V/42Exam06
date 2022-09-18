#include <stdio.h>
#include <stdlib.h>

int main(int ac, char **av){
    if (ac != 2){
        printf("Wrong numer of arguments\n");
        exit(1);
    }
    printf("Port: '%s'\n", av[1]);
}