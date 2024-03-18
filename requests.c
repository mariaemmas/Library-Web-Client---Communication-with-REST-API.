#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char* cookies[], int cookies_no, char* token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    if (message == NULL) {
        printf("Memory not allocated for message\n");
        exit(1);
    }
    char *line = calloc(LINELEN, sizeof(char));
    if (line == NULL) {
        printf("Memory not allocated for line\n");
        exit(1);
    }

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies_no > 0) {
        strcat(message, "Cookie: ");
        for (int i = 0; i < cookies_no; i++) {
            sprintf(line, "%s", cookies[i]);
            compute_message(message, line);
        }
    }
    // Step 4: add final new line
    if (strlen(token) > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    compute_message(message, "\r\n");

    free(line);
    return message;
}


char *compute_delete_request(char *host, char *url, char* token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    if (message == NULL) {
        printf("Memory not allocated for message\n");
        exit(1);
    }
    char *line = calloc(LINELEN, sizeof(char));
    if (line == NULL) {
        printf("Memory not allocated for line\n");
        exit(1);
    }

    // Step 1: write delete line
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: add token
    if (strlen(token) > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    compute_message(message, "\r\n");
    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *body_data,
                             int cookies_no, char* cookies[], char* token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    if (message == NULL) {
        printf("Memory not allocated for message\n");
        exit(1);
    }
    char *line = calloc(LINELEN, sizeof(char));
    if (line == NULL) {
        printf("Memory not allocated for line\n");
        exit(1);
    }
    char *body_data_buffer = calloc(LINELEN, sizeof(char));
    if (body_data_buffer == NULL) {
        printf("Memory not allocated for body_data_buffer\n");
        exit(1);
    }

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size

    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    int len = strlen(body_data); // si ii calculam lungimea 
    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);

    if (strlen(token) > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    // Step 4 (optional): add cookies
    if (cookies_no > 0) {
        strcat(message, "Cookie: ");
        for (int i = 0; i < cookies_no; i++) {
            sprintf(line, "%s", cookies[i]);
            strcat(message, line);
        }
    }
    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    strcpy(body_data_buffer, body_data);

    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}

