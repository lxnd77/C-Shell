/* Made by Rishabh Bhargava, - CMPT 300 - prof: Arrvindh Shriraman - 301243452*/
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HISTORY_DEPTH 10
#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)

typedef struct History_Stack
{
    char history[HISTORY_DEPTH][COMMAND_LENGTH];
    int counter;
    int round;

}History_Stack;

History_Stack HS;

void handle_SIGINT(int Sig);
void addToHistory(History_Stack *HS, char *buffer);
void printHistory(History_Stack *HS);
int read_command(char *buff, char *tokens[], _Bool *in_background);

int main(int argc, char* argv[])
{
    


    char input_buffer[COMMAND_LENGTH]="";
    char *tokens[NUM_TOKENS]={};
    char *resetter[NUM_TOKENS];
    int howMany=0;
    
    while (true) {
        
        // Get command
        // Use write because we need to use read()/write() to work with
        // signals, and they are incompatible with printf().
        
        signal(SIGINT,handle_SIGINT);
        
        char buff[COMMAND_LENGTH];
        getcwd(buff,COMMAND_LENGTH);
        strcat(buff,">");
        write(STDOUT_FILENO, buff, strlen(buff));
         memset(buff,0,COMMAND_LENGTH);
         
        _Bool in_background = false;
        
        howMany=read_command(input_buffer, tokens, &in_background);    
        if(howMany!=0)
        {
            if(!strcmp(tokens[0],"exit"))
            {
                exit(0);
            }
            else if(!strcmp(tokens[0],"history"))
            {
                printHistory(&HS);
            }
            
            else if(!strcmp(tokens[0],"pwd"))
            {
                char tempbuff[COMMAND_LENGTH];
                getcwd(tempbuff,COMMAND_LENGTH);
                if(tempbuff==NULL)
                {
                    perror("Could not get Working Directory");
                }
                else
                {
                    write(STDOUT_FILENO, tempbuff, strlen(tempbuff));
                }
            }
            else if(!strcmp(tokens[0],"cd"))
            {
                int res=chdir(tokens[1]);
                if((res!=0)&&(errno!=0))
                {
                    perror("Some error occured while calling chdir");
                }
            }
            else
            {
                memset(resetter,0,COMMAND_LENGTH);
                for(int i=0;i<howMany;i++)
                {
                    resetter[i]=tokens[i];
                }
                
                int pid=fork();
                if(pid==0)
                {
                    execvp(resetter[0],resetter);
                    if(errno!=0)
                    {
                        perror("Child could not be created");
                    }
                    exit(0);
                }
                else
                {
                    
                    if((in_background==0)&&(errno==0))
                    {
                        waitpid(pid,NULL,0);
                    }
                    else
                    {
                        //do nothing
                    }
                }
                
            }
        }
        else
        {
            //do nothing
        }
        
         
    }
    
    while( waitpid(-1,NULL,WNOHANG)>0)
                    ;
    return 0;
}

void addToHistory(History_Stack *HS, char *buffer)
{
    if(HS->round==10)
    {
        HS->round=9;
        for(int i=0;i<9;i++)
        {
            strcpy(HS->history[i],HS->history[i+1]);
        }
        memset(HS->history[9],0,COMMAND_LENGTH);
        strcpy(HS->history[9],buffer);
        HS->round++;
        HS->counter++;
        
    }
    else
    {  
        strcpy(HS->history[HS->round],buffer);
        HS->round++;
        HS->counter++;
    }
}
void printHistory(History_Stack *HS)
{
    char s[20]="";

    if(HS->counter>HS->round || HS->counter==10)
    {
        for(int i=0;i<10;i++)    
        {
    
            sprintf(s,"%d",HS->counter-HS->round+i);
            write(STDOUT_FILENO, s, strlen(s));
            write(STDOUT_FILENO,"\t",2);
            write(STDOUT_FILENO,HS->history[i],strlen(HS->history[i]));
            write(STDOUT_FILENO,"\n",2);
        }
    }
    else
    {
        for(int i=0;i<HS->round;i++)    
        {
            sprintf(s,"%d",i+1);
            write(STDOUT_FILENO, s, strlen(s));
            write(STDOUT_FILENO,"\t",2);
            write(STDOUT_FILENO,HS->history[i],strlen(HS->history[i]));
            write(STDOUT_FILENO,"\n",2);
        }
    }
}
char * accessHistory(char *buffer,History_Stack *HS)
{
    int index=atoi(buffer);
    index=index-1;
    if(index < HS->counter - HS->round || index >= HS->counter)
    {
        return "";
    }
    //char C[COMMAND_LENGTH]="";
    strcpy(buffer,HS->history[(HS->counter - HS->round)+index]);
    return buffer;
}
void handle_SIGINT(int Sig)
{
    if(Sig==SIGINT)
    {
        write(STDOUT_FILENO,"\n",strlen("\n"));
        printHistory(&HS);    
        char buff[COMMAND_LENGTH];
        getcwd(buff,COMMAND_LENGTH);
        strcat(buff,">");
        write(STDOUT_FILENO, buff, strlen(buff));
    }
}

int read_command(char *buff, char *tokens[], _Bool *in_background)
{
    
    *in_background=0;
      
    if(!(buff)) //check for no buffer
    {
        perror("No Buffer found \n");
        exit(-1);
    }
     
    if(!(tokens)) //check for no tokens
    {
        perror("No Tokens Array found \n");
        exit(-1);
    }
    
    buff=fgets(buff,COMMAND_LENGTH-1,stdin); //get line from stdin
    if(errno==EINTR)
        {
            exit(0);
        }
        
    if(strcmp(buff,"\n")==0)  //check if no line before '\0'
    {
        return 0;
    }
    if(buff[0]=='!')
    {
        buff=buff+1;
        buff=accessHistory(buff,&HS);
        if(strcmp(buff,"")==0)
        {
            errno=ESPIPE;
            perror("Invalid Operation");
            return 0;
        }
    }
    
    addToHistory(&HS,buff);
    
    buff[strlen(buff)-1]='\0';
    int i=0;    //counter for how many tokens (will be returned )
    buff=strtok(buff," \n\t"); //first token
   
    while(buff!=NULL)          //if not empty
    {
        
        int cc=strlen(buff);
        
        if((cc>=2)&&(strcmp((buff+cc-1),"&")==0)) //if token is bigger than 1 byte ends with '&'
        {
            *in_background=1;               //set as open in background
            buff[cc-1]='\0';
            tokens[i]=buff;
            i++;            
            return i;
        }
        
        else
        {
            tokens[i]=buff;
            i++;
            buff=strtok(NULL," \n\t");
        }
    }
    
    if(i>1&&strcmp(tokens[i-1],"&")==0)
    {
        *in_background=1;
        tokens[i-1]=0;
    }
    
    return i;
}




