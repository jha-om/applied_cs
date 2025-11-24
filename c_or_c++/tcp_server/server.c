#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>

#define PORT 7777

int main() {
    int socket_fd, client_fd, struct_size;
    struct sockaddr_in my_addr, conn_addr;
    socklen_t addr_size = sizeof(conn_addr);
    char buffer[1024];

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1)
    {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(socket_fd, 5) == -1) {
        perror("listen failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);


    while(1) {
        client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);
        if(client_fd == -1) {
            perror("accept failed");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
    
        printf("Client connected\n");

        sleep(10);
        
        char msg[] = "HTTP/1.1 200 OK\r\n\r\nHello from server!\r\n";
        send(client_fd, msg, strlen(msg), 0);
        recv(client_fd, buffer, sizeof(buffer), 0);

        close(client_fd);
        printf("Client disconnected\n");
    }




    close(socket_fd);
    printf("Server disconnected\n");

    return 0;
}