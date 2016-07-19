#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <mcrypt.h>
#include <getopt.h>

#include <math.h>
#include <stdint.h>
#include <fcntl.h>


static char* globalkey;
static int encrypt = 0;
int global_newsockfd;
int global_childpid;
void handler(int signum);

void thread_routine(void *ptr);

int Encrypt_Decrypt(void *buf, int buflen, char* IV, char* key, int keylen, int option);

int main( int argc, char *argv[] ) 
{
   char c;
   int options;
   char * port;
   int ported = 0;
   int portno;
   opterr = 0;
   
   while(1)
   {
	   static struct option long_options[] =
	   {
		 {"encrypt", no_argument, &encrypt, 1},
		 {"port", required_argument, 0, 'p'},
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
		   case '?':
				perror("Incorrect option. Only correct options are [--encrypt] [--port=<portnum>].");
				break;
		   default:
				perror("don't get here");
				exit(1);
	   }
   }
   if(ported != 1)
   {
	   perror("No port specified");
	   exit(1);
   }
	
   int sockfd, newsockfd, clilen;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int  n;
   
   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }
      
   /* Now start listening for the clients, here process will
 *       * go in sleep mode and will wait for the incoming connection
 *          */
   
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   /* Accept actual connection from the client */
   newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	
  
	
	
   if (newsockfd < 0) {
      perror("ERROR on accept");
      exit(1);
   }
   
   
   
   global_newsockfd = newsockfd;


   int to_child_pipe[2];
   int from_child_pipe[2];
   pid_t rc = -1;
   int childstatus;
   
   if(pipe(to_child_pipe) == -1)
   {
	  fprintf(stderr, "Pipe failure.\n");
      exit(1);
   }
   if(pipe(from_child_pipe) == -1)
   {
      fprintf(stderr, "Pipe failure.\n");
	  exit(1);
   }
   
   //---------------get key from file in case of encryption. 
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
	globalkey = key;
	//-------------------end get key from file
   
   
   rc = fork();
   global_childpid = rc;
   if(rc < 0)
   {
	   fprintf(stderr, "fork failed\n");
	   exit(1);
   }
   else if(rc == 0) //child process aka forked shell
   {
	      close(to_child_pipe[1]);
	      close(from_child_pipe[0]);
		  //make the child process's standard input be to_child_pipe[0]
	      dup2(to_child_pipe[0], STDIN_FILENO);	
		  //make the child process's standard output be from_child_pipe[1];
		  dup2(from_child_pipe[1], STDOUT_FILENO);
		  dup2(from_child_pipe[1], STDERR_FILENO);
		  close(to_child_pipe[0]);
		  close(from_child_pipe[1]);
		  
		 
		  
		  char *execvp_argv[2];
		  char execvp_filename[] = "/bin/bash";
		  execvp_argv[0] = execvp_filename;
		  execvp_argv[1] = NULL;
		  if(execvp(execvp_filename, execvp_argv) == -1)
		  {
			  fprintf(stderr, "execvp() failed.\n");
			  exit(1);
		  }
		  _exit(1); //should never execute
   }
   else		//parent aka server process aka parent thread
   {
		  pthread_t thread1;
		  pthread_create(&thread1, NULL, (void *) &thread_routine, from_child_pipe);
		
		  close(to_child_pipe[0]);
		  close(from_child_pipe[1]);
		  dup2(newsockfd, STDIN_FILENO);
		  MCRYPT td, td2;
		  char buf;
		  char buffer[16];
		  char send;
		  int bufcounter = 0;
		  char *IV = "AAAAAAAAAAAAAAAA";
		  while(read(STDIN_FILENO, &buf, 1) > 0)
		  {
			  if(encrypt)
			  {
				if(bufcounter == 16)
				{
					bufcounter = 0;
					Encrypt_Decrypt(buffer, 16, IV, key, keylen, 1);
					send = buffer[8];
					write(to_child_pipe[1], &send, 1);
				}
				buffer[bufcounter] = buf;
				bufcounter++;
			  }
			  else
				write(to_child_pipe[1], &buf, 1);
		  }
		  //getting here means we the server got a EOF or read error from socket
		  //since got EOF/read error from network connection, 
		  perror("Server received read error or EOF from socket. Closing network, killing child shell, exiting RC = 1.");
		  close(newsockfd);
		  kill(rc, SIGTERM);
		  waitpid(rc, &childstatus, 0);
		  pthread_join(thread1, NULL);
		  exit(1);
   }
			
			
			
   return 0;
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


void thread_routine(void *ptr)
{	
	signal(SIGPIPE, handler);
	dup2(global_newsockfd, STDOUT_FILENO);
	dup2(global_newsockfd, STDERR_FILENO);
	char buf;
	int *passedpipe;
	passedpipe = (int *) ptr;
	//MCRYPT td, td2;
	char* IV = "AAAAAAAAAAAAAAAA";
	int j;
	char buffer[16];
	while(read(passedpipe[0], &buf, 1) > 0)
	{	/*
		if(encrypt)
		{
			for(j = 0; j != 16; j++)
			{
				if(j == 8)
					buffer[j] = buf;
				else
					buffer[j] = (rand() % (122-48)) + 48;
			}
			Encrypt_Decrypt(buffer, 16, IV, globalkey, 16, 0);
			
			for(j = 0; j != 16; j++)
			{
				buf = buffer[j];
				write(STDOUT_FILENO, &buf, 1);
			}
		}
		
		else */
			write(STDOUT_FILENO, &buf, 1);	
	}
	//if get a EOF from shell pipe, read return 0 and we end up here
	perror("Server received EOF from shell, Exiting with RC =2");
	close(global_newsockfd);
	exit(2);
}

void handler(int signum)
{
	int childstatus1;
	if (signum == SIGPIPE)
	{
		fprintf(stderr, "Server received SIGPIPE from shell. Exiting with RC =2\n");
		close(global_newsockfd);
		waitpid(global_childpid, &childstatus1, 0);
		exit(2);
	}
}
