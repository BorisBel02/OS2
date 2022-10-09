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

int write_msg(struct pollfd* pollfd_in, struct pollfd* pollfd_out, struct pendingData* pending_data, char* msg, ssize_t msg_size, char* shutdown){
    if(pending_data->size > 0){
        if(write(pollfd_out->fd, pending_data->data, pending_data->size) == -1){
            strerror_r(errno, err_msg, 255);
            printf("writing into the socket: %d failed. Closing connection.\nError: %s\n", pollfd_out->fd, err_msg);

            if(close(pollfd_out->fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                *shutdown = 1;
            }
            if(close(pollfd_in->fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                *shutdown = 1;
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
                *shutdown = 1;
            }
            if (close(pollfd_in->fd) == -1) {
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
                *shutdown = 1;
            }
            return -1;
        }
    }
    return 0;
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

    struct sockaddr_in server_address;
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

    char client_msg[BUFSIZ], compress = 0, shutdown = 0;

    nfds_t nfds_in = 1, nfds_out = 0, new_nfds_in, new_nfds_out;
    struct pollfd fds_in[511], fds_out[510];

    fds_in[0].fd = listen_sfd;
    fds_in[0].events = POLLIN;
    int timeout = 60 * 1000;

    struct pendingData pending_messages_to_server[510], pending_messages_to_client[510];
    for(int i = 0; i < 510; ++i){
        pending_messages_to_client[i].data = NULL;
        pending_messages_to_client[i].size = 0;
        pending_messages_to_server[i].data = NULL;
        pending_messages_to_server[i].data = 0;
    }

    for(;;){
        err = poll(fds_in, nfds_in, timeout);
        if(err == -1){
            strerror_r(errno, err_msg, 255);
            printf("poll failed: %s\n", err_msg);
            break;
        }
        if(err == 0){
            printf("poll timed out. Shutdown.\n");
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

            if (err == -1) {
                strerror_r(errno, err_msg, 255);
                printf("poll failed: %s\n", err_msg);
                break;
            }
            if (err == 0) {
                printf("poll timed out. Shutdown.\n");
                for (int i = 0; i < nfds_in; ++i) {
                    if (close(fds_in[i].fd) == -1) {
                        strerror_r(errno, err_msg, 255);
                        printf("closing socket: %d failed. Error: %s", fds_in[i].fd, err_msg);
                    }
                }
                break;
            }
        }
        new_nfds_in = 0;
        new_nfds_out = 0;
        //handling clients connections
        for(int i = 0; i < nfds_in; ++i){
            if(fds_in[i].revents == 0){
                continue;
            }

            if(fds_in[i].revents != POLLIN && fds_in[i].revents != POLLOUT && fds_in[i].revents != (POLLIN | POLLOUT)){
                if(fds_in[i].fd == listen_sfd){
                    printf("Error on listening socket. revents = %d. Shutdown.", fds_in[i].revents);
                    shutdown = 1;
                    break;
                }
                printf("Error on socket: %d. revents = %d. Closing connection.\n", fds_in[i].fd, fds_in[i].revents);
                if(close(fds_in[i].fd) == -1){
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown = 1;
                    break;
                }
                if(close(fds_out[i].fd) == -1){
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown = 1;
                    break;
                }
                --nfds_in;

                --nfds_out;

                compress = 1;
            }
            
            if(fds_in[i].fd == listen_sfd){
                //printf("starting accepting connections\n");
                if(nfds_in < 511){//
                    //printf("accepting\n");
                    int new_sfd = accept(listen_sfd, NULL, NULL);
                    if(new_sfd == -1){
                        strerror_r(errno, err_msg, 255);
                        printf("accept failed: %s\n", err_msg);
                        break;
                    }
                    fds_in[nfds_in].fd = new_sfd;
                    fds_in[nfds_in].events = POLLIN | POLLOUT;

                    int new_connection_sfd;
                    if((new_connection_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
                        strerror_r(errno, err_msg, 255);
                        printf("creating server socket failed: %s\n", err_msg);
                        if(close(new_sfd) == -1){
                            strerror_r(errno, err_msg, 255);
                            printf("close failed: %s\n", err_msg);
                            shutdown = 1;
                            break;
                        }
                        fds_in[nfds_in].fd = -1;
                        break;
                    }
                    if(connect(new_connection_sfd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
                        strerror_r(errno, err_msg, 255);
                        printf("connection failed failed: %s\n", err_msg);
                        if(close(new_sfd) == -1){
                            strerror_r(errno, err_msg, 255);
                            printf("close failed: %s\n", err_msg);
                            shutdown = 1;
                            break;
                        }
                        if(close(new_connection_sfd) == -1){
                            strerror_r(errno, err_msg, 255);
                            printf("close failed: %s\n", err_msg);
                            shutdown = 1;
                            break;
                        }                    }
                    fds_out[nfds_in - 1].fd = new_connection_sfd;
                    fds_out[nfds_in - 1].events = POLLIN | POLLOUT;
                    printf("New connection! Client socket = %d, server socket = %d\n", fds_in[nfds_in].fd, fds_out[nfds_in-1].fd);
                    ++new_nfds_out;
                    ++new_nfds_in;
                }
                else {
                    int sfd;
                    sfd = accept(listen_sfd, NULL, NULL);
                    if (close(sfd) == -1) {
                        strerror_r(errno, err_msg, 255);
                        printf("close failed: %s\n", err_msg);
                        shutdown = 1;
                        break;
                    }
                }
            }
            else{
                ssize_t msg_size = 0;
                if(fds_in[i].revents & POLLIN) {
                    //printf("reading\n");
                    if ((msg_size = read(fds_in[i].fd, client_msg, BUFSIZ)) == -1) {
                        strerror_r(errno, err_msg, 255);
                        printf("read failed: %s\n", err_msg);
                        break;
                    }
                    if (msg_size == 0) {
                        //what to do if client closed connection and server doesn't ready to get data? hm???
                        if (write_msg(&fds_in[i], &fds_out[i - 1], &pending_messages_to_server[i - 1], client_msg, msg_size, &shutdown) == -1) {
                            if (shutdown) {
                                break;
                            }
                            compress = 1;
                            --nfds_in;
                            --nfds_out;
                            continue;
                        }
                        printf("Connection closed.\n");
                        if (close(fds_in[i].fd) == -1) {
                            strerror_r(errno, err_msg, 255);
                            printf("close failed: %s\n", err_msg);
                            shutdown = 1;
                            break;
                        }
                        if (close(fds_out[i - 1].fd) == -1) {
                            strerror_r(errno, err_msg, 255);
                            printf("close failed: %s\n", err_msg);
                            shutdown = 1;
                            break;
                        }
                        fds_in[i].fd = -1;
                        --nfds_in;

                        fds_out[i - 1].fd = -1;
                        --nfds_out;

                        compress = 1;
                        continue;
                    }
                }
                 //printf("Get msg: %s\n", client_msg);
                 if(fds_out[i-1].revents & POLLOUT) {
                     //printf("writing\n");
                     if (write_msg(&fds_in[i], &fds_out[i - 1], &pending_messages_to_server[i - 1], client_msg, msg_size, &shutdown) == -1) {
                         if (shutdown) {
                             break;
                         }
                         compress = 1;
                         --nfds_in;
                         --nfds_out;
                     }
                 }
                 else{
                     pending_messages_to_server[i-1].data = (char*)realloc(pending_messages_to_server[i].data, (pending_messages_to_server[i].size + msg_size)*sizeof(char));
                     //using realloc may be unsafe
                     strcpy(&pending_messages_to_server[i-1].data[pending_messages_to_server[i-1].size], client_msg);
                     pending_messages_to_server[i-1].size += msg_size;
                 }
            }

        }
        if(shutdown){
            break;
        }
        //handling servers connection
        for(int i = 0; i < nfds_out; ++i) {
            if (fds_out[i].revents == 0 || fds_out[i].fd == -1) {
                continue;
            }

            if (fds_out[i].revents != POLLIN && fds_out[i].revents != POLLOUT && fds_out[i].revents != (POLLIN | POLLOUT)) {
                printf("Error on socket: %d. revents = %d. Closing connection.\n", fds_in[i].fd, fds_in[i].revents);
                if (close(fds_in[i+1].fd) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown = 1;
                    break;
                }
                if (close(fds_out[i].fd) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("close failed: %s\n", err_msg);
                    shutdown = 1;
                    break;
                }
                --nfds_in;

                --nfds_out;

                compress = 1;
            }

            ssize_t msg_size = 0;
            if (fds_out[i].revents & POLLIN) {
                if ((msg_size = read(fds_out[i].fd, client_msg, BUFSIZ)) == -1) {
                    strerror_r(errno, err_msg, 255);
                    printf("read failed: %s\n", err_msg);
                    break;
                }
                if (msg_size == 0) {
                    //what to do if server closed connection and client doesn't ready to get a data? hm???
                    if (write_msg(&fds_out[i], &fds_in[i + 1], &pending_messages_to_client[i], client_msg, msg_size, &shutdown) == -1) {
                        if (shutdown) {
                            break;
                        }
                        compress = 1;
                        --nfds_in;
                        --nfds_out;
                        continue;
                    }
                    printf("Connection closed.\n");
                    if (close(fds_in[i].fd) == -1) {
                        strerror_r(errno, err_msg, 255);
                        printf("close failed: %s\n", err_msg);
                        shutdown = 1;
                        break;
                    }
                    if (close(fds_out[i - 1].fd) == -1) {
                        strerror_r(errno, err_msg, 255);
                        printf("close failed: %s\n", err_msg);
                        shutdown = 1;
                        break;
                    }
                    fds_in[i].fd = -1;
                    --nfds_in;

                    fds_out[i - 1].fd = -1;
                    --nfds_out;

                    compress = 1;
                    continue;
                }
            }
            //printf("Get msg: %s\n", client_msg);
            if (fds_in[i + 1].revents & POLLOUT) {
                if (write_msg(&fds_out[i], &fds_in[i + 1], &pending_messages_to_client[i], client_msg, msg_size,&shutdown) == -1) {
                    if (shutdown) {
                        break;
                    }
                    compress = 1;
                    --nfds_in;
                    --nfds_out;
                }
            } else {
                pending_messages_to_client[i].data = (char *) realloc(pending_messages_to_client[i].data,
                                                                          (pending_messages_to_client[i].size +
                                                                               msg_size) * sizeof(char));
                //using realloc may be unsafe
                strcpy(&pending_messages_to_client[i].data[pending_messages_to_client[i].size], client_msg);
                pending_messages_to_client[i].size += msg_size;
            }
        }
        if(shutdown){
            break;
        }
        nfds_in += new_nfds_in;
        nfds_out += new_nfds_out;
        if(compress){
            int offset = 0;
            for(int i = 1; i < nfds_in; ++i){
                while(fds_in[i + offset].fd == -1){
                    ++offset;
                }
                if(offset){
                    fds_in[i].fd = fds_in[i + offset].fd;
                    fds_out[i - 1] = fds_out[i + offset - 1];
                    pending_messages_to_server[i - 1] = pending_messages_to_server[i + offset - 1];
                    pending_messages_to_client[i - 1] = pending_messages_to_client[i + offset - 1];
                }
            }
            compress = 0;
        }
    }
    if(shutdown){
        for(int i = 1; i < nfds_in; ++i){
            if(close(fds_in[i].fd) == -1){
                strerror_r(errno, err_msg, 255);
                printf("close failed: %s\n", err_msg);
            }
            if(close(fds_out[i-1].fd) == -1){
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
