/*
Lucas Stefan Abe - 8531612
Mathias Van Sluys Menck - 4343470
*/

#include <stdio.h> /* printf, readline*/
#include <stdlib.h> /* malloc, free*/
#include <unistd.h> /* getcwd, getuid, fork, execve*/
#include <string.h> /* strcmp, strtok */
#include <errno.h> /* getcwd */
#include <readline/readline.h> /* readline */
#include <readline/history.h>/* add_history */
#include <sys/stat.h> /* chmod */
#include <sys/types.h> /* getuid */
#include <sys/wait.h> /* waitpid */

/* passa a string do diretório, constroi e passa a string do promp para o vetor de strings *strings[]*/
void diretorio_atual (char *strings[]) {
    char *dir = getcwd (0, 0);
    char *prompt;
    int n, i;

    for (n = 0; dir[n] != '\0'; n++);
    prompt = malloc ((n+5)*sizeof (char));
    prompt[0] = '(';
    prompt[n+1] = ')';
    prompt[n+2] = ':';
    prompt[n+3] = ' ';
    prompt[n+4] = '\0';
    for (i = 0; i < n; i++) prompt[i+1] = dir[i]; 
    strings[0] = dir;
    strings[1] = prompt;
}

/* separa a linha em tokens com espaço como delimitador e manda para *parameters[]*/
void tokens (char *line, char *parameters[]) {
    char *token;
    int i;

    token = strtok (line, " ");
    for (i = 0; token != NULL; i++) {
        parameters[i] = token;
        token = strtok (NULL, " ");
    }
    parameters[i] = NULL;
}

int main (int argc, char *argv[]) {
    int status, n;
    char *parameters[100], *strings[2];
    pid_t pid;
    char *dir, *prompt;
    int i, j;
    diretorio_atual (strings);
    dir = strings[0];
    prompt = strings[1];

    while (1) {
        /*captura linha digitada pelo usuário*/
        char *line = readline (prompt);
        /* adiciona linha no histórico do shell*/
        if (line == NULL || strcmp (line, "") == 0)
            continue;
        add_history (line);

        tokens (line, parameters);
        if (strcmp (parameters[0], "chmod") == 0) {
            int size1 = strlen (dir);
            int size2 = strlen (parameters[2]);
            char *file_path = malloc ((size1 + size2 + 5)*sizeof (char));
            long int mode = strtol (parameters[1], 0, 8);

            for (i = 0; dir[i] != '\0'; i++) file_path[i] = dir[i];
            file_path[i++] = '/';
            for (j = 0; parameters[2][j] != '\0'; j++) file_path[i+j] = parameters[2][j];
            file_path[i+j] = '\0';
            chmod (file_path, mode);
            free (file_path);
        }
        else if (strcmp (parameters[0], "id") == 0) {
            uid_t id = getuid ();
            printf("%d\n", id);
        }
        else {
            if ((pid = fork ()) != 0) {
                /* Codigo do pai */
                waitpid (-1,&status,0);
            } else {
                /* Codigo do filho */
                execve (parameters[0],parameters,0);
            }
        }
    }
    free (dir);
    free (prompt);
}
