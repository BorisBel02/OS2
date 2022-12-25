//
// Created by boris on 25/12/22.
//


#include "read_request.h"


struct httpRequestProperties{
    char* url;
    char* host;
    char* port;
    char* method;
    char* version;
}httpRequestProperties;
void freeHttpRequestProperties(struct httpRequestProperties* properties){

    if(properties->url){
        free(properties->url);
    }
    if(properties->host){
        free(properties->host);
    }
    if(properties->port){
        free(properties->port);
    }
    if(properties->version){
        free(properties->version);
    }
    free(properties);
}
struct httpRequestProperties* parseHttp(char* request, size_t request_size){
    //printf("REQUEST:\n%s", request);
    char* buf;
    char* end_of_headers = strstr(request, "\r\n\r\n");
    if(end_of_headers == NULL){
        return NULL;
    }
    buf = (char*)malloc(sizeof(char) * (end_of_headers - request + 1));
    memcpy(buf, request, (end_of_headers - request) * sizeof(char));
    buf[end_of_headers - request + 1] = '\0';


    //Getting one line with \r\n delimiter
    char* line, * saveptr1, * strbuf;
    strbuf = buf;
    line = strtok_r(strbuf, "\r\n", &strbuf);
    if(line == NULL){
        printf("bad request\n");
        free(buf);
        return NULL;
    }
    //parsing first line
    char* token, *saveptr2, *linebuf;
    linebuf = line;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        printf("bad request\n");
        free(buf);
        return NULL;
    }
    if((strcmp(token, "GET")) != 0){
        printf("Only GET allowed\n");
        free(buf);
        return NULL;
    }
    linebuf = NULL;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        printf("No URL found.\n");
        free(buf);
        return NULL;
    }
    struct httpRequestProperties* props = (struct httpRequestProperties*)malloc(sizeof(struct httpRequestProperties));
    props->url = NULL;
    props->host = NULL;
    props->port = NULL;
    props->method = NULL;
    props->version = NULL;

    props->url = (char*) malloc(sizeof(char) * (strlen(token) + 1));
    memcpy(props->url, token, (strlen(token)) * sizeof(char));
    props->url[strlen(token) + 1] = '\0';

    linebuf = NULL;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        free(props->url);
        free(props);
        free(buf);
        printf("Version not specified.\n");
        return NULL;
    }
    /*if(token[5] != '1'){
        free(props->url);
        free(props);
        free(buf);
        printf("version: %s is not supported\n", token);
        return NULL NULL;
    }*/
    if(strbuf == NULL){
        printf("parsing failed\n");
        free(props->url);
        free(props);
        free(buf);
        return NULL;
    }
    props->version = malloc(sizeof(char) * (strlen(token - 4)));
    strncpy(props->version, &token[5], strlen(token) - 5);


    char host_found = 0;

    for(;host_found == 0;strbuf = NULL){
        line = strtok_r(strbuf, "\r\n", &saveptr1);
        if(line == NULL){
            break;
        }
        linebuf = line;
        token = strtok_r(linebuf, ": ", &saveptr2);
        if(strcmp(token, "Host") == 0){
            host_found = 1;
            linebuf = NULL;
            break;
        }
        /*for(linebuf = line; host_found == 0; linebuf = NULL){
            token = strtok_r(linebuf, " ", &saveptr2);
            if(strcmp(token, "Host:") == 0){
                host_found = 1;
                linebuf = NULL;
                break;
            }
            break;
        }*/
    }
    if(host_found == 0){
        free(props->url);
        free(props);
        free(buf);
        printf("Host is not specified.\n");
        return NULL;
    }
    token = strtok_r(linebuf, ": ", &saveptr2);
    if(token == NULL){
        free(props->url);
        free(props);
        free(buf);
        printf("Host not specified.\n");
        return NULL;
    }
    props->host = (char*)malloc(sizeof(char) * (strlen(token) + 1));
    memcpy(props->host, token, sizeof(char) * (strlen(token)));
    props->host[strlen(token) + 1] = '\0';

    linebuf = NULL;
    token = strtok_r(linebuf, ": ", &saveptr2);
    if(token){
        props->port = (char*)malloc(sizeof(char) * (strlen(token) + 1));
        memcpy(props->port, token, sizeof(char) * strlen(token));
        props->port[strlen(token) + 1] = '\0';
    }
    else{
        props->port = (char*)malloc(sizeof(char) * (3));
        strcpy(props->port, "80");
        props->port[2] = '\0';
    }
    free(buf);
    return props;
}

void* read_request(void* args){
    char err_msg[255];
    int err_code;
    char msg[BUFSIZ] = { 0 };

    struct task* current_task = (struct task*)args;

    struct workThreadArguments* arguments = current_task->arguments;
    printf("handling REQUEST\n");
    ssize_t msg_size = 0;
    if((msg_size = read(current_task->client_sfd, msg, BUFSIZ)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("read failes %s on desc: %d\n", err_msg, current_task->client_sfd);
        close(current_task->client_sfd);
        free(current_task);
        return NULL;
    }
    if(msg_size == 0){
        close(current_task->client_sfd);
        free(current_task);
        return NULL;
    }
    printf("parsing\n");
    struct httpRequestProperties* properties = parseHttp(msg, msg_size);
    if(properties == NULL){
        close(current_task->client_sfd);
        free(current_task);
        return NULL;
    }
    //cache checking
    struct HashTableCell* response;
    printf("cachekey\n");
    char* cachekey = (char*)malloc(sizeof(char) * (strlen(properties->url) + strlen(properties->host) + 1));
    if(cachekey){
        strcpy(cachekey, properties->host);
        strcpy(&cachekey[strlen(properties->host)], properties->url);
        printf("key created\n");
        cachekey[strlen(properties->url) + strlen(properties->host) + 1] = '\0';
        //printf("host: %s\nurl:%s\n", properties->host, properties->url);
        //printf("key: %s\n", cachekey);
        response = getCachedResponse(cachekey, arguments->cache);
        printf("got cache\n");
        if(response){   //maybe create WRITE_RESPONSE task
            printf("CACHE HIT! Writing response from cache\n");
            if(write(current_task->client_sfd, response->response_string, response->response_size) < 0){
                printf("write to client failed\n");
            }
            printf("free stuff\n");
            freeHttpRequestProperties(properties);
            free(cachekey);
            free(current_task);
            return NULL;
        }
    }
    else{
        printf("malloc failed\n");
        freeHttpRequestProperties(properties);
        close(current_task->client_sfd);
        free(current_task);
        return NULL;
    }
    int server_sfd;
    struct addrinfo hints;
    struct addrinfo* result, * rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    printf("getting addr info for host %s\n", properties->host);
    err_code = getaddrinfo(properties->host, properties->port, &hints, &result);
    if(err_code != 0){
        printf("getaddrinfo failed:\n%s\n", gai_strerror(err_code));
        close(current_task->client_sfd);
        freeHttpRequestProperties(properties);
        free(current_task);
        return NULL;
    }
    printf("CONNECTING...\n");
    char ip[50];
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        server_sfd = socket(rp->ai_family, rp->ai_socktype,
                            rp->ai_protocol);
        if (server_sfd == -1)
            return NULL;
        inet_ntop(AF_INET, &rp->ai_addr->sa_data[2], ip, 50);
        printf("connecting to ip: %s\n", ip);
        if (connect(server_sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(server_sfd);
    }

    if(rp == NULL){
        strerror_r(errno, err_msg, 255);
        printf("connection to host %s failed:\n%s\n", properties->host ,err_msg);
        freeHttpRequestProperties(properties);
        close(current_task->client_sfd);
        free(current_task);
        freeaddrinfo(result);
        return NULL;
    }
    freeaddrinfo(result);

    if(server_sfd < *arguments->max_connections_qty){
        current_task->server_sfd = server_sfd;
        current_task->type = WRITE_REQUEST;
        current_task->string = malloc(msg_size * sizeof(char));
        memcpy(current_task->string, msg, msg_size * sizeof(char));
        current_task->string_size = msg_size;
        current_task->cache_key = cachekey;
        pendingTaskAppend(arguments->tl, current_task, server_sfd);

        pthread_mutex_lock(&arguments->rwlock);
        arguments->connections[*arguments->connections_qty].fd = server_sfd;
        arguments->connections[*arguments->connections_qty].events = POLLOUT;
        ++*arguments->connections_qty;
        pthread_mutex_unlock(&arguments->rwlock);

        printf("new server conn: %d\n", server_sfd);
    }
    else{
        printf("socket num greater than array size\n");
        close(current_task->client_sfd);
        close(server_sfd);
        free(current_task);
    }
    freeHttpRequestProperties(properties);
}