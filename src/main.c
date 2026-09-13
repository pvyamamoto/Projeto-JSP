#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct Task { 
    int operation;
    int machine;
    int duration;
    int jobId;
    int startTime;
    int endTime;
} Task;

typedef struct Job {
    int id;
    int numTasks;
    int tempoAtual;
    int operationAtual;
    Task* tasks;

    int completo;
} Job;

typedef struct Machine{
    int id;
    int makespan;
    int numTasks;
    Task* tasks;

    int disponivel;
} Machine;

void iniciarInstancias(const char *filePath, int *numJobs, int *numMachines, Job **jobs, Machine **machines){

    FILE *file = fopen(filePath, "r");

    if (file == NULL) {

        perror("Erro ao abrir as instancias de teste\n");
        
        *jobs = NULL;
        *machines = NULL;
        return;
    }

    if (fscanf(file, "%d %d", numJobs, numMachines) != 2){
        fprintf(stderr, "Erro ao ler o numero de jobs e maquinas do arquivo.\n");
        fclose(file);
        *jobs = NULL;
        *machines = NULL;
        return;
    }

    *jobs = (Job*)malloc(*numJobs * sizeof(Job));
    *machines = (Machine*)malloc(*numMachines * sizeof(Machine));

    for (int i = 0; i < *numMachines; i++){
        (*machines)[i].id = i;
        (*machines)[i].makespan = 0;
        (*machines)[i].numTasks = 0;
        (*machines)[i].tasks = (Task*)malloc(*numMachines * sizeof(Task));
        (*machines)[i].disponivel = 0;
    }

    for (int i = 0; i < *numJobs; i++){
        (*jobs)[i].id = i;
        (*jobs)[i].numTasks = *numMachines;
        (*jobs)[i].tempoAtual = 0;
        (*jobs)[i].operationAtual = 0;
        (*jobs)[i].tasks = (Task*)malloc(*numMachines * sizeof(Task));
        (*jobs)[i].completo = 0;

        for(int j = 0; j < *numMachines; j++){
            (*jobs)[i].tasks[j].operation = j;
            (*jobs)[i].tasks[j].jobId = i;
            (*jobs)[i].tasks[j].startTime = 0;
            (*jobs)[i].tasks[j].endTime = 0;
            if (fscanf(file, "%d %d", &(*jobs)[i].tasks[j].machine, &(*jobs)[i].tasks[j].duration) != 2) {
                fprintf(stderr, "Erro na leitura das tarefas do Job %d\n", i);
                (*jobs)[i].tasks[j].machine = -1;
                (*jobs)[i].tasks[j].duration = -1;

                *jobs = NULL;
                *machines = NULL;
                return;
            }
            
        }
    }


    fclose(file);
}

void printJobs(Job *jobs, int numJobs, int numMachines) {
    printf("Numero de Jobs: %d\n", numJobs);
    printf("Numero de Maquinas: %d\n", numMachines);

    for (int i = 0; i < numJobs; i++) {
        printf("Job %d:\n", jobs[i].id);
        for (int j = 0; j < jobs[i].numTasks; j++) {
            printf("  Task %d: Machine %d, Duration %d, Job ID %d\n", 
                   jobs[i].tasks[j].operation, 
                   jobs[i].tasks[j].machine, 
                   jobs[i].tasks[j].duration, 
                   jobs[i].tasks[j].jobId);
        }
        printf("Tempo restante do Job %d: %d\n", jobs[i].id, jobs[i].tempoRestante);
    }
}

/*
void spt(Job** jobs, int numJobs, Machine* machines, int numMachines){
    
    Job *auxJobs = *jobs;
    Machine *auxMachines = machines;

    int jobsCompletos = 0;

    int minMachineTime = INT_MAX; // Menor makespan entre as maquinas
    int minMachineId = 0;
    
    int minJobDuration = INT_MAX;
    int minJobId = 0;

    do{
        for(int i = 0; i < numJobs; i++){
            if(auxMachines[auxJobs[i].tasks[auxJobs[i].operationAtual].machine].disponivel == 0 && !auxJobs[i].completo){
                auxMachines[auxJobs[i].tasks[auxJobs[i].operationAtual].machine].disponivel = 1;
            }
        }
        
        for(int j = 0; j < numMachines; j++){

            if(auxMachines[j].disponivel == 1 && auxMachines[j].makespan == 0){
                minMachineTime = auxMachines[j].makespan;
                minMachineId = j;
                break;
            }
            if(auxMachines[j].disponivel == 1 && auxMachines[j].makespan < minMachineTime){
                minMachineTime = auxMachines[j].makespan;
                minMachineId = j;
            }
        }

        for(int j = 0; j < numJobs; j++){
            if(auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId && auxJobs[j].tasks[auxJobs[j].operationAtual].duration < minJobDuration && !auxJobs[j].completo){
                minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                minJobId = auxJobs[j].id;
            }
        }

        auxJobs[minJobId].tempoAtual = auxMachines[minMachineId].makespan;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = auxMachines[minMachineId].makespan;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = auxMachines[minMachineId].makespan + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].numTasks--;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].makespan += auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        minMachineTime = INT_MAX;
        minJobDuration = INT_MAX;

        if(auxJobs[minJobId].numTasks == 0){
            auxJobs[minJobId].completo = 1;
            jobsCompletos++;
        }
    
    }while(jobsCompletos < numJobs);

}
*/


int main (int argc, char *argv[]){
    
    if (argc < 2) {
        printf("Uso incorreto!\n");
        printf("Sintaxe esperada: %s <caminho_do_arquivo>\n", argv[0]);
        return 1;
    }

    int numJobs = 0;
    int numMachines = 0;
    Job *jobs = NULL;
    Machine *machines = NULL;

    char *caminho_arquivo = argv[1];

    iniciarInstancias(caminho_arquivo, &numJobs, &numMachines, &jobs, &machines);

    if (jobs == NULL) {
        return 1;
    }

    printJobs(jobs, numJobs, numMachines);

    // Libera memória alocada
    if (machines != NULL && jobs != NULL && numMachines > 0) {
        free(machines);

        for (int i = 0; i < numJobs; i++) {
            free(jobs[i].tasks);
        }
        free(jobs);
    }

    return 0;
}