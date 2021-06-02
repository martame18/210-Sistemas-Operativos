/*------------------------------------------------------------------------------
Proyecto Shell de UNIX. Sistemas Operativos
Grados I. Inform�tica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Algunas secciones est�n inspiradas en ejercicios publicados en el libro
"Fundamentos de Sistemas Operativos", Silberschatz et al.

Para compilar este programa: gcc ProyectoShell.c ApoyoTareas.c -o MiShell
Para ejecutar este programa: ./MiShell
Para salir del programa en ejecuci�n, pulsar Control+D
------------------------------------------------------------------------------*/

#include "ApoyoTareas.h" // Cabecera del m�dulo de apoyo ApoyoTareas.c
 
#define MAX_LINE 256 // 256 caracteres por l�nea para cada comando es suficiente
#include <string.h>  // Para comparar cadenas de cars. (a partir de la tarea 2)

// --------------------------------------------
//                     MAIN          
// --------------------------------------------

// manejador de la señal SIGCHLD, que trata los comandos lanzados en segundo plano
void manejador(int segnal){
    job * auxi;
    enum status estado; 
    int infoTarea, statusTarea, waitTarea;
    while (list_size(listatareas) > 0){   //mientras que la lista de tareas no esté vacía la recorremos
        for (int i=list_size(listatareas); i>=0; i--){ 
            auxi = get_item_bypos(listatareas, i);

            waitTarea = waitpid(auxi->pgid, &statusTarea, WNOHANG);  // comprobamos si la tarea ha terminado
            if (auxi->pgid == waitTarea){ //si la tarea ha terminado, imprimimos un mensaje y la borramos de la lista de tareas
                if (auxi->ground == PRIMERPLANO){
                    printf("Comando %s ejecutado en primer plano con PID %i ha finalizado.\n", auxi->command, auxi->pgid);
                } else{
                    printf("Comando %s ejecutado en segundo plano con PID %i ha finalizado.\n", auxi->command, auxi->pgid);
                }
                delete_job(listaTareas, auxi);
            } else{
                waitTarea = waitpid(auxi->pgid, &statusTarea, WUNTRACED);  // comprobamos si la tarea se ha suspendido
                if (auxi->pgid == waitTarea){ //si la tarea se ha suspendido, imprimimos un mensaje y la pasamos a DETENIDO
                    if(auxi->ground == PRIMERPLANO){
                        printf("Comando %s ejecutado en primer plano con PID %i se ha suspendido.\n", auxi->command, auxi->pgid);
                    } else{
                        printf("Comando %s ejecutado en segundo plano con PID %i se ha suspendido.\n", auxi->command, auxi->pgid);
                    }
                    auxi->ground = DETENIDO;
                } else{
                    waitTarea = waitpid(auxi->pgid, &statusTarea, WCONTINUED);  // comprobamos si la tarea se ha reanudado
                    if (auxi->pgid == waitTarea){ //si la tarea se ha reanudado, imprimimos un mensaje
                        if(auxi->ground == PRIMERPLANO){
                            printf("Comando %s ejecutado en primer plano con PID %i ha reanudado su ejecución.\n", auxi->command, auxi->pgid);
                        } else{
                            printf("Comando %s ejecutado en segundo plano con PID %i ha reanudado su ejecución.\n", auxi->command, auxi->pgid);
                        }
                    }
                }
            }
        }
    }
}

// método para comprobar si un comando es interno de mishell o no: 1 si interno, 0 si externo
int esinterno(char args0){
    if (args0 == "cd" || args0 == "logout") return 1;
    else return 0;
}
// método para ejecutar un comando interno de mi shell
void leercomando(char args[]){  // comandos internos que puede ejecutar: cd, logout, jobs
    if (strcmp(args[0], "cd") == 0){   // si el comando introducido es "cd"
            /* getenv("patata") -> busca entre la lista de variables de entorno que podría ser "patata" y devuelve el puntero exacto de dicha variable */
        if (args[1] == NULL) chdir(getenv("HOME")); // si no se introduce un directorio como parámetro, se cambia al directorio por defecto
        else chdir(args[1]);
    } else if (strcmp(args[0], "logout") == 0) exit();  // si el comando introducido es "logout"
    else if (strcmp(args[0], "jobs") == 0){  // si el comando introducido es "jobs"
        block_SIGCHLD();
            if(list_size(listatareas)>0){
		        print_job_list(listatareas);  // si hay tareas las imprime por pantalla
	        }else{
		        printf("No hay tareas pendientes\n");  // si no hay tareas lo indica
	        }
        unblock_SIGCHLD();
    } else if (strcmp(args[0], "bg") == 0)){
        bg(args[1]);
    }
}

// Permite pasar a segundo plano un comando que se encuentre suspendido. pos es el lugar que ocupa en la listatareas
void bg(char pos){
    job * auxi;
    if (pos == NULL) pos = 0;  // si no se introduce argumento, se utiliza la primera posición de la lista
    if (pos < 0 || pos > list_size(listatareas)){  // si el argumento introducido no entra en los límites de la lista, se indica por pantalla
        printf("Esta tarea no existe.");
    } else{  // si el argumento es válido
        auxi = get_item_bypos(listatareas, pos);
        auxi->ground = SEGUNDOPLANO;  // pasa el argumento de DETENIDO a SEGUNDOPLANO
        killpg(auxi->pgid, SIGCONT);
    }
}

int main(void)
{
      char inputBuffer[MAX_LINE]; // B�fer que alberga el comando introducido
      int background;         // Vale 1 si el comando introducido finaliza con '&'
      char *args[MAX_LINE/2]; // La l�nea de comandos (de 256 cars.) tiene 128 argumentos como m�x
                              // Variables de utilidad:
      int pid_fork, pid_wait; // pid para el proceso creado y esperado
      int status;             // Estado que devuelve la funci�n wait
      enum status status_res; // Estado procesado por analyze_status()
      int info;		      // Informaci�n procesada por analyze_status()

      while (1) // El programa termina cuando se pulsa Control+D dentro de get_command()
      {   		
        ignore_terminal_signals();   // ignorar señales del terminal
        signal(SIGCHLD, manejador);  // activa el manejador
        listatareas = new_list("Lista_Tareas");  //crea la lista de tareas en segundo plano, suspendidas y detenidas.
        printf("COMANDO->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); // Obtener el pr�ximo comando
        if (args[0]==NULL) continue; // Si se introduce un comando vac�o, no hacemos nada

        //   Aquí empieza la acción
        //Si es un comando interno
        if (esinterno(args[0]) == 1){  
            leercomando(args);    //ejecuta un comando interno
            continue;
        }
        //Si es un comando externo
        pid_fork = fork();    //crea un proceso hijo
        if (pid_fork == 0){   
            //código del hijo
            printf("Hola! soy el hijo:)\n");
            gpid = new_process_group(getpid());  //gpid = pid del grupo de procesos (que surgen del hijo)
            if (!background)   set_terminal(gpid);  //si el proceso se ejecuta en primer plano => acapara el terminal
            restore_terminal_signals();
            execvp(args[0], args);  //ejecuta un comando externo
            printf("Error. Comando %s no encontrado.\n", args[0]);
			exit(EXIT_FAILURE);     // error si no reconoce el comando
        } else{            
            //código del padre   
            printf("Que pasa broo, yo soy el padre!\n");
            mipid = getpid();
            if (background == 1){ //si el proceso se ejecuta en segundo plano se imprime por pantalla y se añade a la lista de tareas
                printf("Comando %s ejecutando en segundo plano con pid %i.\n", args[0], mipid);
                add_job(*listatareas, new_job(mipid, args[0], ground.SEGUNDOPLANO));
                continue;     //continua, repitiendo el bucle
            } else{               //si el proceso se ejecuta en primer plano (FG)
                pid_wait esperopid = waitpid(pid_fork, &status, WUNTRACED | WNOHANG | WCONTINUED);  //el padre espera a que su hijo termine
                set_terminal(mipid);   // después de esperar al hijo, el padre recupera el terminal
                status_res = analyze_status(status, &info);   // se almacena en status_res el estado del comando ejecutado.
                printf("Comando %s ejecutado en primer plano con pid %i.", args[0], mipid);
                if (status_res == status.FINALIZADO){   // si el comando está en estado finalizado, se imprime por pantalla
                    printf("Estado finalizado.\n");
                } else if (status_res == status.SUSPENDIDO){
                    printf("Estado suspendido.\n");     // si el comando está en estado suspendido, se imprime por pantalla y se añade a la lista de tareas
                    add_job(*listatareas, new_job(mipid, args[0], ground.PRIMERPLANO));
                }
                
            }
      } // end while
}




