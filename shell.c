#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <ncurses.h>

#define BUFFER_SIZE 64;

void read_line(char buffer[]) {
    int position = 0;
    int character;

    while(1) {
        // Lê o caractere
        character = getchar();

        if (character == '\033') { // if the first value is esc
            getchar(); // skip the [
            switch(getchar()) { // the real value
                case 'A':
                    // code for arrow up
                    break;
                case 'B':
                    // code for arrow down
                    break;
                case 'C':
                    // code for arrow right
                    printf("alo\n");
                    break;
                case 'D':
                    // code for arrow left
                    break;
            }
        }

        if (character == EOF || character == '\n') {
            buffer[position] = '\0';
            //strcpy(buffer, line);
            break;
        } else {
            buffer[position] = character;
        }
        position++;
    }
    /*char *line = NULL;
    ssize_t buffer_size = 0;
    // getline faz o processo de alocação e realocação do buffer, dependendo do tamanho
    getline(&line, &buffer_size, stdin);

    return line;*/
}

char **split_line(char *line, int *n_tokens) {
    int buffer_size = BUFFER_SIZE;
    int pos = 0, len;
    char *token, **tokens;

    // TODO: consertar esse memory leak
    tokens = (char**)malloc(buffer_size * sizeof(char*));
    // strtok aloca e realoca de acordo com o tamanho da string encontrada
    token = strtok(line, " \t");
    while(token != NULL) {
        tokens[pos] = token;
        (*n_tokens)+=1;
        pos++;

        // se atingiu o tamanho máximo do buffer, realoca o vetor de strings
        if(pos >= buffer_size) {
            buffer_size += BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
        }

        // próximo termo do comando
        token = strtok(NULL, " \t");
    }
    tokens[pos] = NULL;

    return tokens;
}

void get_current_dir(char *current_dir) {
    char tmp[1024];
    if(getcwd(tmp, sizeof(tmp)) != NULL) {
        strcpy(current_dir, tmp);
    } else {
        perror("getcwd() error");
    }
}

void cmd_cd(char *path, char *last_dir) {
    char tmp[1024];

    //pega o diretório atual para salvar no cache
    get_current_dir(tmp);

    if(path[0] != 0) {
        // Comando "cd -" para voltar para diretório anterior
        if(path[0]=='-') {
            if(last_dir[0] != 0) {
                strcpy(path, last_dir);
            } else {
                // Shell acabou de ser iniciado e não há diretório mais recente acessado
                printf("Hit end of history...\n");
                return;
            }
        }

        // Acessando diretório
        if(chdir(path) != 0) {
        //    perror("lsh");
            printf("cd: The directory “%s” does not exist.\n", path);
        } else {
            strcpy(last_dir, tmp);
        }
    }
}

void cmd_ls(char *path) {
    DIR *d;
    struct dirent *dir;

    d = opendir(path);
    if(d) {
        dir = readdir(d);
        while(dir != NULL) {
            // se for um diretório
            if(dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                printf("%s/ ", dir->d_name);
            } //se for um arquivo normal
            else if(dir->d_type == DT_REG) {
                printf("%s ", dir->d_name);
            } //se for um link simbólico
            else if(dir->d_type == DT_LNK) {
                printf("%s ", dir->d_name);
            }
            dir = readdir(d);
        }
        closedir(d);
        printf("\n");
    } else {
        printf("ls: The directory “%s” does not exist.\n", path);
    }
}

int execute_call(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
    // chama o processo filho que irá executar o comando
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // erro ao chamar processo filho
        perror(args[0]);
    } else {
        // Faz o processo pai esperar pela execução do processo filho
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int main() {
    char buffer[2048], **tokens;
    int i, n_tokens=0, len;

    char last_dir[1024], current_dir[1024];
    last_dir[0]=0;

    while(1) {
        get_current_dir(current_dir);
        printf("%s> ", current_dir);
        read_line(buffer);
        tokens = split_line(buffer, &n_tokens);

        if(strcmp(tokens[0], "exit")==0) break;

        if(strcmp(tokens[0], "cd")==0) {
            cmd_cd(tokens[1], last_dir);
        } else if(strcmp(tokens[0], "ls")==0) {
            if(n_tokens>1)
                cmd_ls(tokens[1]);
            else cmd_ls(".");
        } else if(strcmp(tokens[0], "pwd")==0) {
            printf("%s\n", current_dir);

        } else if(n_tokens>1) {
            execute_call(tokens);
        }

        n_tokens=0;
    }
}
