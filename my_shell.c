#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<signal.h>
#include <stdbool.h>
// #include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
*
*/

int fg_proc_pid = 0;
bool sig_int_called = 0;

void sig_int_handler(int s) {
	// printf("%d\n", fg_proc_pid);
	kill(fg_proc_pid, 9);
	sig_int_called = 1;
	printf("$\n");
}

char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}


int main(int argc, char* argv[]) {
	char  line1[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;
	int bg_process_cnt = 0;
	int bg_processes[64];

	signal(SIGINT, sig_int_handler);

	while(1) {

		int background = 0;
		/* BEGIN: TAKING INPUT */
		bzero(line1, sizeof(line1));
		printf("$ ");
		scanf("%[^\n]", line1);
		getchar();
		
		char *tok1 = line1;
		char *line2;

		/*do-while because at least one iteration will happen*/
		while (tok1 != NULL) {
			if(sig_int_called) {
				break;
			}
			
			line2 = tok1;
			tok1 = strsep(tok1, " && "); // preparing tok for the next iteration

			char *tok2 = tok1;
			char *line;

			if(tok1 != NULL) {
				tok1[0] = '\0';
				tok1 += 4;
			}

			int fg_proc_count = 0;

			while(tok2 != NULL) {
				if(sig_int_called) {
					break;
				}

				line = tok2;
				tok2 = strsep(tok2, " &&& ");
				fg_proc_count++;

				if(tok2 != NULL) {
					tok2[0] = '\0';
					tok2 += 5;
				}

				// printf("Command entered: %s (remove this debug output later)\n", line);
				/* END: TAKING INPUT */
				background = (line[strlen(line)-1] == '&');
				// printf(line);

				if(strcmp("exit", line) == 0) {
					printf("exitting\n");
					while(bg_process_cnt != 0) {
						bg_process_cnt--;
						if(bg_processes[bg_process_cnt] > 0) {
							kill(bg_processes[bg_process_cnt], 9);
							printf(" Shell: Background process killed %d\n", bg_processes[bg_process_cnt]);
						}
					}
					
					exit(0);
				}
				
				if(background) {
					line[strlen(line)-1] = '\0';
				}

				line[strlen(line)] = '\n'; //terminate with new line
				
				tokens = tokenize(line);

				//do whatever you want with the commands, here we just print them

				// for(i=0;tokens[i]!=NULL;i++){
				// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
				// }

				if(strlen(line) == 1) {
					continue;
				}

				/*There is a reason why I chose this place to put this periodic check block*/

				for(i=0; i<bg_process_cnt; i++) {
					if(bg_processes[i] == -1) continue;
					int ret = waitpid(bg_processes[i], NULL, WNOHANG);
					if(ret != 0) {
						printf(" Shell: Background process finished %d\n", bg_processes[i]);
						bg_processes[i] = -1;
					}
				}

				if(strcmp("cd", line) != 0) {
					int pid = fork();

					if(pid == 0) {
						if(background) setpgid(0, 0);
						execvp(tokens[0], tokens);
					} else {
						if(background) {
							bg_processes[bg_process_cnt] = pid;
							bg_process_cnt++;
							line[strlen(line)-1] = '&';
						} else {
							fg_proc_pid = pid;
							wait(NULL);
						}
					}
				} else if(strcmp("cd", line) == 0) {
					printf(tokens[1]);
					chdir(tokens[1]);
				}

				// Freeing the allocated memory	
				for(i=0;tokens[i]!=NULL;i++){
					free(tokens[i]);
				}
				free(tokens);

			}
		}	
	}
	return 0;
}
