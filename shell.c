#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//all global variables                                                                 
#define BUFF 4069
char buffer[BUFF];


//declares exit function                                                               
int cmdexit(char **args);
int cmdcd(char **args);
void prompt1(void);
void prompt2(void);

//allows for exit input to be input from user to exit program                          
char *built_str[] = {"exit", "cd"};
int (*built_func[]) (char**) = {&cmdexit, &cmdcd};

int built_nums() {
  return sizeof(built_str) / sizeof(char*);
}

int redir(char **args) {//function for redirection                                     

  int in = 0, out = 0;
  char input[BUFF], output[BUFF];

  for(int i = 0; i < strlen(*args); i++) {
    if(strcmp(args[i], "<") == 0) {//checks if command is <                            
      strcpy(input, args[i+1]);
      args[i] = NULL;
      in = 1;
    }
    if(strcmp(args[i], ">") == 0) { //checks if command is >                           
      strcpy(output, args[i+1]);
      args[i] = NULL;
      out = 1;
    }
  }
  if(in == 1) { //if command contains <                                                
    int fd1 = open(input, O_RDONLY);
    if(fd1 < 0) {
      write(STDERR_FILENO, "ERROR\n", 6);
      exit(0);
    }
    return dup2(fd1, STDIN_FILENO);
  }
  if(out == 1) { //if command contains >                                               
    int fd2 = creat(output, BUFF);
    if(fd2 < 0) {
      write(STDERR_FILENO, "ERROR\n", 6);
      exit(0);
    }
    return dup2(fd2, STDOUT_FILENO);
  }
  else return 1;
}

/*                                                                                     
  @allows for program to start and ius passed to execute function                      
  @to see what command is being input                                                  
*/
int start(char **args) {

  pid_t pid, wpid;
  int status;
  int options = 0;// in = 0, out = 0;                                                  
  //char input[BUFF], output[BUFF];                                                    

  pid = fork();
  if(pid == 0) { //child process                                                       
    if(execvp(args[0], args) == -1) { //error message if forking doesn't work          
      write(STDERR_FILENO, "ERROR\n", 6);
    }

    exit(EXIT_FAILURE);
    redir(args);
  } else if(pid < 0) { //error message if forking doesn't work                         
    write(STDERR_FILENO, "ERROR\n", 6);
  } else { //parent process                                                            
    do {
      wpid = waitpid(pid, &status, options);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  if(WIFSIGNALED(status)) {//prints program status                                     
    int ts = WTERMSIG(status);
    printf("child with pid %d exited abnormally due to signl = %d\n", wpid, ts);
  }
  return 1;
}

//function takes user input and returns command                                        
char *read_line(void) {
  char *line = NULL;
  size_t size = 0;
  ssize_t s_read = getline(&line, &size, stdin);

  //getline checks if user input a command                                             
  if(s_read < 0) {
    exit(EXIT_SUCCESS);
  }

  return line;
}

#define TOK_D " \t\r\n\a<>"
//function parses each input from read_line to make it readable for program            
char **split(char *line) {
  int pos = 0; int bsize = BUFF;
  char **tokens = malloc(bsize * sizeof(char*));
  char *token;

  //if nothing to tokenize, error message appears                                      
  if(!tokens) {
    write(STDERR_FILENO, "ERROR\n", 6);
    exit(EXIT_FAILURE);
  }

  //tokenizes users command                                                            
  token = strtok(line, TOK_D);
  while(token != NULL) {
    tokens[pos] = token;
    pos++;

    if(pos >= bsize) {
      bsize += BUFF;
      tokens = realloc(tokens, bsize * sizeof(char*));
      if(!tokens) {
        write(STDERR_FILENO, "ERROR\n", 6);
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, TOK_D);
  }
  tokens[pos] = NULL;
  return tokens;
}

//allows for program to take user input and process commands                           
int execute(char **args) {

  //checks if there is no comand                                                       
  if (args[0] == NULL) {
    return 1;
  }

  //allows for user command to be passed to start function                             
  for(int i = 0; i < built_nums(); i++) {
    if(strcmp(args[0], built_str[i]) == 0) {
      return (*built_func[i])(args);
    }
  }
  return start(args);
}

int cmdcd (char **args) { //command for changing directories                           
  if(args[1] == NULL) {
    write(STDIN_FILENO, "No such directory\n", 20);
  } else {
    if(chdir(args[1]) != 0) {
      write(STDIN_FILENO, "ERROR\n", 7);
    }
  }
  return 1;
}

//exit command                                                                         
int cmdexit(char **args) {
  return 0;
}

void prompt1(void) { //prompt for home directory                                       
  write(STDERR_FILENO, "1730sh:~$ ", 10);
}

void prompt2(void) { //prompt if directory is changed                                  
  write(STDERR_FILENO, "1730sh:", 7);
}

int main(int argc, char **argv) {

  char *line;
  char **args;
  int status;
  char hpath[BUFF];
  char temppath[BUFF];
  char newpath[BUFF];

  //loops program to allow for multiple user inputs                                    
  do {
    //checks which prompt to show user                                                 
    getcwd(hpath, sizeof(hpath));
    strcpy(temppath, hpath);
    getcwd(newpath, sizeof(newpath));
    if(strcmp(temppath, newpath)) { //if in home directory ~$ is printed               
      prompt1();
    }
    else { //if directory changed, file path is shown                                  
      prompt2();
      printf("%s$ ", newpath);
    }

    line = read_line();
    args = split(line);
    status = execute(args);

    free(line);
    free(args);
  } while(status);

  return EXIT_SUCCESS;
}
