#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>

int catch_getopt(int argc, char *argv[], int *num_hijos, int *menu);
void crear_hijos(pid_t padre, pid_t *pid, int num_hijos, int menu);
void pedir_signal(pid_t padre, pid_t *pid, int num_hijos);
int *parsear(int *input);
void esperar_signal(int n);
void handler(int nulo);
// Flag para capturar la señal SIGINT
static volatile int handler_flag = 0;


int main(int argc, char *argv[]){
	// Usar handler para manejar SIGINT
	siginterrupt(2, 1);
	signal(2, handler);
	
	// Inicializar valores para getopt y ejecutarlo
	int num_hijos = -1;
	int menu = 0;
	catch_getopt(argc, argv, &num_hijos, &menu);
	
	// Guardar pid del padre para identificarlo
	// Crear arreglo de pids de los hijos del padre
    pid_t padre = getpid();
    pid_t pid[num_hijos];
	
    crear_hijos(padre, pid, num_hijos, menu);
    // El padre espera a que se muestre la informacion sobre los hijos
   	pedir_signal(padre, pid, num_hijos);

    return 0;
}

// Captura los parámetros de lanzamiento del programa y decide
// cuántos hijos crear, y si muesta información sobre ellos con la flag -m
int catch_getopt(int argc, char *argv[], int *num_hijos, int *menu){
	int opcion = 0, opterr = 0;

    while((opcion = getopt(argc, argv,"h:m")) != -1){
        switch(opcion) {
             case 'h':
                *num_hijos = atoi(optarg);
                break;
             case 'm':
                *menu = 1;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
	
	// Condiciones para creación de hijos
    if((int) *num_hijos == -1){
        printf("Debe ingresar un número de hijos con -h NUM_HIJOS.\n");
        exit(1);
    }
	if((int) *num_hijos == 0){
		printf("Debe crear al menos un hijo.\n");
		exit(1);
	}
	
	return (int) *num_hijos;
}

// Esta función crea tantos hijos como se reciba en el parámetro -h
// Recibe como parámetros el pid del padre, el arreglo de pids hijos
// el número de hijos deseados y la flag booleana de -m
void crear_hijos(pid_t padre, pid_t *pid, int num_hijos, int menu){
    int i;

    for(i = 0; i < num_hijos; i++){
        pid[i] = fork();
        if(pid[i] < 0){
            perror("Error en el fork.");
            exit(1);
        // Si el pid es 0, es un hijo
		// también verifica que los hijos vengan del mismo padre
        } else if(pid[i] == 0 && getppid() == padre) {
            if(menu == 1){
                printf("Número: %d   ,pid: %d\n", i+1, getpid());
            }
            // Todos los hijos ejecutan esta función
            // Lo que hará, será ponerlos a esperar las señales del padre
            esperar_signal(i);
        }
    }
}

// Esta función es la encargada de manejar el envío de señales por stdin
// Parámetros: pid del padre, arreglo de pids hijos y el número de hijos
void pedir_signal(pid_t padre, pid_t *pid, int num_hijos){
    int estado;
    // Si estamos en el padre
    if(getpid() == padre){
        while(1){
			//sleep(1);
			// Si la flag indica que se presionó una vez CTRL+C, se
			// devuelve SIGINT a su función estándar
			if(handler_flag == 1){
				signal(2, SIG_DFL);
				int i;
				for(i = 0; i < num_hijos; i++){
					if(pid[i] >= 0)
						printf("<Soy el hijo con pid: %d, y estoy vivo aún>\n", pid[i]);
				}
				// Aumentar flag para que ya no entre al if
				handler_flag--;
			}
			// Pedir hijo - señal por consola
			sleep(1);
            printf("Ingresar número de hijo y señal a enviar (X - Y):\n");

            int input[2];
            // Parsear el string de entrada para obtener los valores X, Y
			if(!parsear(input)){
				continue;
			}
			// Los valores obtenidos son: x = Nº de hijo; y = Señal
			int x = input[0] - 1, y = input[1];
			// Verificar condiciones de borde para nº de hijo
            if(x < 0 || x > num_hijos - 1){
                printf("Número de hijo inválido.\n");
                continue;
            }
			
            if(pid[x] < 0){
                printf("Ese hijo ya está muerto!\n");
            } else {
				// Si el hijo que recibe la señal está vivo, mandar señal
                kill(pid[x], y);
                // Si la señal es SIGTERM, hacer que el padre espere al hijo
				if(y == 15){
					waitpid(pid[x], &estado, 0);
					printf("Maté al hijo con PID: %d.\n", pid[x]);
					// Actualizar valor en arreglo de pids hijos
					pid[x] = -1;
				}
            }        
        }
    }
}

// Recibe la entrada X - Y y captura los valores X e Y
int *parsear(int *input){
	char entrada[256];
	fgets(entrada, sizeof(entrada), stdin);
	const char separador[3] = "- ";	
	int i = 0;
	// Obtener primer token
	char *token = strtok(entrada, separador);
	// Obtener los demás tokens
	while(token != NULL) {
		input[i++] = atoi(token);
		token = strtok(NULL, separador);
	}

	if(input[1] != 15 && input[1] != 16 && input[1] != 17){
		printf("Debe ingresar una orden válida.\n");
		return NULL;
	}
	return input;
}

// Esta función hace que los hijos permanezcan escuchando señales
// mientras se ejecutan, y además edita las funciones de las señales
// 16 y 17, que corresponden a SIGUSR1 y SIGUSR2
// Recibe como parámetro el arreglo de pids hijos y el número
// de hijo al que se envía la señal
void esperar_signal(int n) {
	// Declara una variable sigset_t para crear un conjunto de señales
	// que se van a "enmascarar" para poder cambiar su funcionamiento
    sigset_t sigset;
    // Inicializar sigset, se requiere para usar sigaddset()
    sigemptyset(&sigset);
    // Agregar SIGUSR1 y SIGUSR2 al conjunto de señales a enmascarar
    sigaddset(&sigset, 16);
	sigaddset(&sigset, 17);
	// Esta función enmascara las señales 16 y 17, que están en sigset
    sigprocmask(SIG_SETMASK, &sigset, NULL);
 
    int contador = 0;
	int resultado;
    while(1){
		if(handler_flag == 1){
			signal(2, SIG_DFL);
		}
		// sigwaitinfo espera a que las señales de sigset sean enviadas
		// si ocurre, devuelve el número de señal capturada, si no, -1
        if ((resultado = sigwaitinfo(&sigset, NULL)) > 0){
			// Si se envió la señal SIGUSR1, imprimir mensaje
            if(resultado == 16){
				printf("<Tengo pid: %d, y he recibido esta llamada %d veces>\n", getpid(), ++contador);
			// Si se envió SIGUSR2, el hijo crea a un hijo propio
			} else if(resultado == 17){
				fork();
			}
		}
	}
}

// Esta función maneja lo que hace SIGINT
// en este caso, activa un flag para avisar que se presionó CTRL+C
void handler(int nulo) {
    handler_flag = 1;
}