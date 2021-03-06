#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_cd(tok_t arg[]){
  if(arg[0] == NULL || strcmp(arg[0], "~") == 0){
    arg[0] = "/home/";
  }
  int i = chdir(arg[0]);
  if(i == -1){
    return -1;
  }

  return 1;
}

int cmd_help(tok_t arg[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_cd, "cd", "change current directory"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){

    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
  /** YOUR CODE HERE */
  return NULL;
}

char* concat(char *s1, char *s2){
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);	/* user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;

  tok_t *paths_t;
  int rtn, fd, in, saved_stdout,saved_stdin, i, access_check, status;
  char *paths;
  char *full_path;
  int out_flag, in_flag = 0;
  char *inname;
  char *outname;
  char *cwd = malloc(INPUT_STRING_SIZE+1);
  size_t size = INPUT_STRING_SIZE+1;
  cwd = getcwd(cwd, size);
  int flag1 = 0;

  init_shell();

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

  lineNum=0;
  fprintf(stdout, "%d %s: ", lineNum, cwd);
  while ((s = freadln(stdin))){
    inname = strstr(s, "<");
    outname = strstr(s, ">"); 
    if(inname){ 
      in_flag = 1;
      inname[0] = '\0';
      inname++;
      if(inname[0] == ' '){
        inname++;
        flag1 = 1;
      }       
      inname[strlen(inname) - 1] = '\0';
    }
    if(outname){ 
      out_flag = 1;
      outname[0] = '\0';
      outname++;
      if(outname[0] == ' '){
        outname++;
      }
      if(in_flag == 0){
        outname[strlen(outname)-1] = '\0';
      }
    }
    if(inname){
      if(out_flag == 1 && flag1 == 1){
        inname[strlen(inname) - 1] = '\0';
      }
    }

    t = getToks(s); /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if(out_flag == 1){
      fd = open(outname, O_WRONLY | O_CREAT | O_TRUNC, mode);    
      saved_stdout = dup(1);
    }
    if(in_flag == 1){
      in = open(inname, O_RDONLY);
      saved_stdin = dup(0);
    }

    if(fundex >= 0){ 
      cmd_table[fundex].fun(&t[1]);
    } 
    else{ 
      cpid = fork(); 
      
      if(out_flag == 1){
        dup2(fd, 1);
        close(fd);
      }
      if(in_flag == 1){
        dup2(in, 0);
        close(in);
      }
      if(cpid == -1){ 
        perror("fork failure");
        exit(1);
      }
      else if(cpid == 0){
	rtn = execv(t[0], t);

        if(rtn == -1){
          paths = getenv("PATH");
          paths_t = getToks(paths); 
          for(i = 0; i < 9; i++){
            full_path = concat(paths_t[i], "/");
            full_path = concat(full_path, t[0]);
            access_check = access(full_path, F_OK);

            if(access_check == 0){
              rtn = execv(full_path, t);
              if(rtn == -1){
                printf("only support proper built ins \n");
                exit(0);  
              }
            }	
          }
        }  
        exit(0);
      }  
      
    else {
      wait(&status);
    }
  }
    if(out_flag == 1){
      dup2(saved_stdout, 1);
      close(saved_stdout);
    }
    if(in_flag == 1){
      dup2(saved_stdin, 0);
      close(saved_stdin);
    }
    cwd = getcwd(cwd, size);
    lineNum += 1;
    fprintf(stdout, "%d %s: ", lineNum, cwd);
  }
  return 0;
}
