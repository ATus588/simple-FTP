#include "FTP_Client.h"

// Compile command: gcc client.c FTPClient.c -o ftpclient

int main(int argc, char const *argv[])
{
	int sock_control;
	int data_sock, retcode;
	char user_input[MAX_SIZE];
	struct command cmd;

	if (argc != 2) {
		printf("usage: ./ftclient ip-address\n");
		exit(0);
	}

	int ip_valid = validate_ip(argv[1]);
	if(ip_valid == INVALID_IP) {
		printf("Error: Invalid ip-address\n");
		exit(1);
	}

	sock_control = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(sock_control == INVALID_SOCKET) {
		perror("Error");
		exit(1);
	}

	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT); // use some unused port number
	servAddr.sin_addr.s_addr = inet_addr(argv[1]);

	int connectStatus = connect(sock_control, (SOCKADDR*)&servAddr, sizeof(servAddr));

	if (connectStatus == -1) {
		printf("Error...\n");
		exit(1);
	}

	// Get connection, welcome messages
	printf("Connected to %s.\n", argv[1]);
	print_reply(read_reply(sock_control)); 
	

	/* Get name and password and send to server */
	ftclient_login(sock_control);

	while (1) { // loop until user types quit

		// Get a command from user
		int cmd_stt = ftclient_read_command(user_input, sizeof(user_input), &cmd);
		if ( cmd_stt == -1 ) {
			printf("Invalid command\n");
			continue;	// loop back for another command
		} else if( cmd_stt == 0 ){

			// Send command to server
			if (send(sock_control, user_input, strlen(user_input), 0) < 0 ) {
				close(sock_control);
				exit(1);
			}

			retcode = read_reply(sock_control);		
			if (retcode == 221) {
				/* If command was quit, just exit */
				print_reply(221);
				break;
			}
			
			if (retcode == 502) {
				// If invalid command, show error message
				printf("%d Invalid command.\n", retcode);
			} else {
				
			// Command is valid (RC = 200), process command
			
				// open data connection
				if ((data_sock = ftclient_open_conn(sock_control)) < 0) {
					perror("Error opening socket for data connection");
					exit(1);
				}
				
				// execute command
				if (strcmp(cmd.code, "LIST") == 0) {
					ftclient_list(data_sock, sock_control); 
				} else if(strcmp(cmd.code, "CWD ") == 0) {
					if(read_reply(sock_control) == 250) {
						print_reply(250);
					} else {
						printf("%s is not a directory\n", cmd.arg);
					}
				} else if(strcmp(cmd.code, "PWD ") == 0) {
					if(read_reply(sock_control) == 212) {
						ftclient_list(data_sock, sock_control); // ham nay in mess tu server
					}
				}
				else if (strcmp(cmd.code, "RETR") == 0) {
					// wait for reply (is file valid)
					if (read_reply(sock_control) == 550) {
						print_reply(550);		
						close(data_sock);
						continue; 
					}
					clock_t start = clock();
					ftclient_get(data_sock, sock_control, cmd.arg);
					clock_t end = clock();
					double cpu_time = ((double)(end - start))/CLOCKS_PER_SEC;
					print_reply(read_reply(sock_control));
					printf("Time taken %lf\n",cpu_time);
				}
				else if (strcmp(cmd.code, "STOR") == 0) {
					printf("Uploading ...\n");
					upload(data_sock,cmd.arg, sock_control);
					printf("xong\n");
				}
				close(data_sock);
			}
		}

	} // loop back to get more user input

	// Close the socket (control connection)
	close(sock_control);
	return 0;
}