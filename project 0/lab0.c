#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

//induces a segmentation fault SIGSEGV by setting a value equal to that pointed by nullptr
void induceSIGSEGV(void)
{
	int *a = 0;
	int b = *a;	
	return;
}

//handles the SIGSEGV signal and exits
void handler(int signum)
{
	if(signum == SIGSEGV)
		perror("Caught a segmentation fault. Exiting the program...\n");
	else
		perror("The handler was called, and we are not equipped for other signals. Exiting.\n");
	exit(3);

}

//main function. Takes in input from command line
int main(int argc, char **argv)
{
	opterr = 0;			//get rid of warning where getopt() doesnt recognize option character. this is handled personally				
	int c;				//integer used for switch statement 
	static int segfault = 0;	//integers indicating if the respective option - segfault, catch, input, and output - were used
	static int catch = 0;
	int in = 0;
	int out = 0;
	char* input;			//C-strings that are the names of the input and output values (assuming they are used)
	char* output;
	int inputfd;			//integers that correspond to the file descriptors of the input and output files (assuming used)
	int outputfd;
	
	//parse through every input option
	while(1)
	{
		//define the options. {name of option, argument_needed?, int*ptr for parsing option, VAL}
		static struct option long_options[] =
		{
			{"segfault", no_argument, &segfault, 1},
			{"catch", no_argument, &catch, 1},
			{"input", required_argument, 0, 'i'},
			{"output", required_argument, 0, 'o'},
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "", long_options, &option_index);
		//printf("the value of c is %i.\n", c);
		
		//break out of the while loop when there are no more options left to parse
		if(c == -1)
			break;
		
		switch (c)
		{	
			//when the flag of the option is NOOTT equal to NULL (aka 0) like segfault and catch,
			//getopt_long returns 0 and sets the flag equal to val (the 4th item above).
			case 0:	
				//printf("do nothing.\n); 
				break;
			//when the flag of the option is equal to NULL (aka 0) like input and output, getopt_long returns
			//val for the option (the 4th item above, 'i' and 'o' for input and output respectively).
			case 'i':
			//	printf("the value of input's parameter is %s\n", optarg);	//line used for debugging
				input = optarg;
				in = 1;
				break;
			case 'o':
			//	printf("the value of output's parameter is %s\n", optarg);	//line used for debugging
				output = optarg;
				out = 1;
				break;
			//if the input option is invalid, getopt_long returns '?'. Prompt the user for correct option
			case '?':
				printf("Invalid option fed. Valid options are [--segfault], [--catch], [--input=inputfile] or [--input inputfile], [--output=outputfile] or [--output outputfile].\n");
				exit(EXIT_FAILURE);
			//you should never get here, as getopt_long and getopt return -1 when there are no more option
			//chars left, and we will break out of the while loop
			default:
				printf("you should not get here\n");
				break;
		}
	}
	
	//used for debugging purposes	
	/*
	int i;
	for (i = 0; i != argc; i++)
		printf("%s,",argv[i]);
	printf("\n");
	*/
	//printf("%i, %i\n", segfault, catch);


	//induce segfault/catching 
	if(segfault)
	{	
		if(catch)
			signal(SIGSEGV, handler);
		induceSIGSEGV();
	}


	//this part is the redirection and file I/O
	
	//save a copy of the file descriptor for STDIN and STDOUT	
	int insave = dup(0);
	int outsave = dup(1);
	
	//if an input file was specified, redirect its descriptor to STDIN
	if(in)
	{
		inputfd = open(input, O_RDONLY);		//opens file descriptor
		if(inputfd == -1)				//check if there was any problems
		{
			perror("Error with input file:");
			exit(1);
		}
		dup2(inputfd, 0);				//replace STDIN with the file
	}	
	
	//identical and analogous to the input file -> STDIN, except now its output file -> STDOUT
	if(out)
	{
		outputfd = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if(outputfd == -1)
		{
			perror("Error with output file:");
			exit(2);
		}
		dup2(outputfd, 1);
	}
	
	//read from STDIN and write to STDOUT char by char.
	char ch;
	while(read(STDIN_FILENO, &ch, 1) > 0)
		write(1, &ch, 1);
	
	//redirect the STDIN and STDOUT back to the console using the saved descriptors at the beginning
	//then close them as we are done using them
	if(in)
	{
		dup2(insave, 0);
		close(insave);
	}
	if(out)
	{
		dup2(outsave, 1);
		close(outsave);
	}

	exit(0);		
}
