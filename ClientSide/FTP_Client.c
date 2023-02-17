#include "FTP_Client.h"

/*Validating IP Address*/
int validate_ip(const char *ip){
	int value_1 = -1;
	int value_2 = -1;
	int value_3 = -1;
	int value_4 = -1;
	int count = 0;
	int i = 0;

	while(ip[i] != '\0')
	{
		if(ip[i] == '.')
			count++;
		i++;
	}
	
	if(count != 3 )
		return INVALID_IP;
	else
	{
		sscanf(ip,"%d.%d.%d.%d",&value_1,&value_2,&value_3,&value_4);
		
		if(value_1 < 0 || value_2 < 0 || value_3 < 0 || value_4 < 0 || value_1 > 255 || value_2 > 255 || value_3 > 255 || value_4 > 255)/* IP Addresses from 0.0.0.0 to 255.255.255.255*/
			return INVALID_IP;
		else
			return 1;
	}
}


/**
 * Print response message
 */
void print_reply(int rc) {
	switch (rc) {
		case 220:
			printf("220 Welcome, FTP server ready.\n");
			break;
		case 221:
			printf("221 Goodbye!\n");
			break;
		case 212:
			printf("221 Directory status!\n");
			break;
		case 226:
			printf("226 Closing data connection. Requested file action successful.\n");
			break;
		case 250:
			printf("250 Directory successfully changed.\n");
			break;
		case 550:
			printf("550 Requested action not taken. File unavailable.\n");
			break;
	}
}

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply(int sock_control){
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof(retcode), 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}	
	return retcode;
}

/**
 * Input: cmd struct with an a code and an arg
 * Concats code + arg into a string and sends to server
 */
int ftclient_send_cmd(struct command *cmd, int sock_control)
{
	char buffer[MAX_SIZE+5];
	int rc;

	sprintf(buffer, "%s %s", cmd->code, cmd->arg);
	
	// Send command string to server
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server");
		return -1;
	}
	return 0;
}

/** 
 * Read input from command line
 */
void read_input(char* user_input, int size)
{
	memset(user_input, 0, size);
	int n = read(STDIN_FILENO,user_input,size);
	user_input[n] = '\0';		
	
	/* Remove trailing return and newline characters */
	if(user_input[n - 1] == '\n')
		user_input[n - 1] = '\0';
	if(user_input[n - 1] == '\r')
		user_input[n - 1] = '\0';
}


/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login(int sock_control)
{
	struct command cmd;
	char user[MAX_SIZE];
	memset(user, 0, MAX_SIZE);

	// Get username from user
	printf("Name: ");	
	fflush(stdout); 		
	read_input(user, MAX_SIZE);

	// Send USER command to server
	strcpy(cmd.code, "USER");
	strcpy(cmd.arg, user);
	ftclient_send_cmd(&cmd, sock_control);
	
	// Wait for go-ahead to send password
	int wait;
	recv(sock_control, &wait, sizeof(wait), 0);

	// Get password from user
	fflush(stdout);	
	char *pass = getpass("Password: ");	

	// Send PASS command to server
	strcpy(cmd.code, "PASS");
	strcpy(cmd.arg, pass);
	ftclient_send_cmd(&cmd, sock_control);
	
	// wait for response
	int retcode = read_reply(sock_control);
	switch (retcode) {
		case 430:
			printf("430 Invalid username/password.\n");
			exit(0);
		case 230:
			printf("230 Successful login.\n");
			break;
		default:
			perror("error reading message from server");
			exit(1);		
			break;
	}
}

/**
 * Parse command in cstruct
 */ 
int ftclient_read_command(char* user_input, int size, struct command *cstruct){
	
	memset(cstruct->code, 0, sizeof(cstruct->code));
	memset(cstruct->arg, 0, sizeof(cstruct->arg));
	
	printf("ftp> ");	// prompt for input		
	fflush(stdout); 	

	// wait for user to enter a command
	read_input(user_input, size);

	// user_input: 
	// chang directory on client side
	if (strcmp(user_input, "!ls") == 0 || strcmp(user_input, "!ls ") == 0) {
		system("ls"); // client side
		return 1;
	}
	else if (strcmp(user_input, "!pwd") == 0 || strcmp(user_input, "!pwd ") == 0) {
		system("pwd"); // client side
		return 1;
	}
	else if (strncmp(user_input,"!cd ",4) == 0) {
		if(chdir(user_input + 4) == 0){
			printf("Directory successfully changed\n");
		}			
		else{
			perror("Error change directory");
		}
		return 1;
	}
	// change directory on server side
	else if (strcmp(user_input, "ls ") == 0 || strcmp(user_input, "ls") == 0) {
		strcpy(cstruct->code, "LIST");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else if (strncmp(user_input, "cd ", 3) == 0) {
		strcpy(cstruct->code, "CWD ");
		strcpy(cstruct->arg, user_input+3);
		
		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s",cstruct->code, cstruct->arg);
	}
	else if (strcmp(user_input, "pwd") == 0 || strcmp(user_input, "pwd ") == 0) {
		strcpy(cstruct->code, "PWD ");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	// upload and download file
	else if (strncmp(user_input, "get ", 4) == 0) { // RETRIEVE
		strcpy(cstruct->code, "RETR");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s",cstruct->code, cstruct->arg);		
	}
	else if (strncmp(user_input, "put ", 4) == 0) {
		strcpy(cstruct->code, "STOR");	// STORE
		strcpy(cstruct->arg, user_input+4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s",cstruct->code, cstruct->arg);
	}
	// quit
	else if (strcmp(user_input, "quit") == 0) {
		strcpy(cstruct->code, "QUIT");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else { // invalid
		return -1;
	}

	return 0;
}

/**
 * Create listening socket on remote host
 * Returns -1 on error, socket fd on success
 */
int socket_create(int port)
{
	int sockfd;
	SOCKADDR_IN sock_addr;

	// create new socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error"); 
		return -1; 
	}

	// set local address info
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind
	int flag = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
	if (bind(sockfd, (SOCKADDR *) &sock_addr, sizeof(sock_addr)) < 0) {
		close(sockfd);
		perror("bind() error"); 
		return -1; 
	}
   
	// begin listening for incoming TCP requests
	if (listen(sockfd, 5) < 0) {
		close(sockfd);
		perror("listen() error");
		return -1;
	}              
	return sockfd;
}

/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept(int sock_listen)
{
	int sockfd;
	SOCKADDR_IN client_addr;
	int len = sizeof(client_addr);

	// Wait for incoming request, store client info in client_addr
	sockfd = accept(sock_listen, (SOCKADDR *) &client_addr, &len);
	
	if (sockfd < 0) {
		perror("accept() error"); 
		return -1; 
	}
	return sockfd;
}

/**
 * Open data connection
 */
int ftclient_open_conn(int sock_con){
	int sock_listen = socket_create(DEFAULT_PORT);

	// send an ACK on control conn
	int ack = 1;
	if ((send(sock_con, &ack, sizeof(ack), 0)) < 0) {
		printf("client: ack write error :%d\n", errno);
		exit(1);
	}		

	int sock_conn = socket_accept(sock_listen);
	close(sock_listen);
	return sock_conn;
}

/** 
 * Do list commmand
 */
int ftclient_list(int sock_data, int sock_ctrl)
{
	size_t num_recvd;			// number of bytes received with recv()
	char buf[MAX_SIZE];			// hold a filename received from server
	int tmp = 0;
	
	memset(buf, 0, sizeof(buf));
	while ((num_recvd = recv(sock_data, buf, MAX_SIZE, 0)) > 0) {
        printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	
	if (num_recvd < 0) {
	    perror("error");
	}
	return 0;
}

/**
 * Do get <filename> command 
 */
int ftclient_get(int data_sock, int sock_control, char* arg){
    char data[MAX_SIZE];
    int size;
    FILE* fd = fopen(arg, "w");
    
    while ((size = recv(data_sock, data, MAX_SIZE, 0)) > 0) {
        fwrite(data, 1, size, fd);
    }

    if (size < 0) {
        perror("error\n");
    }

    fclose(fd);
    return 0;
}

void upload(int data_sock, char* filename, int sock_control) {

	FILE* fd = NULL;
	char data[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;
	int stt;					
		
	fd = fopen(filename, "r");
	
	if (!fd) {	
		// send error code (550 Requested action not taken)
		printf("ko the mo file\n");
		stt = 550;
		send(sock_control, &stt, sizeof(stt), 0);
	} else {	
		// send okay (150 File status okay)
		stt = 150;
		send(sock_control, &stt, sizeof(stt), 0);
	
		do {
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0) {
				printf("error in fread()\n");
			}

			// send block
			send(data_sock, data, num_read, 0);

		} while (num_read > 0);											

		fclose(fd);
	}

}