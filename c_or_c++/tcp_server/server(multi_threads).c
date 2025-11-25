// standard i/o
#include <stdio.h>
// for using string properties
#include <string.h>
// for using system data types, like socklen_t, size_t, pid_t, clock_t etc.
#include <sys/types.h>
// for socket and its types
#include <sys/socket.h>
// for creating sockaddr_in => for getting server and clients info
#include <netinet/in.h>
// for memory management, malloc etc, that does not comes under stdio.h file
// exit() => program control
#include <stdlib.h>
// for creating threads
#include <pthread.h>

#include <unistd.h>

#define PORT 7777

typedef struct
{
    int client_fd;
    struct sockaddr_in addr;
} client_info_t;

void *handle_client(void *arg)
{
    client_info_t *client_info = (client_info_t *)arg;
    int client_fd = client_info->client_fd;
    free(client_info);

    char buffer[1024];

    printf("client connected (thread ID: %lu)\n", pthread_self());

    char msg[256];

    snprintf(msg, sizeof(msg),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n"
             "Hello from server!\n"
             "Thread ID: %lu\n",
             pthread_self());

    sleep(10);
    // char msg[] = "HTTP/1.1 200 OK\r\n\r\nHello from server!\r\n";
    send(client_fd, msg, strlen(msg), 0);
    recv(client_fd, buffer, sizeof(buffer), 0);

    close(client_fd);

    printf("client disconnected (thread ID: %lu)\n", pthread_self());

    return NULL;
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
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    // this sets the padding required to make the network request of 16 bytes;
    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;         // 2 bytes
    my_addr.sin_addr.s_addr = INADDR_ANY; // 4 bytes
    my_addr.sin_port = htons(PORT);       // 2 bytes

    if (bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind error failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 5) == -1)
    {
        perror("listen failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("multi threaded server is listening on port %d\n", PORT);

    while (1)
    {
        memset(&conn_addr, 0, sizeof(conn_addr));

        client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);
        if (client_fd == -1)
        {
            perror("accept failed");
            continue;
        }

        // allocating memory for client info before creating new thread;
        // why do we allocate new memory?
        // because if we don't then the last thread created spreads the last client_fd to each
        // and every threads created so far.
        // therefore it will create race condition across threads, we will surely get wrong data;
        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (client_info == NULL)
        {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        client_info->client_fd = client_fd;
        client_info->addr = conn_addr;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_info) != 0)
        {
            perror("pthread_create failed");
            free(client_info);
            close(client_fd);
            continue;
        }

        // now detaching this thread created when its done processing
        pthread_detach(thread);
    }

    close(socket_fd);

    return 0;
}