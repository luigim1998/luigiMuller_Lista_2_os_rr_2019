/**
 * @file   TODO_USER.c
 * @author Luigi Muller
 * @date   11 June 2019
 * @version 0.1
 * @brief  A user space program to support a task list that will run as an LKM.
 * This code is based on Derek Molloy's code.
 * @see http://www.derekmolloy.ie/ for the base code.
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 1200              ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

int main(){
   int ret, fd;
   char stringToSend[256];
   printf("Comecando o codigo de teste do dispositivo...\n");
   fd = open("/dev/TODO_LIST", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Falha em abrir dispositivo...");
      return errno;
   }
   printf("Pressione ENTER para as cinco tarefas serem inseridas no LKM:\n");
   getchar();

   printf("Escrevendo as tarefas no dispositivo.\n");
   ret = write(fd, NULL, 0); // Send the string to the LKM
   if (ret < 0) {
      perror("Falha em escrever no dispositivo.\n");
      return errno;
   }

   printf("Pressione ENTER para ler as tarefas a fazer...\n");
   getchar();

   printf("Lendo do dispositivo...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Falha em ler a mensagem do dispositivo.");
      return errno;
   }
   printf("Mensagem do dispositivo: [%s]\n", receive);
   printf("Fim do programa, TCHAU\n");
   return 0;
}