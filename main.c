// Tomás Child
// Sebastián Ossandón
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "header.h"

int main(int argc, char *argv[]){
	// Usar handler para manejar SIGINT
	siginterrupt(2, 1);
	signal(2, handler);
	
	// Inicializar valores para getopt y ejecutarlo
	int num_hijos = -1;
	int menu = 0;
	catch_getopt(argc, argv, &num_hijos, &menu);
	
	// Crear arreglo de pids de los hijos del padre
    pid_t pid[num_hijos];
	
    crear_hijos(pid, num_hijos, menu);
    // El padre espera a que se muestre la informacion sobre los hijos
   	pedir_signal(pid, num_hijos);

    return 0;
}
