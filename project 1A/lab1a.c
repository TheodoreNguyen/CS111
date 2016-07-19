#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
pid_t global_child_pid = -1;

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

void handler(int signum)
{	
	int childstatus1;
	if(signum == SIGINT)
	{
		fprintf(stderr, "SIGINT recieved from terminal. Sending SIGINT to shell and exiting...\n");
		if(global_child_pid == -1)
			fprintf(stderr, "There is no child to send SIGINT to. Exiting..\n");
		else
		{
			kill(global_child_pid, SIGINT);
			waitpid(global_child_pid, &childstatus1, 0);
			printf("Child exit status: %i.\n", childstatus1);
		}
		exit(1);
	}
	else if(signum == SIGPIPE)
	{
		fprintf(stderr, "Received SIGPIPE, or EOF from shell. Exiting with return code 1");
		waitpid(global_child_pid, &childstatus1, 0);
		printf("Child exit status: %i.\n", childstatus1);
		exit(1);
	}
	else //this should never happen
		fprintf(stderr, "The handler was called and now everything will exit.\n");
		exit(1);
}

void thread_routine(void *ptr)
{
	signal(SIGPIPE, handler);
	char buf;
	int *passedpipe;
	passedpipe = (int *) ptr;
	while(read(passedpipe[0], &buf, 1) > 0)
	{
		if(buf == '\004')
			handler(SIGPIPE);
		write(STDOUT_FILENO, &buf, 1);
	}
}


int main (int argc, char* argv[])
{
  char c;
  int options;
  static int shell = 0;
  
  set_input_mode ();
  
  opterr = 0;
  while(1)
  {
	  static struct option long_options[] =
	  {
		  {"shell", no_argument, &shell, 1}
	  };
	  
	  int option_index = 0;
	  options = getopt_long(argc, argv, "", long_options, &option_index);
	  
	  if(options == -1)
		  break;
	  
	  switch(options)
	  {
		case 0:
			break;
		case '?':
			printf("Invalid option fed. Valid options are [--shell] and no options at all.\n");
			exit(EXIT_FAILURE);
		default:
			printf("how are you here\n");
			exit(1);
			break;
	  }
  }
 
 
  if(shell) 
  { 
	 int to_child_pipe[2];
     int from_child_pipe[2];
     pid_t rc = -1;
     int childstatus;
	 
	 signal(SIGINT, handler);
	
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
	 signal(SIGINT, handler);
	 rc = fork();
	 global_child_pid = rc;
	 if(rc < 0)			//fork failed
	 {
		 fprintf(stderr, "Fork failed.\n");
		 exit(1);
	 }
	 else if(rc == 0) //child process
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
	 else  //parent process, aka rc > 0
	 {
		 pthread_t thread1;
		 pthread_create(&thread1, NULL, (void *) &thread_routine, from_child_pipe);
		 
		 close(to_child_pipe[0]);
		 close(from_child_pipe[1]);
		 char buf1;
		 while(1)
		 {	 
			while(read(STDIN_FILENO, &buf1, 1) > 0)
			 {
				 if (buf1 == '\004')
				 {
					 printf("EOF received from terminal. Closing pipes..\n");
					 close(to_child_pipe[1]);
					 close(from_child_pipe[0]);
					 printf("Sending SIGHUP to child shell...\n");
					 kill(rc, SIGHUP);
					 waitpid(rc, &childstatus, 0);
					 printf("Child exit status: %i.\n", childstatus);
					 printf("Exiting, return code 0.\n");
					 exit(0);
				 }
				 else if(buf1 == '\n' || buf1 == '\r')
				 {
					 buf1 = '\r';
					 write(STDOUT_FILENO, &buf1, 1);
					 buf1 = '\n';
					 write(STDOUT_FILENO, &buf1, 1);
					 write(to_child_pipe[1], &buf1, 1);
				 }
				 else
				 {
					write(STDOUT_FILENO, &buf1, 1);
					write(to_child_pipe[1], &buf1, 1);
				 } 
			 }
			 
			 
			 //shouldnt get here
			 pthread_join(thread1, NULL);
			 waitpid(rc, &childstatus, 0);
			 printf("Child exit status: %i.\n", childstatus);
			 printf("The parent has somehow got here and is exiting\n");
			 exit(1);
		 }
     }
 
  //'\004' is EOT, execute with C-d
  //'\010' is LF, or newline, execute with C-j			\n
  //'\013' is CR, or carriage return, execute with C-m	\r
  }  
  else 		//if shell option not specified, do behaviour implemented in part 1
  {
	while (1)
    {
      read (STDIN_FILENO, &c, 1);
      if (c == '\004')          /* C-d */
        break;
	  else if(c == '\r' || c == '\n')
	  {
		  c = '\r';
		  putchar(c);
		  c = '\n';
		  putchar(c);
	  }
      else
        putchar(c);
    }
	printf("EOT entered, exiting from no-shell-option and restoring original terminal settings..\n");	
  }
  
  
  //atexit allows automatic restoration of normal terminal settings on exit	
  return EXIT_SUCCESS;
}
