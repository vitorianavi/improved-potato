#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

#define BUFFER_SIZE 64;
char **commands;

int found_bar(char *cwd, int *vet){
  int i,j= 0;
  for(i = 0; i < strlen(cwd);i++){
    if(cwd[i] == '/'){
      vet[j] = i;
      j++;
    }
  }
  return j;
}
char* simple_cwd(char *cwd){
  char *str  = (char*)malloc(sizeof(char)*1024);
  int vet[1024];
  int i = 0,n;
  int j = found_bar(cwd,vet);
//  printf("%d\n", j);
  if(j > 1){
   n = vet[j-1]+1;
   while(cwd[n] != '\0'){
     str[i] = cwd[n];
     i++;
     n++;
   }
   return str;
  }
 if(strcmp(cwd, "/home") == 0){
   str[0] = '~';
   return str;
 }
 return cwd;
}

char* current_dir(){
  char *cwd,*str;
  size_t allocSize = sizeof(char) * 1024;
  cwd = (char*)malloc(allocSize);
  str = (char*)malloc(allocSize);
  if (getcwd(cwd, allocSize) != NULL){
        strcpy(str, simple_cwd(cwd));
        return str;
  }
  else{
       perror("getcwd() error");
  }
  return NULL;
}

char* shell_name(){
  char host[1024]= "";
  char *user = getenv("USER");
  char *prompt = (char*)malloc(sizeof(char)*1024);
  char cwd[1024];
  strcpy(cwd,current_dir());
  gethostname(host,sizeof(host));
  prompt[0] = '[';
  strcat(prompt,user);
  strcat(prompt,"@");
  strcat(prompt,host);
  strcat(prompt," ");BUFFER_SIZE
  strcat(prompt,cwd);
  strcat(prompt,"]");
  strcat(prompt,"$");
  strcat(prompt," ");
  return prompt;
}

void read_all_commands() {
  FILE *file;
  int index = 0;
  char aux_comm[64];
  file = fopen("all_commands", "r");

  while(fprintf(file, "%s\n",aux_comm)==1){
    strcpy(commands[index], aux_comm);
    index++;
  }
  fclose(file);
}


char* cmd_generator(const char *text, int state){
	int index = 0, len;
	char *com_result;

	if (!state) {
		len = strlen(text);
	}

  com_result = commands[index];
	while (com_result) {
		if (strncmp(com_result, text, len) == 0) {
			return strdup(com_result);
		}
    index++;
    com_result = commands[index];
	}

	return NULL;
}

char** completion(const char *text, int start, int end){
	rl_attempted_completion_over = 0;
	return rl_completion_matches(text, cmd_generator);
}


void read_line(char *cmd){
 rl_attempted_completion_function = completion;
  char *line;
  line = readline(shell_name());
	if (line != NULL && line[0] != 0){
		add_history(line);
	}

  if(line) {
    strcpy(cmd, line);
    free(line);
  }
}

char **split_line(char *line, int *n_tokens) {
    int buffer_size = BUFFER_SIZE;
    int pos = 0;
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
    char dir_home[1024];
    strcpy(dir_home,"/home/");
  	strcat(dir_home,getenv("LOGNAME"));

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
        if(path[0] == '~'){
          memmove(&path[0], &path[0+ 1], strlen(path) - 0);
          strcat(dir_home,path);
          strcpy(path,dir_home);
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
void cmd_pipe(char **tokens, int n_tokens, int index_pipe){
  int pipefd[2], i, j;
//  pid_t pid1, pid2;

  char **cmd1, **cmd2;
  cmd1 = (char**)malloc(sizeof(char*)*64);
  cmd2 = (char**)malloc(sizeof(char*)*64);
  for(i = 0; i < index_pipe;i++){
    cmd1[i] = tokens[i];
  }
  for(i = index_pipe+1, j = 0; i < n_tokens; i++, j++ ){
    cmd2[j] = tokens[i];
  }

  pipe(pipefd);
  //pid1 = fork();
  switch (fork()) {
    case -1:
      printf("Erro no fork\n");
      break;
    case 0:
      close(1);
      dup(pipefd[1]);
      close(pipefd[1]);
      close(pipefd[0]);
      execvp(cmd1[0], cmd1);
      break;
    default:
      close(0);
      dup(pipefd[0]);
      close(pipefd[0]);
      close(pipefd[1]);
      execvp(cmd2[0], cmd2);
      break;
  }
}

int found_pipe(char **tokens, int n_tokens){
  int i;

  for(i = 0; i < n_tokens; i++){
      if(strcmp(tokens[i],"|") == 0){
        return i;
      }
  }

  return -1;
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
    int n_tokens=0;
    int i, index_pipe;
    char last_dir[1024];
    char *cwd = (char*)malloc(sizeof(char)*1024);
    last_dir[0]=0;

    commands = (char**) malloc(sizeof(char*)*4000);
    for (i = 0; i < 4000; i++) {
        commands[i] = (char*) malloc(sizeof(char)*64);
    }


    read_all_commands();

    while(1) {
        cwd = current_dir();
        read_line(buffer);
        tokens = split_line(buffer, &n_tokens);
        index_pipe = found_pipe(tokens,n_tokens);
    //    printf("%s\n", tokens[0]);

        if(strcmp(tokens[0], "exit")==0) break;
        if(index_pipe != -1){
          cmd_pipe(tokens,n_tokens, index_pipe);
        }else if(strcmp(tokens[0], "cd")==0) {
            cmd_cd(tokens[1], last_dir);
        } else if(strcmp(tokens[0], "ls")==0) {
            if(n_tokens>1)
                cmd_ls(tokens[1]);
            else cmd_ls(".");
        } else if(strcmp(tokens[0], "pwd")==0) {
            //printf("%s\n", cwd);
            getcwd(cwd, sizeof(char) * 1024);
            printf("%s\n", cwd);
        } else if(n_tokens>1) {
            execute_call(tokens);
        }


        n_tokens=0;
    }
}
