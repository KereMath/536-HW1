#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

int sock_fd;
int running = 1;

void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);

        if (n <= 0) {
            if (n < 0) {
                perror("ERROR receiving");
            }
            running = 0;
            break;
        }

        printf("<<< %s", buffer);
        fflush(stdout);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <connection_string>\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  Unix socket: %s @/tmp/mysocket\n", argv[0]);
        fprintf(stderr, "  TCP/IP:      %s 127.0.0.1:8080\n", argv[0]);
        exit(1);
    }

    char *conn_str = argv[1];

    // Check if it's a Unix domain socket (starts with @)
    if (conn_str[0] == '@') {
        // Abstract Unix socket
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            perror("ERROR creating socket");
            exit(1);
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        addr.sun_path[0] = '\0';  // Abstract socket
        strncpy(addr.sun_path + 1, conn_str + 1, sizeof(addr.sun_path) - 2);

        socklen_t addr_len = sizeof(sa_family_t) + 1 + strlen(conn_str + 1);

        if (connect(sock_fd, (struct sockaddr *)&addr, addr_len) < 0) {
            perror("ERROR connecting");
            exit(1);
        }

        printf("Connected to Unix socket: %s\n", conn_str);
    }
    else {
        // TCP/IP socket (format: host:port)
        char *colon = strchr(conn_str, ':');
        if (!colon) {
            fprintf(stderr, "ERROR: TCP connection string must be in format host:port\n");
            exit(1);
        }

        *colon = '\0';
        char *host = conn_str;
        int port = atoi(colon + 1);

        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            perror("ERROR creating socket");
            exit(1);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
            perror("ERROR invalid address");
            exit(1);
        }

        if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("ERROR connecting");
            exit(1);
        }

        printf("Connected to TCP socket: %s:%d\n", host, port);
    }

    // Start receiver thread
    pthread_t recv_tid;
    if (pthread_create(&recv_tid, NULL, receive_thread, NULL) != 0) {
        perror("ERROR creating thread");
        exit(1);
    }

    // Main loop - read from stdin and send
    char input[BUFFER_SIZE];
    printf("Ready to send commands. Type your commands and press Enter.\n");
    printf("Commands: REQUEST <duration>, UPGRADE <duration>, QUIT\n");
    printf(">>> ");
    fflush(stdout);

    while (running && fgets(input, BUFFER_SIZE, stdin) != NULL) {
        if (send(sock_fd, input, strlen(input), 0) < 0) {
            perror("ERROR sending");
            running = 0;
            break;
        }

        // Check if user sent QUIT
        if (strncmp(input, "QUIT", 4) == 0) {
            printf("Quitting...\n");
            sleep(1);  // Give time for response
            running = 0;
            break;
        }

        printf(">>> ");
        fflush(stdout);
    }

    running = 0;
    pthread_join(recv_tid, NULL);
    close(sock_fd);

    printf("\nDisconnected.\n");
    return 0;
}
