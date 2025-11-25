#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 7777

void handle_client(int client_fd) {
    char buffer[1024];

    printf("client connected (PID: %d)\n", getpid());
    sleep(10);
    
    char msg[] = "HTTP/1.1 200 OK\r\n\r\nHello from server!\r\n";
    send(client_fd, msg, strlen(msg), 0);
    recv(client_fd, buffer, sizeof(buffer), 0);
    
    close(client_fd);
    
    printf("client disconnected (PID: %d)\n", getpid());
}

int main()
{
    int socket_fd, client_fd;

    struct sockaddr_in my_addr, conn_addr;
    socklen_t addr_size = sizeof(conn_addr);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 5) == -1)
    {
        perror("listen failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("multi-process server is listening on port %d\n", PORT);

    while (1)
    {
        memset(&conn_addr, 0, sizeof(conn_addr));

        client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);
        if (client_fd == -1)
        {
            perror("accpet failed");
            continue;
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            close(socket_fd);
            handle_client(client_fd);
            exit(0);
        }
        else if (pid > 0)
        {
            close(client_fd);
        }
        else
        {
            perror("fork failed");
            close(client_fd);
        }
    }

    close(socket_fd);

    return 0;
}