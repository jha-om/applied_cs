#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>

#define PORT 7777

int main() {
    int socket_fd, client_fd;
    // sockaddr_in => {transport address, port_number};
    struct sockaddr_in my_addr, conn_addr;
    socklen_t addr_size = sizeof(conn_addr);
    char buffer[1024];

    // socket => {type of internet[address family internet](ipv4, ipv6), socket_type(sock_stream => ipv4, sock_dgram => ipv6, sock_raw => for raw bytes), protocol(0 => apply best according to yourself)}
    // return type of socket;
    // in layman terms => basically which section of network is free for this program
    // in this case, socket_fd => 3, because 0 => stdin, 1 => stdout, 2 => stderr, so 3 is the next available slot;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1)
    {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    //                                    {sin_family,              sin_port,            ip address}
    // for delivering network messages => {address_family(country), port(apartment no.), sin_addr.s_addr(server listening)}
    // af_inet => address_family(ipv4)_internet;
    my_addr.sin_family = AF_INET;
    
    // htons => network byte order
    // h(host) - to - n(network) - s(short(16 bit number for ports));

    // why to use htons?
    // because different computers store numbers differently;

    // why not just simple my_addr.sin_port = PORT?, what's the issue in this?
    // it might work on my computer(classic say "it works on my computer");
    // htons => converts port number to standard network byte order;
    my_addr.sin_port = htons(PORT);

    // inaddr_any => listen to any network to this connection;
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 2) == -1) {
        perror("listen failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);


    while(1) {
        memset(&conn_addr, 0, sizeof(conn_addr));

        client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);
        if(client_fd == -1) {
            perror("accept failed");
            continue;
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