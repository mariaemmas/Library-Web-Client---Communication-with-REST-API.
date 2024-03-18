#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "requests.h"
#include "parson.h"

int BUFSIZE = 50;                // buffer read
int MAX_COMMAND = 50;            // max length command
int MAX_USERNAME = 150;       
int MAX_PASSWORD = 150;
int BUF_BODY_DATA = 500;
// int MAX_DATA_NO = 20;              // max number of data fields
int MAX_COOKIES_NO = 20;             // max number of cookies
int MAX_TOKEN_LENGTH = 4096;
int MAX_BOOK_TITLE = 150;
int MAX_BOOK_GENRE = 150;
int MAX_BOOK_AUTHOR = 150;
int MAX_BOOK_PUBLISHER = 150;

struct user {
    char* username;
    char* password;
};

struct book {
    char* title;
    char* author;
    char* genre;
    int page_count;
    char* publisher;
};

int read_command(char* read_data, int data_length) {
    printf("Command=");
    while(strlen(read_data) <=  1) {
        memset(read_data, '\0', data_length);
        fgets(read_data, BUFSIZE, stdin);
    }
    return 0;
}

int read_user(struct user* user) {
    user->username = malloc(MAX_USERNAME);
    if (user->username == NULL) {
        perror("Memory not allocated for username\n");
        return -1;
    }
    user->password = malloc(MAX_PASSWORD);
    if (user->username == NULL) {
        perror("Memory not allocated for password\n");
        return -1;
    }
    printf("username=");
    memset(user->username, '\0', MAX_USERNAME);
    fgets(user->username, MAX_USERNAME, stdin);
    printf("password=");
    memset(user->password, '\0', MAX_PASSWORD);
    fgets(user->password, MAX_PASSWORD, stdin);
    if ((strchr(user->username,' ') != NULL) || (strchr(user->password,' ') != NULL)) {
        printf("\nSpaces inside username or password are not allowed!\n\n");
        return -1; 
    }

    user->username[strlen(user->username)-1]='\0';
    user->password[strlen(user->password)-1]='\0';
    return 0;
}

void free_book(struct book* book) {       
    free(book->author);
    free(book->genre);
    free(book->publisher);
    free(book->title);
    book->author = NULL;
    book->genre = NULL;
    book->publisher = NULL;
    book->title = NULL;
    book->page_count = 0;
}

int read_book(struct book* book) {
    book->title = malloc(MAX_BOOK_TITLE);
    if (book->title == NULL) {
        perror("Memory not allocated for book title\n");
        return -1;
    }
    book->author = malloc(MAX_BOOK_AUTHOR);
    if (book->author == NULL) {
        perror("Memory not allocated for book author\n");
        return -1;
    }
    book->genre = malloc(MAX_BOOK_GENRE);
    if (book->genre == NULL) {
        perror("Memory not allocated for book genre\n");
        return -1;
    }
    book->publisher = malloc(MAX_BOOK_PUBLISHER);
    if (book->publisher == NULL) {
        perror("Memory not allocated for book publisher\n");
        return -1;
    }
    printf("title=");
    memset(book->title, '\0', MAX_BOOK_TITLE);
    fgets(book->title, MAX_BOOK_TITLE, stdin);
    printf("author=");
    memset(book->author, '\0', MAX_BOOK_AUTHOR);
    fgets(book->author, MAX_BOOK_AUTHOR, stdin);
    printf("genre=");
    memset(book->genre, '\0', MAX_BOOK_GENRE);
    fgets(book->genre, MAX_BOOK_GENRE, stdin);

    printf("publisher=");
    memset(book->publisher, '\0', MAX_BOOK_PUBLISHER);
    fgets(book->publisher, MAX_BOOK_PUBLISHER, stdin);

    book->page_count = 0;
    printf("page_count=");
    scanf("%d", &book->page_count);
    fflush(stdin);

    if (book->page_count <= 0) {
        printf("\nIncorrect data. Next time please set page_count to a numeric value greater than 0\n\n");
        free_book(book);
        return -1;
    }

    book->title[strlen(book->title)-1]='\0';
    book->author[strlen(book->author)-1]='\0';
    book->genre[strlen(book->genre)-1]='\0';
    book->publisher[strlen(book->publisher)-1]='\0';

    return 0;  
}

void free_cookies(char* cookies[], int cookies_no){
    for (int i = 0; i < cookies_no; i++) {
        if (cookies[i] != NULL) {
            free(cookies[i]);
            cookies[i] = NULL;
        }
    }
};

void free_user(struct user* user) {       
    free(user->username);
    free(user->password);
    user->username = NULL;
    user->password = NULL;
}


void write_error_msg(char* errmessage) {
    JSON_Value *json_val = NULL;
    json_val = json_parse_string(errmessage);
    JSON_Object* json_obj = json_object(json_val);

    int cnt_obj = json_object_get_count(json_obj);
    for (int i = 0; i < cnt_obj; i++) {
        const char* jname = json_object_get_name(json_obj, i);
        JSON_Value* jval = json_object_get_value_at(json_obj, i);            
        int jjj = json_value_get_type(jval);
        if (jjj == 2) {
            printf("%s: %s\n", jname,json_object_get_string(json_obj, jname));
        }
        if (jjj == 3) {
            printf("%s: %d\n", jname,(int)json_object_get_number(json_obj, jname));
        }
    }
}

void get_token(char* message, char* token) {
    if (message == NULL) {
        printf("Token is missing\n"); 
    }
    else {
        JSON_Value *json_val = NULL;
        json_val = json_parse_string(message);
        JSON_Object* json_obj = json_object(json_val);
        int tokenlen = json_object_get_string_len(json_obj, "token"); 
        memset(token, 0, MAX_TOKEN_LENGTH);
        strncpy(token, json_object_get_string(json_obj, "token") , tokenlen); 
        json_value_free(json_val);
    }
}

int register_user(struct user* user, char* host, int port, char* hostp, char* register_access_route, char* payload_type, char* token) {

    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char* message;
    char* response; 

    JSON_Value *json_user = json_value_init_object();
    JSON_Object *json_ouser = json_value_get_object(json_user);
    json_object_set_string(json_ouser, "username", user->username);
    json_object_set_string(json_ouser, "password", user->password);

    char* body_data = json_serialize_to_string(json_user);

    message = compute_post_request(hostp, register_access_route, payload_type, body_data,0, NULL, token);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        printf("User was registered");
    }
    else {
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    free(body_data);
    return xreturn;
}

int login_user(struct user* user, char* host, int port, char* hostp, char* login_access_route, char* payload_type, char* cookies[], int* cookies_no, char* token) {

    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char* message;
    char* response; 

    JSON_Value *json_user = json_value_init_object();
    JSON_Object *json_ouser = json_value_get_object(json_user);
    json_object_set_string(json_ouser, "username", user->username);
    json_object_set_string(json_ouser, "password", user->password);

    char* body_data = json_serialize_to_string(json_user);

    message = compute_post_request(hostp, login_access_route, payload_type, body_data,0, NULL, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n%s\n", oneword); 
    oneword=strtok(oneword," ");

    free_cookies(cookies, *cookies_no);
    *cookies_no = 0;
    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        // success, read cookies
        printf("User was successfully logged in\n");
        char* start_cookie1=strstr(restrows,"Set-Cookie");
        char* start_cookie = start_cookie1;
        while (start_cookie != NULL) {
            char* end_cookie=strchr(start_cookie,';');
            int length_cookie = end_cookie - start_cookie - 12 + 1;
            cookies[*cookies_no] = malloc(length_cookie * sizeof(char));
            if (cookies[*cookies_no] == NULL) {
                perror("Memory not allocated for cookies\n");
                return -1;
            }
            strncpy(cookies[*cookies_no], start_cookie + 12, length_cookie); 
            *cookies_no += 1;
            start_cookie1 = start_cookie1 + 1;
            start_cookie=strstr(start_cookie1,"Set-Cookie");
            if (*cookies_no >= MAX_COOKIES_NO) {
                break;
            }
        }
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(body_data);
    free(message);
    free(response);
    return xreturn;
}

int enter_library(char* host, int port, char* hostp, char* enter_library_access_route, char* cookies[], int* cookies_no, char* token) {
    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char *message;
    char *response;
    char* query_params = NULL;
    message = compute_get_request(hostp, enter_library_access_route, query_params,
        cookies, *cookies_no, token);

    /*trimit la server mesajul*/
    send_to_server(sockfd, message);    
    response = receive_from_server(sockfd); // primesc raspuns de la server 

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        // success, read token
        get_token(strstr(restrows,"{\""), token);
        printf("Library opened\n");
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}

int get_books(char* host, int port, char* hostp, char* book_access_route, char* cookies[], int* cookies_no, char* token) {
    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char *message;
    char *response;
    char* query_params = NULL;
    message = compute_get_request(hostp, book_access_route, query_params,
        cookies, *cookies_no, token);

    send_to_server(sockfd, message);    
    response = receive_from_server(sockfd); // primesc raspuns de la server 

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        // success, read token
        char* jmessage = strstr(restrows,"[{");
        JSON_Value *json_book = json_value_init_object();
        json_book = json_parse_string(jmessage);

        JSON_Array* json_abook = json_value_get_array(json_book);
        int books_no = json_array_get_count(json_abook);
        printf("Books in library :\n");
        // for all books
        for (int i = 0; i < books_no; i++) {
            JSON_Object* json_obj = json_array_get_object(json_abook, i);
            // for one book
            int cnt_obj = json_object_get_count(json_obj);
            for (int j = 0; j < cnt_obj; j++) {
                const char* jname = json_object_get_name(json_obj, j);
                JSON_Value* jval = json_object_get_value_at(json_obj, j);            
                int jjj = json_value_get_type(jval);
                if (jjj == 2) {
                    printf("%s: %s   ", jname,json_object_get_string(json_obj, jname));
                }
                if (jjj == 3) {
                    printf("%s: %d    ", jname,(int)json_object_get_number(json_obj, jname));
                }
            }
            printf("\n");
        }
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}



int get_book(char* host, int port, char* hostp, char* book_access_route, char* cookies[], int* cookies_no, char* token, int book_id) {
    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char *message;
    char *response;
    char* query_params = NULL;
    // add book id to url
    char* book_access_route_new = malloc((strlen(book_access_route)+10) * sizeof(char) );
    if (book_access_route_new == NULL) {
        perror("Memory not allocated for book_access_route_new\n");
        return -1;
    }
    sprintf(book_access_route_new,"%s/%d",book_access_route,book_id);
    message = compute_get_request(hostp, book_access_route_new, query_params,
        cookies, *cookies_no, token);

    send_to_server(sockfd, message);    
    response = receive_from_server(sockfd); // primesc raspuns de la server 

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n");
    printf("%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        // parse book info (without hardcodes!!!)
        char* jmessage=strstr(restrows,"{\"");
        JSON_Value *json_val = NULL;
        json_val = json_parse_string(jmessage);
        JSON_Object* json_obj = json_object(json_val);
        int cnt_obj = json_object_get_count(json_obj);
        for (int i = 0; i < cnt_obj; i++) {
            const char* jname = json_object_get_name(json_obj, i);
            JSON_Value* jval = json_object_get_value_at(json_obj, i);            
            int jjj = json_value_get_type(jval);
            if (jjj == 2) {
                printf("%s: %s\n", jname,json_object_get_string(json_obj, jname));
            }
            if (jjj == 3) {
                printf("%s: %d\n", jname,(int)json_object_get_number(json_obj, jname));
            }
        }
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}

int delete_book(char* host, int port, char* hostp, char* book_access_route, char* cookies[], int* cookies_no, char* token, int book_id) {
    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }
    int xreturn = 0;
    char *message;
    char *response;
    char* book_access_route_new = malloc((strlen(book_access_route)+10) * sizeof(char) );
    if (book_access_route_new == NULL) {
        perror("Memory not allocated for book_access_route_new\n");
        return -1;
    }
    sprintf(book_access_route_new,"%s/%d",book_access_route,book_id);
    message = compute_delete_request(hostp, book_access_route_new, token);

    send_to_server(sockfd, message);    
    response = receive_from_server(sockfd); // receive response from server

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n");
    printf("%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        printf("Book was deleted\n");
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}

int add_book(char* host, int port, char* hostp,  char* login_access_route, 
        char* payload_type, char* cookies[], int* cookies_no, char* token) {
    struct book book;
    if (read_book(&book) < 0) {
        free_book(&book);
        return -1;
    }
    JSON_Value *json_book = json_value_init_object();
    JSON_Object *json_obook = json_value_get_object(json_book);
    json_object_set_string(json_obook, "title", book.title);
    json_object_set_string(json_obook, "author", book.author);
    json_object_set_string(json_obook, "genre", book.genre);
    json_object_set_number(json_obook, "page_count", book.page_count);
    json_object_set_string(json_obook, "publisher", book.publisher);

//    json_serialize_to_file_pretty(json_book, "aa.txt");
    char* body_data = json_serialize_to_string(json_book);

    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char* message;
    char* response; 

    message = compute_post_request(hostp, login_access_route, payload_type, body_data,0, NULL, token);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        // success, book added
        printf("Book was successfully added\n");
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}

int logout_user(char* host, int port, char* hostp, char* logout_access_route, char* cookies[], int* cookies_no, char* token) {
    // open socket for TCP connection (and connect)
    int sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error while creating TCP socket for client\n");
        return -1;
    }

    int xreturn = 0;
    char *message;
    char *response;
    char* query_params = NULL;
    message = compute_get_request(hostp, logout_access_route, query_params,
        cookies, *cookies_no, token);

    /*trimit la server mesajul*/
    send_to_server(sockfd, message);    
    response = receive_from_server(sockfd); // primesc raspuns de la server 

    char* firstrow = strtok(response,"\n");
    char* restrows=strtok(NULL,"\0");

    char* oneword=strtok(firstrow," ");
    oneword=strtok(NULL,"\0");
    printf("\n");
    printf("%s\n", oneword); 
    oneword=strtok(oneword," ");

    if ((strcmp(oneword, "200") == 0) || (strcmp(oneword, "201") == 0)) {
        printf("client was successfully logged out\n");
        // success
    }
    else {
        // error, print error message
        write_error_msg(strstr(restrows,"{\""));
        xreturn = -1;
    }
    printf("\n");
    close_connection(sockfd);
    free(message);
    free(response);
    return xreturn;
}

int main(int argc, char *argv[])
{
    // char *response;
    // int sockfd;

    char* cookies[MAX_COOKIES_NO];
    int cookies_no = 0;
    char* token = malloc(MAX_TOKEN_LENGTH * sizeof(char));
    if (token == NULL) {
        perror("Memory not allocated for token\n");
        return -1;
    }
    char *body_data = malloc(BUF_BODY_DATA);
    if (body_data == NULL) {
        perror("Memory not allocated for body_data\n");
        return -1;
    }

    char register_command[] = "register";
    char login_command[] = "login";
    char enter_library_command[] = "enter_library";
    char get_books_command[] = "get_books";
    char get_book_command[] = "get_book";
    char add_book_command[] = "add_book";
    char delete_book_command[] = "delete_book";
    char logout_command[] = "logout";
    char exit_command[] = "exit";

    char host[] = "34.254.242.81";
    char hostp[] = "34.254.242.81:8080";
    int port = 8080;

    char register_access_route[] = "/api/v1/tema/auth/register";
    char login_access_route[] = "/api/v1/tema/auth/login";
    char enter_library_access_route[] = "/api/v1/tema/library/access";
    char book_access_route[] = "/api/v1/tema/library/books";
    char logout_access_route[] = "/api/v1/tema/auth/logout";

    char payload_type[] = "application/json";

    struct user user, registered_user;  // user = current user, registered_user = to register an user
    // user -> filled in by login, freed by logout; checked by all tasks except register and exit

    char* command = malloc(MAX_COMMAND);
    if (command == NULL) {
        perror("Memory not allocated for command\n");
        return 1;
    }

    memset(token, 0, MAX_TOKEN_LENGTH);

    while(1) {
//        system("clear");
        memset(command,0,MAX_COMMAND);
        read_command(command, MAX_COMMAND);
        if (strlen(command) == 0) {
            continue;
        }
        if (strncmp(command,register_command,strlen(register_command)-1) == 0) {
            // register
            if (read_user(&registered_user) < 0) {
                free_user(&registered_user);
                continue;
            }
            if ((strlen(registered_user.username) <= 1) || (strlen(registered_user.password) <= 1)) {
                printf("\nIncorrect username or password\n\n");
                free_user(&registered_user);
                continue;
            }
            register_user(&registered_user, host, port, hostp, register_access_route, payload_type, token);
            free_user(&registered_user);
        }
        if (strncmp(command,login_command,strlen(login_command)-1) == 0) {
            // login
            if (user.username != NULL) {
                printf("\nAlready logged in, user %s password %s\n\n", user.username, user.password);
                continue;
            }
            if (read_user(&user) < 0) {     // get user name and password
                free_user(&user);
                continue;
            }
            if ((strlen(user.username) <= 1) || (strlen(user.password) <= 1)) {
                printf("\nIncorrect username or password\n\n");
                free_user(&user);
                continue;
            }
            if (login_user(&user, host, port, hostp, login_access_route, payload_type, cookies, &cookies_no, token) < 0) {
                free_user(&user);
            }
        }
        if (strncmp(command,enter_library_command,strlen(enter_library_command)-1) == 0) {
            // enter_library
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            // if (strlen(token) != 0) {
            //     printf("Already entered in library\n");
            //     continue;
            // }
            if (enter_library(host, port, hostp, enter_library_access_route, cookies, &cookies_no, token) < 0) {
                // ...
            }
        }
        if (strncmp(command, get_books_command,strlen(get_books_command)) == 0) {
            // get books
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            if (strlen(token) == 0) {
                printf("\nNot entered in library\n\n");
                continue;
            }
            if (get_books(host, port, hostp, book_access_route, cookies, &cookies_no, token) < 0) {
                // ...
            }
            continue;
        }
        if (strncmp(command, get_book_command,strlen(get_book_command)) == 0) {
            // get book
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            if (strlen(token) == 0) {
                printf("\nNot entered in library\n\n");
                continue;
            }
            int book_id = 0;
            printf("id=");
            scanf("%d", &book_id);
            fflush(stdin);
            if (book_id > 0) {
                if (get_book(host, port, hostp, book_access_route, cookies, &cookies_no, token, book_id) < 0) {
                    // ...
                }
            }
        }
        if (strncmp(command, add_book_command,strlen(add_book_command)-1) == 0) {
            // add book
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            if (strlen(token) == 0) {
                printf("\nNot entered in library\n\n");
                continue;
            }
            if (add_book(host, port, hostp, book_access_route, payload_type, cookies, &cookies_no, token) < 0) {
                // ...
            }
        }

        if (strncmp(command, delete_book_command,strlen(delete_book_command)) == 0) {
            // delete book
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            if (strlen(token) == 0) {
                printf("\nNot entered in library\n\n");
                continue;
            }
            int book_id = 0;
            printf("id=");
            scanf("%d", &book_id);
            fflush(stdin);
            if (book_id > 0) {
                if (delete_book(host, port, hostp, book_access_route, cookies, &cookies_no, token, book_id) < 0) {
                    // ...
                }
            }
        }

        if (strncmp(command,logout_command,strlen(logout_command)-1) == 0) {
            // logout
            if (user.username == NULL) {
                printf("\nNot logged in !\n\n");
                continue;
            }
            if (logout_user(host, port, hostp, logout_access_route, cookies, &cookies_no, token) >= 0) {
                memset(token, 0, MAX_TOKEN_LENGTH);
                free_user(&user);
            }
        }
        if (strncmp(command,exit_command,strlen(exit_command)-1) == 0) {
            // exit
            if (user.username != NULL) {
                printf("\nPlease logout, user %s password %s\n\n", user.username, user.password);
                // exit_no += 1;
                // if (exit_no > 1) {
                //     break;
                // }
            }
            else {
                break;
            }
        }
    }
    free(token);
    free(body_data);
    free_cookies(cookies, cookies_no);
    free(command);
    if (user.username != NULL) {
        free_user(&user);
    }
    return 0;
}
