#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Task { 
    int operation;
    int machine;
    int duration;
} Task;

typedef struct Job {
    int id;
    Task* tasks;
} Job;

typedef struct Machine{
    int id;
    int totalDuration; // PENSAR MELHOR SE É DURAÇÃO TOTAL MESMO 
} Machine;

Job* iniciarInstancias(const char *filePath, int *numJobs, int *numMachines) {

    FILE *file = fopen(filePath, "r");

    if (file == NULL) {

        perror("Erro ao abrir as instancias de teste\n");
        
        return NULL;
    }

    if (fscanf(file, "%d %d", numJobs, numMachines) != 2){
        fprintf(stderr, "Erro ao ler o numero de jobs e maquinas do arquivo.\n");
        fclose(file);
        return NULL;
    }

    Job *jobs = (Job*)malloc(*numJobs * sizeof(Job));

    for (int i = 0; i < *numJobs; i++){
        jobs[i].id = i;
        jobs[i].tasks = (Task*)malloc(*numMachines * sizeof(Task));

        for(int j = 0; j < *numMachines; j++){
            jobs[i].tasks[j].operation = j;

            if (fscanf(file, "%d %d", &jobs[i].tasks[j].machine, &jobs[i].tasks[j].duration) != 2) {
                fprintf(stderr, "Erro na leitura das tarefas do Job %d\n", i);
                jobs[i].tasks[j].machine = -1;
                jobs[i].tasks[j].duration = -1;

                return NULL;
            }
        }
    }

    fclose(file);
    return jobs;
}

void printJobs(Job *jobs, int numJobs, int numMachines) {
    printf("Numero de Jobs: %d\n", numJobs);
    printf("Numero de Maquinas: %d\n", numMachines);

    for (int i = 0; i < numJobs; i++) {
        printf("Job %d:\n", jobs[i].id);
        for (int j = 0; j < numMachines; j++) {
            printf("  Task %d: Machine %d, Duration %d\n", 
                   jobs[i].tasks[j].operation, 
                   jobs[i].tasks[j].machine, 
                   jobs[i].tasks[j].duration);
        }
    }
}

int main (int argc, char *argv[]){
    
    if (argc < 2) {
        printf("Uso incorreto!\n");
        printf("Sintaxe esperada: %s <caminho_do_arquivo>\n", argv[0]);
        return 1;
    }

    int numJobs = 0;
    int numMachines = 0;
    Job *jobs = NULL;

    char *caminho_arquivo = argv[1];

    jobs = iniciarInstancias(caminho_arquivo, &numJobs, &numMachines);

    if (jobs == NULL) {
        return 1;
    }

    printJobs(jobs, numJobs, numMachines);

    // Liberar memória alocada
    for (int i = 0; i < numJobs; i++) {
        free(jobs[i].tasks);
    }
    free(jobs);

    return 0;
}