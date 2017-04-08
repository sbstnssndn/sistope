
/*DECLARACIÓN DE FUNCIONES*/

void esperar_signal();
int catch_getopt(int argc, char *argv[], int *num_hijos, int *menu);
void crear_hijos(pid_t *pid, int num_hijos, int menu);
void pedir_signal(pid_t *pid, int num_hijos);
int *parsear(int *input);
void handler(int nulo);