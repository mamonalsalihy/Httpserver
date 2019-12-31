#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULTPORT 80
#define handle_error(err, fd) \
    do                        \
    {                         \
        close(fd);            \
        perror(err);          \
        exit(EXIT_FAILURE);   \
    } while (0)

#define LISTEN_BACKLOG 50
#define BUFF_SIZE 32000
#define NEWLINE "\r\n\r\n"

int getport(char *, int, int);
int validatefile(char *);
int filesize(int);
struct in_addr gethostname(char *);
void responseGET(char *, size_t, int);
void responsePUT(char *, size_t, int);
void handlefile(char *, char *, size_t, int, int);

//read and process multiple files from header
//account for empty files

int main(int argc, char *argv[])
{
    if (argc == 1 || argc > 3)
    {
        perror("Invalid argument number");
        exit(EXIT_FAILURE);
    }
    //opening connection
    struct sockaddr_in address;
    int sockfd, currsock;
    long readfd;
    int address_len = sizeof(address);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error("socket failure", sockfd);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(inet_ntoa(gethostname(argv[1])));
    address.sin_port = htons(getport(argv[2], argc, sockfd));
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));

    if (bind(sockfd, (struct sockaddr *)&address, address_len) == -1)
        handle_error("bind failure", sockfd);
    if (listen(sockfd, LISTEN_BACKLOG) == -1)
        handle_error("listen failure", sockfd);

    while (1)
    {
        if ((currsock = accept(sockfd, (struct sockaddr *)&address, (socklen_t *)&address_len)) == -1)
            handle_error("accept failure", currsock);

        char headerbuf[BUFF_SIZE] = {0};
        size_t byte = read(currsock, headerbuf, sizeof(headerbuf));
        char *header;
        char *requestidentifier;
        const char *delimiter = " ";
        header = strtok(headerbuf, delimiter);
        requestidentifier = header;

        header = strtok(NULL, delimiter);
        //determine which request
        if (strncmp(requestidentifier, "GET\0", 4) == 0)
        {
            //validates filename
            if (validatefile(header) == 0)
            {
                write(currsock, "HTTP/1.1 400 Bad Request\r\n\r\n", 28);
            }
            else
            {
                responseGET(header, byte, currsock);
            }
        }
        else if (strncmp(requestidentifier, "PUT\0", 4) == 0)
        {
            if (validatefile(header) == 0)
            {
                write(currsock, "HTTP/1.1 400 Bad Request\r\n\r\n", 28);
            }
            else
            {
                responsePUT(header, byte, currsock);
            }
        }
        else
        {
            write(currsock, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
        }
        close(currsock);
    }
    close(sockfd);
    return (0);
}
struct in_addr gethostname(char *hostname)
{
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(hostname)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    else
    {
        addr_list = (struct in_addr **)he->h_addr_list;
        return *addr_list[0];
    }
}
int getport(char *argv, int argc, int sockfd)
{
    if (argv != NULL && atoi(argv) == 0)
    {
        handle_error("Invalid input", sockfd);
    }
    return argc == 2 ? DEFAULTPORT : atoi(argv);
}

int validatefile(char *filename)
{
    if (strlen(filename) == 27)
    {
        for (int i = 0; filename[i] != '\0'; i++)
        {
            if ((filename[i] >= '0' && filename[i] <= '9') || (filename[i] >= 'A' && filename[i] <= 'Z') || (filename[i] >= 'a' && filename[i] <= 'z') || filename[i] == '-' || filename[i] == '_')
            {
            }
            else
            {
                return 0;
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}
void responseGET(char *file, size_t byte, int currsock)
{
    char *buffer[BUFF_SIZE] = {0};
    char contentlen[BUFF_SIZE] = {0};
    int fd;
    if ((fd = open(file, O_RDONLY)) == -1)
    {
        //private file without permission
        if (errno == EACCES)
        {
            write(currsock, "HTTP/1.1 403 Forbidden\r\n\r\n", 26);
            close(fd);
            exit(EXIT_FAILURE);
        }
        //file does not exist
        if (errno == ENOENT)
        {
            write(currsock, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        while ((byte = read(fd, buffer, sizeof(buffer))) > 0)
        {
            struct stat sb;
            if (fstat(fd, &sb) == -1)
            {
                perror("stat");
                exit(EXIT_FAILURE);
            }
            else
            {
                sprintf(contentlen, "%ld", sb.st_size);
                write(currsock, "HTTP/1.1 200 OK\r\n", 17);
                write(currsock, "Content-Length: ", 16);
                write(currsock, contentlen, byte);
                write(currsock, NEWLINE, 4);
                write(currsock, buffer, byte);
            }
        }
    }
    close(fd);
}
void responsePUT(char *contentval, size_t byte, int currsock)
{
    char *file = contentval;
    if (access(file, F_OK) != -1)
    {
        //file exists
        handlefile(file, contentval, byte, currsock, 1);
    }
    else
    {
        handlefile(file, contentval, byte, currsock, 0);
    }
}
void handlefile(char *file, char *contentval, size_t byte, int currsock, int flag)
{
    char *buffer[BUFF_SIZE] = {0};
    char *contentlen[BUFF_SIZE] = {0};
    int fd;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    size_t currbyte;
    if ((fd = open(file, O_RDWR | O_CREAT | O_TRUNC, mode)) < 0)
    {
        if (errno == EACCES)
        {
            write(currsock, "HTTP/1.1 403 Forbidden\r\n\r\n", 26);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int enter = 0;
        while (contentval != NULL)
        {
            contentval = strtok(NULL, " ");
            if (contentval != NULL && strcmp(contentval, "*/*\r\nContent-Length:") == 0)
            {
                enter = 1;
                contentval = strtok(NULL, " ");
                int length = atoi(contentval);
                if (length == 0)
                {
                    if ((currbyte = read(currsock, buffer, byte)) != -1)
                    {
                        write(currsock, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
                        write(fd, buffer, length);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    char exists[100] = "HTTP/1.1 200 OK\r\nContent-Length: ";
                    char created[100] = "HTTP/1.1 201 Created\r\nContent-Length: ";
                    strcat(exists, contentval);
                    strcat(exists, NEWLINE);
                    strcat(created, contentval);
                    strcat(created, NEWLINE);
                    if ((currbyte = read(currsock, buffer, byte)) != -1)
                    {
                        flag == 1 ? write(currsock, exists, strlen(exists)) : write(currsock, created, strlen(created));
                        write(fd, buffer, length);
                    }
                }
            }
        }
        //used to check if contentlength exists
        if (enter == 0)
        {
            char exists[100] = "HTTP/1.1 200 OK\r\nContent-Length: ";
            char created[100] = "HTTP/1.1 201 Created\r\nContent-Length: ";
            strcat(exists, contentval);
            strcat(exists, NEWLINE);
            strcat(created, contentval);
            strcat(created, NEWLINE);
            if ((currbyte = read(currsock, buffer, byte)) != -1)
            {
                flag == 1 ? write(currsock, exists, strlen(exists)) : write(currsock, created, strlen(created));
                write(fd, buffer, currbyte);
            }
        }
    }
    close(fd);
}