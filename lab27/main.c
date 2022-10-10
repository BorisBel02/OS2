#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


typedef struct pendingData{
    char* data;
    size_t size;
} pendingData;


char err_msg[256];
int err;

struct sockaddr_in server_address;
char client_msg[BUFSIZ], compress = 0, shutdown_server = 0;

int write_msg(struct pollfd* pollfd_in, struct pollfd* pollfd_out, struct pendingData* pending_data, char* msg, ssize_t msg_size){
    if(pending_data->size > 0){
        if(write(pollfd_out->fd, pending_data->data, pending_data->size) == -1){
            strerror_r(errno, err_msg, 255);
            printf("writing into the socket: %d failed. Closing connection.\nError: %s\n", pollfd_out->fd, err_msg);

            if(close(pollfd_out->fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
            }
            if(close(pollfd_in->fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
            }
            pollfd_out->fd = -1;
            pollfd_in->fd = -1;
            return -1;
        }
        pending_data->size = 0;
        free(pending_data->data);
    }

    if(msg_size > 0) {
        if (write(pollfd_out->fd, msg, msg_size) == -1) {
            strerror_r(errno, err_msg, 255);
            printf("writing into the socket: %d failed. Closing connection.\nError: %s\n", pollfd_out->fd, err_msg);

            if (close(pollfd_out->fd) == -1) {
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
            }
            if (close(pollfd_in->fd) == -1) {
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
            }
            return -1;
        }
    }
    return 0;
}
int accept_new_connections(int listen_sfd, struct pollfd* fds_in, nfds_t* nfds_in, struct pollfd* fds_out, nfds_t* nfds_out){
    printf("starting accepting connections\n");
    if(*nfds_in < 511){//
        //printf("accepting\n");
        int new_sfd = accept(listen_sfd, NULL, NULL);
        if(new_sfd == -1){
            strerror_r(errno, err_msg, 255);
            printf("accept failed: %s\n", err_msg);
            return -1;
        }
        fds_in[*nfds_in].fd = new_sfd;
        fds_in[*nfds_in].events = POLLIN | POLLOUT;

        int new_connection_sfd;
        if((new_connection_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            strerror_r(errno, err_msg, 255);
            printf("creating server socket failed: %s\n", err_msg);
            if(close(new_sfd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
                return -1;
            }
            fds_in[*nfds_in].fd = -1;
            return -1;
        }
        if(connect(new_connection_sfd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
            strerror_r(errno, err_msg, 255);
            printf("connection failed failed: %s\n", err_msg);
            if(close(new_sfd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
                return -1;
            }
            if(close(new_connection_sfd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
                return -1;
            }                    }
        fds_out[*nfds_in].fd = new_connection_sfd;
        fds_out[*nfds_in].events = POLLIN | POLLOUT;
        printf("New connection! Client socket = %d, server socket = %d\n", fds_in[*nfds_in].fd, fds_out[*nfds_in].fd);
        ++*nfds_in;
        ++*nfds_out;
    }
    else {
        int sfd;
        sfd = accept(listen_sfd, NULL, NULL);
        if (close(sfd) == -1) {
            strerror_r(errno, err_msg, 255);
            printf("close failed: %s\n", err_msg);
            shutdown_server = 1;
            return -1;
        }
    }
    return 0;
}

void handle_connections(struct pollfd* fds_in, nfds_t* nfds_in, struct pollfd* fds_out, nfds_t* nfds_out, struct pendingData* pending_messages){
    for(int i = 0; i < *nfds_in; ++i){
        if(fds_in[i].revents == 0){
            continue;
        }

        if(fds_in[i].revents != POLLIN && fds_in[i].revents != POLLOUT && fds_in[i].revents != (POLLIN | POLLOUT)){
            printf("Error on socket: %d. revents = %d. Closing connection.\n", fds_in[i].fd, fds_in[i].revents);
            if(close(fds_in[i].fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
                return;
            }
            if(close(fds_out[i].fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                shutdown_server = 1;
                return;
            }
            --*nfds_in;

            --*nfds_out;

            compress = 1;
        }

        ssize_t msg_size = 0;
        if(fds_in[i].revents & POLLIN) {
            //printf("reading\n");
            if ((msg_size = read(fds_in[i].fd, client_msg, BUFSIZ)) == -1) {
                strerror_r(errno, err_msg, 255);
                printf("read failed: %s\n", err_msg);
                return;
            }
            if (msg_size == 0) {
                if (write_msg(&fds_in[i], &fds_out[i], &pending_messages[i], client_msg, msg_size) == -1) {
                    if (shutdown_server) {
                        return;
                    }
                    compress = 1;
                    --*nfds_in;
                    --*nfds_out;
                    continue;
                }
                printf("Connection closed.\n");
                if (close(fds_in[i].fd) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown_server = 1;
                    return;
                }
                if (close(fds_out[i].fd) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown_server = 1;
                    return;
                }
                fds_in[i].fd = -1;
                --*nfds_in;

                fds_out[i].fd = -1;
                --*nfds_out;

                compress = 1;
                continue;
            }
            printf("Get msg: %s\n", client_msg);
        }
        if(fds_out[i].revents & POLLOUT) {
            //printf("writing\n");
            if (write_msg(&fds_in[i], &fds_out[i], &pending_messages[i], client_msg, msg_size) == -1) {
                if (shutdown_server) {
                    return;
                }
                compress = 1;
                --*nfds_in;
                --*nfds_out;
            }
        }
        else if(msg_size > 0){
            pending_messages[i].data = (char*)realloc(pending_messages[i].data, (pending_messages[i].size + msg_size)*sizeof(char));
            //using realloc may be unsafe
            strcpy(&pending_messages[i].data[pending_messages[i].size], client_msg);
            pending_messages[i].size += msg_size;
        }

    }
}
int main(int argc, char* argv[]) {
    if(argc != 4){
        printf("wrong argument quantity\n");
        exit(0);
    }
    int listen_sfd, listen_port, trans_port;
    listen_port = atoi(argv[1]);
    trans_port = atoi(argv[3]);

    if((listen_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("creating listen socket failed: %s\n", err_msg);
        exit(errno);
    }
    struct sockaddr_in proxy_address;
    proxy_address.sin_addr.s_addr = INADDR_ANY;
    proxy_address.sin_family = AF_INET;
    proxy_address.sin_port = htons(listen_port);

    if(bind(listen_sfd, (struct sockaddr*)&proxy_address, sizeof(proxy_address)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("bind failed: %s\n", err_msg);
        close(listen_sfd);
        exit(errno);
    }
    if(listen(listen_sfd, 10) == -1){
        strerror_r(errno, err_msg, 255);
        printf("listening failed: %s\n", err_msg);
        close(listen_sfd);
        exit(errno);
    }

    if((err = inet_pton(AF_INET, argv[2], &(server_address.sin_addr))) != 1){
        if(err == 0){
            printf("second argument does not representing an ipv4 address\n");
        }
        else{
            strerror_r(errno, err_msg, 255);
            printf("Converting from string to ipv4 failed: %s\n", err_msg);
        }
        close(listen_sfd);
        exit(EXIT_FAILURE);
    }
    server_address.sin_port = htons(trans_port);
    server_address.sin_family = AF_INET;


    nfds_t nfds_in = 0, nfds_out = 0;
    struct pollfd all_fds[1021];
    struct pollfd* fds_in = &all_fds[1];
    struct pollfd* fds_out = &all_fds[511];

    all_fds[0].fd = listen_sfd;
    all_fds[0].events = POLLIN;
    int timeout = 60 * 1000;

    struct pendingData pending_messages_to_server[510], pending_messages_to_client[510];
    for(int i = 0; i < 510; ++i){
        pending_messages_to_client[i].data = NULL;
        pending_messages_to_client[i].size = 0;
        pending_messages_to_server[i].data = NULL;
        pending_messages_to_server[i].data = 0;
    }

    for(;;){
        err = poll(all_fds, nfds_in + 1, timeout);
        if(err == -1){
            strerror_r(errno, err_msg, 255);
            printf("poll failed: %s\n", err_msg);
            break;
        }
        if(err == 0){
            printf("poll timed out. shutdown_server.\n");
            for (int i = 0; i < nfds_in; ++i) {
                if(close(fds_in[i].fd) == -1){
                    strerror_r(errno, err_msg, 255);
                    printf("closing socket: %d failed. Error: %s", fds_in[i].fd, err_msg);
                }
            }
            break;
        }
        if(nfds_out > 0) {
            err = poll(fds_out, nfds_out, timeout);
        }
        if (err == -1) {
            strerror_r(errno, err_msg, 255);
            printf("poll failed: %s\n", err_msg);
            break;
        }
        if (err == 0) {
            printf("poll timed out. shutdown_server.\n");
            for (int i = 0; i < nfds_in; ++i) {
                if (close(fds_in[i].fd) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("closing socket: %d failed. Error: %s", fds_in[i].fd, err_msg);
                }
            }
            break;
        }

        //handling clients connections
        handle_connections(fds_in, &nfds_in, fds_out, &nfds_out, pending_messages_to_server);
        if(shutdown_server){
            break;
        }
        //handling servers connection
        handle_connections(fds_out, &nfds_out, fds_in, &nfds_in, pending_messages_to_client);

        if(shutdown_server){
            break;
        }
        if(all_fds[0].revents & POLLIN){
            if(accept_new_connections(listen_sfd, fds_in, &nfds_in, fds_out, &nfds_out) == -1)  {
                break;
            }
        }
        else if(all_fds[0].revents != 0){
            shutdown_server = 1;
            break;
        }
        if(compress){
            int offset = 0;
            for(int i = 1; i < nfds_in; ++i){
                while(fds_in[i + offset].fd == -1){
                    ++offset;
                }
                if(offset){
                    fds_in[i] = fds_in[i + offset];
                    fds_out[i] = fds_out[i + offset];
                    pending_messages_to_server[i] = pending_messages_to_server[i + offset];
                    pending_messages_to_client[i] = pending_messages_to_client[i + offset];
                }
            }
            compress = 0;
        }
    }
    if(shutdown_server){
        for(int i = 1; i < nfds_in; ++i){
            if(close(fds_in[i].fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
            }
            if(close(fds_out[i].fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
            }
        }
        if(close(listen_sfd) == -1){
            strerror_r(errno, err_msg, 255);
            printf("close failed: %s\n", err_msg);
        }
        return EXIT_FAILURE;
    }
    return 0;
}
