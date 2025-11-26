#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
// file control
// fnctl() => file descriptor.
#include <fcntl.h>
// helps in selecting multiple file descriptors
#include <sys/select.h>
//
#include <errno.h>
#include<time.h>

#define PORT 7777
#define MAX_CLIENTS 10

typedef struct
{
    int fd;
    time_t connect_time;
    int ready_to_send;
} client_state_t;

// without this function, we have by default blocking mode, meaning until the process is
// completed it blocks the process
// meaning doing things -> synchronously
// so by default, send() and recv() are blocking -> they pause the program until data arrives;

int set_nonblocking(int fd)
{
    // f_getfl => get current flag
    // f_setfl => set new flag
    // o_nonblock => non blocking flag
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int socket_fd, client_fd, max_sd, activity, i;
    struct sockaddr_in my_addr, conn_addr;
    socklen_t addr_size = sizeof(conn_addr);
    // for reading file descriptor;
    fd_set readfds;
    client_state_t clients[MAX_CLIENTS];
    char buffer[1024];

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = 0;
        clients[i].connect_time = 0;
        clients[i].ready_to_send = 0;
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    // allowing to reuse socket
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
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

    printf("I/O multiplexing server is listening on port %d\n", PORT);
    printf("Server can handle upto %d concurrent clients\n", MAX_CLIENTS);

    while (1)
    {
        memset(&conn_addr, 0, sizeof(conn_addr));
        // clearing the socket set;
        FD_ZERO(&readfds);

        // adding master socket to set;
        FD_SET(socket_fd, &readfds);
        max_sd = socket_fd;

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clients[i].fd;

            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        // waiting for the activity on one of the sockets;

        // what select do?
        // it helps in monitoring all the file descriptors(can be file, socket, i/o ops);
        // eg. without select() => is like picking a phone in call center one at a time
        // until the call disconnects we can't pick up any other phone calls
        // with select() => is like having a monitor all the phones, but still we can answer one at a time;
        // if we dont' select the timeout for it, without it select method will not return to it.
        // it's like we talk on phone 1 for(10 sec), then puts it on hold then talk on next phone, again put on hold after 10sec.
        
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }

        if (FD_ISSET(socket_fd, &readfds))
        {
            client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);
            if (client_fd < 0)
            {
                perror("accept");
                continue;
            }

            printf("New connection, socket fd is %d\n", client_fd);

            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd == 0)
                {
                    clients[i].fd = client_fd;
                    clients[i].connect_time = time(NULL);
                    clients[i].ready_to_send = 0;
                    printf("adding to list of sockets as %d\n", i);
                    break;
                }
            }

            if (i == MAX_CLIENTS)
            {
                printf("too many clients, rejecting connection\n");
                close(client_fd);
            }
        }

        time_t current_time = time(NULL);
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clients[i].fd;

            if (sd == 0)
            {
                continue;
            }

            if (!clients[i].ready_to_send && (current_time - clients[i].connect_time >= 10))
            {
                clients[i].ready_to_send = 1;
                char msg[256];

                snprintf(msg, sizeof(msg),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "Hello from server!\n"
                        "client no. %d\n",
                        i
                );
                send(sd, msg, strlen(msg), 0);
            }

            if(FD_ISSET(sd, &readfds)) {
                int valread = recv(sd, buffer, sizeof(buffer), 0);
                printf("valread value: %d\n", valread);

                if(valread == 0) {
                    printf("client disconnected, socket fd %d\n", sd);
                    close(sd);
                    clients[i].fd = 0;
                    clients[i].connect_time = 0;
                    clients[i].ready_to_send = 0;
                } else if(valread > 0) {
                    buffer[valread] = '\0';
                    printf("received data from socket %d: %d bytes\n", sd, valread);
                }
            }
        }
    }

    close(socket_fd);
    return 0;
}