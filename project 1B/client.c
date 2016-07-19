#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

#include <fcntl.h>

#include <mcrypt.h>
#include <math.h>
#include <stdint.h>

static int globalkeylen;
static char* globalkey;
static char *bytes = " bytes: ";
static char newline = '\n';
static int logfd;
static int logged = 0;
static int encrypt = 0;

struct termios saved_attributes;
void reset_input_mode (void);
void set_input_mode (void);

void thread_routine(void *ptr);

int Encrypt_Decrypt(void *buf, int buflen, char* IV, char* key, int keylen, int option);

int main(int argc, char* argv[])
{
	char c;
	int options;
	
	int ported = 0;
	char* port;
	char* log;
	
	set_input_mode();
	opterr = 0;
	
	int sockfd;
	int portno;
	int n;
	
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	
	while(1)
	{
		static struct option long_options[] =
		{
			{"encrypt", no_argument, &encrypt, 1},
			{"port", required_argument, 0, 'p'},
			{"log", required_argument, 0, 'l'}
		};
		
		int option_index = 0;
		options = getopt_long(argc, argv, "", long_options, &option_index);
		
		if(options == -1)
			break;
		
		switch(options)
		{
			case 0:
				break;
			case 'p':
				port = optarg;
				portno = atoi(port);
				ported = 1;
				break;
			case 'l':
				log = optarg;
				logged = 1;
				break;
			case '?':
				perror("Invalid option fed. Valid options are [--encrypt], [--port=<portnum>], [--log=<logname>].");
				exit(EXIT_FAILURE);
			default:
				perror("You should never get here.");
				exit(EXIT_FAILURE);
		}
	}
		//check if a port was specified. if not, we are done here
	if(ported)
	{
		if(logged)
		{
			logfd = open(log, O_CREAT | O_RDWR | O_TRUNC | O_APPEND, 0666);
			if(logfd == -1)
			{
				perror("Error with log file creation:");
				exit(2);
			}
		}
		//network business
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0)
		{
			perror("There was an error opening the socket. Exiting...");
			exit(EXIT_FAILURE);
		}
		
		server = gethostbyname("localhost");
		if(server == NULL)
		{
			fprintf(stderr, "Error, there exists no such host.\n");
			exit(0);
		}
		
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(portno);
		
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("There was an error connecting.");
			exit(1);
		}
		
		
		//thread has client receiving from socket, writing to standard output
		pthread_t thread1;
		pthread_create(&thread1, NULL, (void *) &thread_routine, &sockfd);
		
		//read message from user to be sent to server
		//read from terminal, output to socket
		
		
		//client receiving from standard input, writing to socket
		
		
		//------------encryption business - operates even when encryption not called, but will not do anything besides store vars
		
		
		char keybuf;
		int keylen = 0;
		int keyfd = open("my.key", O_RDONLY);
		if(keyfd == -1)
		{
			if(encrypt)
			{
				printf("error opening key file");
				exit(1);
			}
		}
		//if(encrypt)
			while(read(keyfd, &keybuf, 1) > 0)
				keylen++;
		close(keyfd);
		keyfd = open("my.key", O_RDONLY);
		if(keyfd == -1)
		{
			if(encrypt)
			{
				printf("error opening key file");
				exit(1);
			}
		}
		char key[keylen];
		if(encrypt)
		{
			read(keyfd, key, keylen);
		}
		//printf("%s", key);
		globalkey = key;
		globalkeylen = keylen;
		//--------------------------------------end of encryption business
		
		
		
		
		
		char* IV = "AAAAAAAAAAAAAAAA";
		const int buffer_len = 16;
		char buffer[16];
		int i;
		int j;
		int randomindex;
		
		char buf;
		char *SENT = "SENT ";
		char numbytes = '1';
		
		
		
		while(read(STDIN_FILENO, &buf, 1) > 0)
		{
			if(buf == '\004')		
			{
				perror("^D was entered on terminal, closing connections and exiting with RC = 0");
				close(sockfd);
				exit(0);
			}
			
			if(encrypt)
			{
				write(STDOUT_FILENO, &buf, 1);
				randomindex = 8;//rand() % buffer_len;
				for(j = 0; j != buffer_len; j++)
				{
					if(j == randomindex)
						buffer[j] = buf;
					else
						buffer[j] = (rand() % (122 - 48)) + 48;
				}
				Encrypt_Decrypt(buffer, buffer_len, IV, key, keylen, 0);
				for(j = 0; j != buffer_len; j++)
				{
					buf = buffer[j];
					if(logged)
					{
						write(logfd, SENT, 5);
						write(logfd, &numbytes, 1);
						write(logfd, bytes, 8);
						write(logfd, &buf, 1);
						write(logfd, &newline, 1);
					}
					write(sockfd, &buf, 1);
				}
			}
			else
			{	
				if(logged)
				{
					
					write(logfd, SENT, 5);
					write(logfd, &numbytes, 1);
					write(logfd, bytes, 8);
					write(logfd, &buf, 1);
					write(logfd, &newline, 1);
				}
					write(sockfd, &buf, 1);
					write(STDOUT_FILENO, &buf, 1);
			}
		}
		pthread_join(thread1, NULL);
	}
	else
	{
		perror("No port was specified. Will not do anything. Exiting...");
		exit(EXIT_FAILURE);
	}
	
	printf("You've reached the end of the client code SOMEHOW. Now exiting...\n");
	return 0;
}


//client receiving from socket, writing to standard output
void thread_routine(void *ptr)
{	
	char *RECEIVED = "RECEIVED ";
	char numbytes = '1';
	char buf;
	int *passedpipe;
	passedpipe = (int *) ptr;
	
	char buffer[16];
	char send;
	int bufcounter = 0;
	char *IV = "AAAAAAAAAAAAAAAA";
	while( read(*passedpipe, &buf, 1) > 0)
	{  /*
		if(encrypt)
		{
			if(logged)
			{
				write(logfd, RECEIVED, 9);
				write(logfd, &numbytes, 1);
				write(logfd, bytes, 8);
				write(logfd, &buf, 1);
				write(logfd, &newline, 1);
			}
			//printf("bufcounter %i", bufcounter);
			if(bufcounter == 16)
			{
				bufcounter = 0;
				Encrypt_Decrypt(buffer, 16, IV, globalkey, globalkeylen, 1);
				send = buffer[8];
				
				//printf("%c\n\n", send);
				
				write(STDOUT_FILENO, &send, 1);
			}
			buffer[bufcounter] = buf;
			bufcounter++;
		}
		else  */
		{
			if(logged)
			{
				write(logfd, RECEIVED, 9);
				write(logfd, &numbytes, 1);
				write(logfd, bytes, 8);
				write(logfd, &buf, 1);
				write(logfd, &newline, 1);
			}
			write(STDOUT_FILENO, &buf, 1);
		}
	}
	//if you are here, then either read() returned 0 (reached end of file)
	//or read() returned -1 (read encountered an error)
	//thus, to be here, the client either got a read error or EOF from the socket
	perror("read() got an EOF or an error from socket. Closing network, exiting RC=1");
	close(*passedpipe);
	exit(1);
}

int Encrypt_Decrypt(void *buf, int buflen, char* IV, char* key, int keylen, int option)
{
	if(option == 0 || option == 1)
	{
		MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
		int blocksize = mcrypt_enc_get_block_size(td);
		
		if((buflen % blocksize) != 0 )
			return 1;
		
		mcrypt_generic_init(td, key, keylen, IV);
		if(option == 0)
			mcrypt_generic(td, buf, buflen);
		else if(option == 1)
			mdecrypt_generic(td, buf, buflen);
		else
			perror("It is impossible to get here.");
		mcrypt_generic_deinit (td);
		mcrypt_module_close(td);
	}
	else
	{
		perror("Please specify an option, 6th parameter. option=0 for encrypt, option=1 for decrypt");
		exit(1);
	}
	
	return 0;
	
}



void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode (void)
{
  struct termios tattr;
  char *name;

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);

  /* Set the funny terminal modes. */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}







