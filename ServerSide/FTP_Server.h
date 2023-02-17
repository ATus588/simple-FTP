#ifndef FTP_SERVER_H
#define FTP_SERVER_H 


#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#define INVALID_SOCKET -1
#define INVALID_IP -1
#define MAX_SIZE 1024
#define PORT 9000
#define DEFAULT_PORT 3000
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

/**
 * Trim whiteshpace and line ending
 * characters from a string
 */
void trimstr(char *str, int n);

int socket_create();

/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept(int sock_listen);

int send_response(int sockfd, int rc);

/**
 * Receive data on sockfd
 * Returns -1 on error, number of bytes received 
 * on success
 */
int recv_data(int sockfd, char* buf, int bufsize);

/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char*user, char*pass);

/** 
 * Log in connected client
 */
int ftserve_login(int sock_control);

/**
 * Wait for command from client and
 * send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char*cmd, char*arg);

/**
 * Connect to remote host at given port
 * Returns:	socket fd on success, -1 on error
 */
int socket_connect(int port, char*host);

/**
 * Open data connection to client 
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn(int sock_control);


/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftserve_list(int sock_data, int sock_control);

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftpServer_cwd(int sock_control, char* folderName);

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
void ftpServer_pwd(int sock_control, int sock_data);

/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr(int sock_control, int sock_data, char* filename);

int recvFile(int sock_control ,int sock_data, char* filename);
/** 
 * Child process handles connection to client
 */
void ftserve_process(int sock_control);
#endif // FTP_SERVER_H