#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>

#define BUFF 4096

/* builtins */
int cmdexit(char **args);
int cmdcd(char **args);

/* builtin tables */
char *built_str[] = {"exit", "cd"};
int (*built_func[])(char **) = {cmdexit, cmdcd};

int built_nums(void) {
    return sizeof(built_str) / sizeof(char *);
}

/* ---------------- REDIRECTION ---------------- */

void redir(char **args) {
    for (int i = 0; args[i] != NULL; i++) {

        /* input redirection */
        if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);

            /* remove < filename */
            for (int j = i; args[j + 2] != NULL; j++)
                args[j] = args[j + 2];
            args[i] = NULL;
            i--;
        }

        /* output redirection */
        else if (strcmp(args[i], ">") == 0) {
            int fd = creat(args[i + 1], 0644);
            if (fd < 0) {
                perror("creat");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            /* remove > filename */
            for (int j = i; args[j + 2] != NULL; j++)
                args[j] = args[j + 2];
            args[i] = NULL;
            i--;
        }
    }
}

/* ---------------- EXECUTION ---------------- */

int start(char **args) {
    pid_t pid = fork();
    int status;

    if (pid == 0) {  /* child */
        redir(args);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        perror("fork");
    }
    else {  /* parent */
        waitpid(pid, &status, 0);
    }
    return 1;
}

/* ---------------- INPUT ---------------- */

char *read_line(void) {
    char *line = NULL;
    size_t size = 0;

    if (getline(&line, &size, stdin) < 0) {
        free(line);
        exit(EXIT_SUCCESS);
    }
    return line;
}

#define TOK_D " \t\r\n\a"

char **split(char *line) {
    int pos = 0;
    int size = BUFF;
    char **tokens = malloc(size * sizeof(char *));
    char *token;

    if (!tokens) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOK_D);
    while (token) {
        tokens[pos++] = token;

        if (pos >= size) {
            size += BUFF;
            tokens = realloc(tokens, size * sizeof(char *));
            if (!tokens) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, TOK_D);
    }
    tokens[pos] = NULL;
    return tokens;
}

/* ---------------- COMMAND HANDLING ---------------- */

int execute(char **args) {
    if (args[0] == NULL)
        return 1;

    for (int i = 0; i < built_nums(); i++) {
        if (strcmp(args[0], built_str[i]) == 0)
            return (*built_func[i])(args);
    }
    return start(args);
}

/* ---------------- BUILTINS ---------------- */

int cmdcd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing operand\n");
    } else if (chdir(args[1]) != 0) {
        perror("cd");
    }
    return 1;
}

int cmdexit(char **args) {
    (void)args;
    return 0;
}

/* ---------------- PROMPT ---------------- */

void prompt(void) {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    printf("1730sh:%s$ ", cwd);
}

/* ---------------- MAIN ---------------- */

int main(void) {
    char *line;
    char **args;
    int status;

    do {
        prompt();
        line = read_line();
        args = split(line);
        status = execute(args);

        free(line);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}
