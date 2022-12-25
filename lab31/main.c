#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/poll.h>
#define closeConn(fd) \
    if(close(fd) == -1){\
    strerror_r(errno, err_msg, 255);\
    printf("close failed. Shutdown. Errno: %s\n", err_msg);\
    shutdown_server = 1;\
    }
/*

int main(int argc, char* argv[]){
    char* address;
    address = argv[1];

    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in remote_address;
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(80);

    struct hostent* remaddrent = gethostbyname(address);

    memcpy(&(remote_address.sin_addr.s_addr), remaddrent->h_addr_list[0], remaddrent->h_length);

    int sfd = connect(client_socket, (struct sockaddr*)&remote_address, sizeof (remote_address));

    char request[] = "GET / HTTP/1.1\r\n\r\n";
    char response[4096];

    send(client_socket, request, sizeof(request), 0);
    recv(client_socket, &response, sizeof (response), 0);

    printf("%s\n", response);
    close(client_socket);
    close(sfd);
    return 0;
}*/
char err_msg[256];
char shutdown_server = 0;
char compress = 0;
char message[BUFSIZ];

int accept_connections(struct pollfd* all_fds, nfds_t* connections_counter){
    int new_client_sfd = accept(all_fds[0].fd, NULL, NULL);
    if(new_client_sfd == -1){
        strerror_r(errno, err_msg,255);
        printf("accept failed: %s\n", err_msg);
        return -1;
    }

    //read below should be changed somehow
    if(read(new_client_sfd, message, BUFSIZ) == -1){
        strerror_r(errno, err_msg, 255);
        printf("read failed. Error: %s\n", err_msg);
        closeConn(new_client_sfd)
        return -1;
    }

    struct hostent* server;
    if((server = gethostbyname(message)) == NULL){
        strerror_r(errno, err_msg, 255);
        printf("Failed to resolve hostname. Error: %s\n", err_msg);
        closeConn(new_client_sfd)
        return -1;
    }

    struct sockaddr_in remote_address;
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(80);
    memcpy(&(remote_address.sin_addr.s_addr), server->h_addr_list[0], server->h_length);

    int new_server_sfd;
    if((new_server_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("creating server socket failed. Errno: %s\n", err_msg);
        closeConn(new_client_sfd)
        return -1;
    }
    if(connect(new_server_sfd, (struct sockaddr*)&remote_address, sizeof (remote_address)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("Connection failed. Error: %s\n", err_msg);
        closeConn(new_client_sfd)
        closeConn(new_server_sfd)
        return -1;
    }
    all_fds[*connections_counter].fd = new_client_sfd;
    all_fds[*connections_counter].events = POLLIN | POLLOUT;

    all_fds[*connections_counter + 1].fd = new_server_sfd;
    all_fds[*connections_counter + 1].fd = POLLIN | POLLOUT;

    *connections_counter += 2;

    return 0;
}

//offset 1 for client connections, -1 for server connections
int handle_connections(struct pollfd* fds, nfds_t* connections, const int offset){
    for(nfds_t i = 0; i < *connections; i += 2){
        if(fds[i].revents != POLLIN && fds[i].revents != POLLOUT && fds[i].revents != (POLLIN | POLLOUT)){
            printf("Error on socket: %d. revents = %d. Closing connections.\n", fds[i].fd, fds[i].revents);
            closeConn(fds[i].fd)
            closeConn(fds[i + offset].fd)
            if(shutdown_server){
                return -1;
            }
            *connections -= 2;
            compress = 1;
        }

        ssize_t msg_size = 0;
        if(fds[i].revents & POLLIN){
            if((msg_size = read(fds[i].fd, message, BUFSIZ) == -1)){
                strerror_r(errno, err_msg, 255);
                printf("read failed. Error: %s\n", err_msg);
            }
            if (msg_size == 0){
                closeConn(fds[i].fd)
                closeConn(fds[i + offset].fd)
                if(shutdown_server){
                    return -1;
                }
                fds[i].fd = -1;
                fds[i + offset].fd = -1;
                *connections -= 2;
                compress = 1;
                continue;
            }
            //
            if(fds[i+offset].revents & POLLOUT){
                if(write(fds[i + offset].fd, message, msg_size) == -1){
                    strerror_r(errno, err_msg, 255);
                    printf("Write failed. Error: %s\n", err_msg);
                    closeConn(fds[i].fd)
                    closeConn(fds[i + offset].fd)
                    if(shutdown_server){
                        return -1;
                    }
                    *connections -= 2;
                    compress = 1;
                    continue;
                }
            }
        }
    }
    return 0;
}
int main(int argc, char* argv[]){
    if(argc !=  2){
        printf("wrong argument qty\n");
        exit(EXIT_FAILURE);
    }
    int listen_sfd, listen_port;
    listen_port = atoi(argv[1]);

    if((listen_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("listen socket was not created. Error: %s\n", err_msg);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in* proxy_address;
    proxy_address->sin_family = AF_INET;
    proxy_address->sin_addr.s_addr = INADDR_ANY;
    proxy_address->sin_port = htons(listen_port);

    if(bind(listen_sfd, (struct sockaddr*)proxy_address, sizeof (proxy_address)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("Binding listening socket failed. Error: %s\n", err_msg);
        close(listen_sfd);
        exit(EXIT_FAILURE);
    }

    nfds_t connections_counter = 1;
    nfds_t all_fds_size = 1025;
    struct pollfd* all_fds = (struct pollfd*) malloc(1025 * sizeof (struct pollfd));

    all_fds[0].fd = listen_sfd;
    all_fds[0].events = POLLIN;
    int timeout = 60*60*1000;

    for(;;){
        poll(all_fds, connections_counter, timeout);

        //handle client connections;
        handle_connections(&all_fds[1], &connections_counter, 1);
        if(shutdown_server){
            break;
        }
        //handle server connections;
        handle_connections(&all_fds[2], &connections_counter, -1);
        if(shutdown_server){
            break;
        }

        if(all_fds[0].revents & POLLIN){
            accept_connections(all_fds, &connections_counter);
            if(shutdown_server){
                break;
            }
        }
        else if(all_fds[0].revents != 0){
            shutdown_server = 1;
            break;
        }

        if(compress){
            int offset = 0;
            for(int i = 1; i < connections_counter; i += 2){
                while (all_fds[i + offset].fd == -1){
                    offset += 2;
                }
                all_fds[i].fd = all_fds[i + offset].fd;
                all_fds[i + 1].fd = all_fds[i + offset + 1].fd;
            }
            compress = 0;
        }
        if(connections_counter == all_fds_size){
            struct pollfd* new_all_fds_arr = malloc(all_fds_size * 2 * sizeof (struct pollfd));
            if(new_all_fds_arr == NULL){
                strerror_r(errno, err_msg, 255);
                printf("allocating space for new connections failed. Error: %s\n", err_msg);
                //
            }
            else{
                memcpy(new_all_fds_arr, all_fds, connections_counter * sizeof (struct pollfd));
                free(all_fds);
                all_fds = new_all_fds_arr;
            }
        }
    }

}