#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define PORT 7777
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

typedef struct
{
    int fd;
    char username[32];
    time_t connect_time;
    char read_buffer[BUFFER_SIZE];
    int read_offset;
    char write_buffer[BUFFER_SIZE];
    int write_offset;
    int write_len;
    int authenticated;
} client_state_t;

client_state_t clients[MAX_CLIENTS];

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fnctl F_GETFL");
        return -1;
    }
    return fcntl(fd, F_GETFL, flags | O_NONBLOCK);
}

void init_clients()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = 0;
        clients[i].connect_time = 0;
        clients[i].username[0] = '\0';
        clients[i].read_offset = 0;
        clients[i].write_offset = 0;
        clients[i].write_len = 0;
        clients[i].authenticated = 0;
    }
}

void reset_client(int index)
{
    clients[index].fd = 0;
    clients[index].connect_time = 0;
    clients[index].username[0] = '\0';
    clients[index].read_offset = 0;
    clients[index].write_offset = 0;
    clients[index].write_len = 0;
    clients[index].authenticated = 0;
}

int find_client_slot()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd == 0)
        {
            return i;
        }
    }
    return -1;
}

// queuing the clients message in each clients buffer;
int queue_message(int client_index, const char *message)
{
    int msg_len = strlen(message);
    int available = BUFFER_SIZE - clients[client_index].write_len;
    if (msg_len > available)
    {
        printf("Warning: write buffer full for client %d\n", client_index);
        return -1;
    }

    memcpy(clients[client_index].write_buffer + clients[client_index].write_len, message, msg_len);
    clients[client_index].write_len += msg_len;

    return 0;
}

void send_to_client(int client_index, const char *message)
{
    queue_message(client_index, message);
}

// sending message to all the connected and authenticated clients except itself;
void broadcast_message(const char *message, int sender_index)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd > 0 && i != sender_index && clients[i].authenticated)
        {
            queue_message(i, message);
        }
    }
}

void process_message(int client_index, char *message)
{
    size_t len = strlen(message);
    if (len > 0 && message[len - 1] == '\n')
    {
        message[len - 1] = '\0';
    }

    if (len > 1 && message[len - 2] == '\r')
    {
        message[len - 2] = '\0';
    }

    printf("Processing message from client %d: '%s'\n", client_index, message);

    // username setup;
    if (!clients[client_index].authenticated)
    {
        if (strlen(message) > 0 && strlen(message) < 32)
        {
            strncpy(clients[client_index].username, message, 31);
            clients[client_index].username[31] = '\0';
            clients[client_index].authenticated = 1;

            char welcome[256];
            snprintf(welcome, sizeof(welcome), "Welcome %s! You can now chat. Type /help for commands.\n", clients[client_index].username);

            send_to_client(client_index, welcome);

            char announce[256];
            snprintf(announce, sizeof(announce), "*** %s has joined the chat ***\n", clients[client_index].username);
            broadcast_message(announce, client_index);

            printf("Client %d authenticated as '%s'\n", client_index, clients[client_index].username);
        }
        else
        {
            send_to_client(client_index, "Invalid username. Please try again.\n");
        }
        return;
    }

    // handle /help commands;
    if (message[0] == '/')
    {
        if (strncmp(message, "/help", 5) == 0)
        {
            send_to_client(client_index,
                           "Commands: \n"
                           "   /help            - Show this help\n"
                           "   /list            - List online users\n"
                           "   /name <newname>  - Change your username\n"
                           "   /quit            - Disconnect\n");
        }
        else if (strncmp(message, "/list", 5) == 0)
        {
            char list[1024] = "Online users:\n";
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd > 0 && clients[i].authenticated)
                {
                    char user_line[64];
                    snprintf(user_line, sizeof(user_line), "  - %s\n", clients[i].username);
                    strcat(list, user_line);
                }
            }
            send_to_client(client_index, list);
        }
        else if (strncmp(message, "/name", 5) == 0)
        {
            char *new_name = message + 6;
            if (strlen(new_name) > 0 && strlen(new_name) < 32)
            {
                char old_name[32];
                strcpy(old_name, clients[client_index].username);
                strncpy(clients[client_index].username, new_name, 31);
                clients[client_index].username[31] = '\0';

                char announce[256];
                snprintf(announce, sizeof(announce), "*** %s is now known as %s ***\n", old_name, clients[client_index].username);

                broadcast_message(announce, -1);
            }
        }
        else if (strncmp(message, "/quit", 5) == 0)
        {
            send_to_client(client_index, "Goodbye!\n");
            close(clients[client_index].fd);

            char announce[256];
            snprintf(announce, sizeof(announce), "*** %s has left the chat ***\n", clients[client_index].username);

            broadcast_message(announce, client_index);

            reset_client(client_index);
        }
        else
        {
            send_to_client(client_index, "Unknown command. Type /help for help.\n");
        }
    }
    else
    {
        // regular chat message
        if (strlen(message) > 0)
        {
            char formatted[BUFFER_SIZE];
            snprintf(formatted, sizeof(formatted), "[%s]: %s\n", clients[client_index].username, message);
            broadcast_message(formatted, client_index);
        }
    }
}

void handle_incoming_data(int client_index)
{
    char *buffer = clients[client_index].read_buffer;
    int offset = 0;

    for (int i = 0; i < clients[client_index].read_offset; i++)
    {
        if (buffer[i] == '\n')
        {
            // this means we found complete line in that message for this client;
            char message[BUFFER_SIZE];
            int msg_len = i - offset + 1;
            memcpy(message, buffer + offset, msg_len);
            message[msg_len] = '\0';

            process_message(client_index, message);
            offset = i + 1;
        }
    }

    // if complete message is not received then move the offset to end message at the start of the buffer;
    if (offset > 0)
    {
        int remaining = clients[client_index].read_offset - offset;
        if (remaining > 0)
        {
            memmove(buffer, buffer + offset, remaining);
        }
        clients[client_index].read_offset = remaining;
    }
}

int main()
{
    int socket_fd, client_fd, max_sd, activity;
    struct sockaddr_in my_addr, conn_addr;
    socklen_t addr_size = sizeof(conn_addr);
    fd_set readfds, writefds;

    init_clients();

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt error");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("bind error");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 5) == -1)
    {
        perror("listen error");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Chat server is listening on port %d\n", PORT);
    printf("Max clients: %d\n", MAX_CLIENTS);

    while (1)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(socket_fd, &readfds);
        max_sd = socket_fd;

        // adding clients sockets to fd sets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clients[i].fd;

            if (sd > 0)
            {
                FD_SET(sd, &readfds);

                // only monitor client if they have some data to send;
                if (clients[i].write_len > clients[i].write_offset)
                {
                    FD_SET(sd, &writefds);
                }
            }

            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        activity = select(max_sd + 1, &readfds, &writefds, NULL, &timeout);

        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }

        // checking for new connection;
        if (FD_ISSET(socket_fd, &readfds))
        {
            client_fd = accept(socket_fd, (struct sockaddr *)&conn_addr, &addr_size);

            if (client_fd < 0)
            {
                perror("this client is not ready to be accepted");
                continue;
            }

            set_nonblocking(client_fd);

            int slot = find_client_slot();
            if (slot >= 0)
            {
                clients[slot].fd = client_fd;
                clients[slot].connect_time = time(NULL);

                char *client_ip = inet_ntoa(conn_addr.sin_addr);
                printf("new connection from %s, socket fd %d, slot %d\n", client_ip, client_fd, slot);

                send_to_client(slot, "Welcome to chat server!\nPlease enter your username: ");
            }
            else
            {
                printf("max clients reached, rejecting connections\n");
                close(client_fd);
            }
        }

        // handle client i/o
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clients[i].fd;

            if (sd == 0)
            {
                continue;
            }

            // handling write events;
            if (FD_ISSET(sd, &writefds))
            {
                int bytes_to_send = clients[i].write_len - clients[i].write_offset;
                int sent = send(sd, clients[i].write_buffer + clients[i].write_offset, bytes_to_send, 0);

                if (sent > 0)
                {
                    clients[i].write_offset += sent;

                    // if all data sent to this client[i], then reset the buffer;
                    if (clients[i].write_offset >= clients[i].write_len)
                    {
                        clients[i].write_offset = 0;
                        clients[i].write_len = 0;
                    }
                }
                else if (sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("sending message error to this client");
                    close(sd);
                    reset_client(i);
                }
            }
            // handle read events;
            if (FD_ISSET(sd, &readfds))
            {
                int space_available = BUFFER_SIZE - clients[i].read_offset - 1;
                int valread = recv(sd, clients[i].read_buffer + clients[i].read_offset, space_available, 0);

                if (valread == 0)
                {
                    printf("Client %d disconnected (socket %d)\n", i, sd);

                    if (clients[i].authenticated)
                    {
                        char announce[256];
                        snprintf(announce, sizeof(announce), "*** %s has left the chat ***\n", clients[i].username);
                        broadcast_message(announce, i);
                    }

                    close(sd);
                    reset_client(i);
                }
                else if (valread > 0)
                {
                    clients[i].read_offset += valread;
                    clients[i].read_buffer[clients[i].read_offset] = '\0';

                    handle_incoming_data(i);
                }
                else if (valread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("recv error");
                    close(sd);
                    reset_client(i);
                }
            }
        }
    }

    close(socket_fd);

    return 0;
}