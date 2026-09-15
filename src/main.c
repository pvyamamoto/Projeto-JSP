#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

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
        (*machines)[i].tasks = (Task*)malloc(*numJobs * sizeof(Task));
        (*machines)[i].disponivel = 0;
    }

    for (int i = 0; i < *numJobs; i++){
        (*jobs)[i].id = i;
        (*jobs)[i].numTasks = *numMachines;
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
    }
}

void printMachines(Machine *machines, int numMachines){
    
    printf("Numero de Maquinas: %d\n", numMachines);

    for (int i = 0; i < numMachines; i++) {
        printf("Machine %d:\n", machines[i].id);
        printf("  Makespan: %d\n", machines[i].makespan);
        printf("  Numero de Tarefas: %d\n", machines[i].numTasks);
        for (int j = 0; j < machines[i].numTasks; j++) {
            printf("    Seq %d: Job %d, Task %d: Machine %d, Duration %d, Job ID %d, Start Time %d, End Time %d\n", 
                   j,
                   machines[i].tasks[j].jobId,
                   machines[i].tasks[j].operation, 
                   machines[i].tasks[j].machine, 
                   machines[i].tasks[j].duration, 
                   machines[i].tasks[j].jobId,
                   machines[i].tasks[j].startTime,
                   machines[i].tasks[j].endTime);
        }
    }
}

void printMachinesToFile(FILE *arquivo, Machine *machines, int numMachines){
    
    fprintf(arquivo, "Numero de Maquinas: %d\n", numMachines);
    for (int i = 0; i < numMachines; i++) {
        fprintf(arquivo, "Machine %d:\n", machines[i].id);
        fprintf(arquivo, "  Makespan: %d\n", machines[i].makespan);
        fprintf(arquivo, "  Numero de Tarefas: %d\n", machines[i].numTasks);
        
        for (int j = 0; j < machines[i].numTasks; j++) {
            fprintf(arquivo, "    Task %d: Machine %d, Duration %d, Job ID %d, Start Time %d, End Time %d\n", 
                   machines[i].tasks[j].operation, 
                   machines[i].tasks[j].machine, 
                   machines[i].tasks[j].duration, 
                   machines[i].tasks[j].jobId,
                   machines[i].tasks[j].startTime,
                   machines[i].tasks[j].endTime);
        }
    }
}

Machine* spt(Job** jobs, int numJobs, Machine** machines, int numMachines){
    
    Job *auxJobs = *jobs;
    Machine *auxMachines = *machines;

    int jobsCompletos = 0;

    int minMachineTime = INT_MAX;
    int minMachineId = 0;
    
    int minJobDuration = INT_MAX;
    int minJobId = 0;

    do{
        
        for(int j = 0; j < numMachines; j++){
            auxMachines[j].disponivel = 0;
        }

        for(int i = 0; i < numJobs; i++){
            if( !auxJobs[i].completo && auxMachines[auxJobs[i].tasks[auxJobs[i].operationAtual].machine].disponivel == 0){
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
            if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId && auxJobs[j].tasks[auxJobs[j].operationAtual].duration < minJobDuration){
                minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                minJobId = auxJobs[j].id;
            }
        }

        int machineFreeTime = auxMachines[minMachineId].makespan;
        int jobFreeTime = (auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime 
                           : 0;

        int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;

        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = realStartTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = realStartTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].numTasks--;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].makespan = realStartTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        if(auxJobs[minJobId].numTasks == 0){
            auxJobs[minJobId].completo = 1;
            jobsCompletos++;
        }

        minMachineTime = INT_MAX;
        minJobDuration = INT_MAX;
        minMachineId = INT_MIN;
        minJobId = INT_MIN;
    
    }while(jobsCompletos < numJobs);

    return auxMachines;
}


int main (int argc, char *argv[]){
    
    clock_t inicio = clock();

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

    Machine *result = spt(&jobs, numJobs, &machines, numMachines);
    //printMachines(result, numMachines);

    char *caminho_saida = argv[2];

    FILE *arquivoSaida = fopen(caminho_saida, "w");

    if (arquivoSaida != NULL) {
        printMachinesToFile(arquivoSaida, result, numMachines);
        
        fclose(arquivoSaida);
        
        printf("Resultado salvo com sucesso no arquivo %s!\n", caminho_saida);
    } else {
        printf("Erro ao criar o arquivo de saída.\n");
    }


    clock_t fim = clock();
    double tempo_execucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("Tempo de execucao: %f segundos\n", tempo_execucao);

    // Libera memória alocada
    if (machines != NULL && jobs != NULL){
        free(machines);

        for (int i = 0; i < numJobs; i++) {
            free(jobs[i].tasks);
        }
        free(jobs);
    }

    return 0;
}