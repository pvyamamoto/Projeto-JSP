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
    int sequencia;
} Task;

typedef struct Job {
    int id;
    int numTasks;
    int workRemaining;
    int operationAtual;
    Task* tasks;

    int completo;
} Job;

typedef struct Machine{
    int id;
    int tempoAtual;
    int numTasks;
    Task* tasks;

    int disponivel;
} Machine;

typedef struct Solution{
    int numMachines;
    int numJobs;
    int maxMakespan;  
    int totalIdletime;
    float mediaFlowtime;
    Machine* machines;
    Job* jobs;
} Solution;

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
        (*machines)[i].tempoAtual = 1;
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
        (*jobs)[i].workRemaining = 0;

        for(int j = 0; j < *numMachines; j++){
            (*jobs)[i].tasks[j].operation = j;
            (*jobs)[i].tasks[j].jobId = i;
            (*jobs)[i].tasks[j].startTime = -1;
            (*jobs)[i].tasks[j].endTime = -1;
            (*jobs)[i].tasks[j].sequencia = -1;
            if (fscanf(file, "%d %d", &(*jobs)[i].tasks[j].machine, &(*jobs)[i].tasks[j].duration) != 2) {
                fprintf(stderr, "Erro na leitura das tarefas do Job %d\n", i);
                (*jobs)[i].tasks[j].machine = -1;
                (*jobs)[i].tasks[j].duration = -1;

                *jobs = NULL;
                *machines = NULL;
                return;
            }

            (*jobs)[i].workRemaining += (*jobs)[i].tasks[j].duration;
            
        }
    }


    fclose(file);
}

void printJobs(Job *jobs, int numJobs, int numMachines) {
    printf("Numero de Jobs: %d\n", numJobs);
    printf("Numero de Maquinas: %d\n", numMachines);

    for (int i = 0; i < numJobs; i++) {
        printf("Job %d:\n", jobs[i].id);
        printf(" Work restante: %d\n", jobs[i].workRemaining);
        for (int j = 0; j < jobs[i].numTasks; j++) {
            printf("  Task %d: Machine %d, Duration %d, Job ID %d\n", 
                   jobs[i].tasks[j].operation, 
                   jobs[i].tasks[j].machine, 
                   jobs[i].tasks[j].duration, 
                   jobs[i].tasks[j].jobId);
        }
    }
}

void printMachines(Solution* solution){
    
    printf("Numero de Maquinas: %d\n", solution->numMachines);
    printf("Maximo Makespan:  %d\n", solution->maxMakespan);
    printf("Tempo Ocioso Total das Maquinas:  %d\n", solution->totalIdletime);
    printf("Media Tempo de Fluxo dos Jobs:  %.2f\n", solution->mediaFlowtime);

    for (int i = 0; i < solution->numMachines; i++) {
        printf("Machine %d:\n", solution->machines[i].id);
        printf("  tempoAtual: %d\n", solution->machines[i].tempoAtual);
        printf("  Numero de Tarefas: %d\n", solution->machines[i].numTasks);
        for (int j = 0; j < solution->machines[i].numTasks; j++) {
            printf("    Seq %d: Job %d, Task %d: Machine %d, Duration %d, Job ID %d, Start Time %d, End Time %d, Sequencia: %d\n", 
                   j,
                   solution->machines[i].tasks[j].jobId,
                   solution->machines[i].tasks[j].operation, 
                   solution->machines[i].tasks[j].machine, 
                   solution->machines[i].tasks[j].duration, 
                   solution->machines[i].tasks[j].jobId,
                   solution->machines[i].tasks[j].startTime,
                   solution->machines[i].tasks[j].endTime,
                   solution->machines[i].tasks[j].sequencia);
        }
    }
}

void printMachinesToFile(FILE *arquivo, Solution *solution){
    
    fprintf(arquivo, "Numero de Maquinas: %d\n", solution->numMachines);
    fprintf(arquivo, "Maximo Makespan:  %d\n", solution->maxMakespan);
    fprintf(arquivo, "Tempo Ocioso Total das Maquinas:  %d\n", solution->totalIdletime);
    fprintf(arquivo, "Media Tempo de Fluxo dos Jobs:  %.2f\n", solution->mediaFlowtime);

    for(int i = 0; i < solution->numMachines; i++){
        fprintf(arquivo, "Machine %d:\n", solution->machines[i].id);
        fprintf(arquivo, "  tempoAtual: %d\n", solution->machines[i].tempoAtual);
        fprintf(arquivo, "  Numero de Tarefas: %d\n", solution->machines[i].numTasks);
        
        for(int j = 0; j < solution->machines[i].numTasks; j++){
            fprintf(arquivo, "    Task %d: Machine %d, Duration %d, Job ID %d, Start Time %d, End Time %d, Sequencia: %d\n", 
                   solution->machines[i].tasks[j].operation, 
                   solution->machines[i].tasks[j].machine, 
                   solution->machines[i].tasks[j].duration, 
                   solution->machines[i].tasks[j].jobId,
                   solution->machines[i].tasks[j].startTime,
                   solution->machines[i].tasks[j].endTime,
                   solution->machines[i].tasks[j].sequencia);
        }
    }
}

int maquinaCritica(Job *jobs, int numJobs, Machine *machines){
    
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

    int minStartTime = INT_MAX;
    int minMachineId = INT_MAX;

    for(int i = 0; i < numJobs; i++){
        
        if(auxJobs[i].completo) continue;
        
        int machineFreeTime = auxMachines[auxJobs[i].tasks[auxJobs[i].operationAtual].machine].tempoAtual;

        int jobFreeTime = (auxJobs[i].operationAtual > 0)                                    //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[i].tasks[auxJobs[i].operationAtual - 1].endTime  //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 1;                                                                     //else jobFreeTime = 0
                
        int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;

        if(realStartTime < minStartTime){
            minStartTime = realStartTime;
            minMachineId = auxJobs[i].tasks[auxJobs[i].operationAtual].machine;
        }
    }

    return minMachineId;
}

Solution* spt_eav(Job *jobs, int numJobs, Machine *machines, int numMachines){
/*
    Shortest Processing Time com Earliest Available Machine (SPT-EAV)
*/
    Solution *solution = (Solution*)malloc(sizeof(Solution));
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

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

            if(auxMachines[j].disponivel == 1 && auxMachines[j].tempoAtual == 0){
                minMachineTime = auxMachines[j].tempoAtual;
                minMachineId = j;
                break;
            }
            if(auxMachines[j].disponivel == 1 && auxMachines[j].tempoAtual < minMachineTime){
                minMachineTime = auxMachines[j].tempoAtual;
                minMachineId = j;
            }
        }

        for(int j = 0; j < numJobs; j++){
            if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId && auxJobs[j].tasks[auxJobs[j].operationAtual].duration < minJobDuration){
                minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                minJobId = auxJobs[j].id;
            }
        }

        int machineFreeTime = auxMachines[minMachineId].tempoAtual;
        int jobFreeTime = (auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime 
                           : 0;

        int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;

        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = realStartTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = realStartTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].numTasks--;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = realStartTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

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

    solution->numJobs = numJobs;
    solution->jobs = auxJobs;
    solution->numMachines = numMachines;
    solution->machines = auxMachines;

    return solution;
}

Solution* spt_2(Job *jobs, int numJobs, Machine *machines, int numMachines){
/*
    Shortest Processing Time com desempate por Earliest Start Time (SPT-EST) v2
*/
    Solution *solution = (Solution*)malloc(sizeof(Solution));
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

    int jobsCompletos = 0;

    //int minMachineTime = INT_MAX;
    int minMachineId = 0;
    
    int minJobDuration = INT_MAX;
    int minJobId = 0;

    int minRealTime = INT_MAX;
    
    int auxSeq = 0;
    do{

        
        
        for(int i = 0; i < numMachines; i++){

            int machineFreeTime = auxMachines[i].tempoAtual;

            for(int j = 0; j < numJobs; j++){

                int jobFreeTime = (auxJobs[j].operationAtual > 0)                       //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[j].tasks[auxJobs[j].operationAtual - 1].endTime    //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 0;                                                         //else jobFreeTime = 0
                
                int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;
                
                if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == i
                   && auxJobs[j].tasks[auxJobs[j].operationAtual].duration < minJobDuration){

                    minRealTime = realStartTime;
                    minMachineId = i;
                    minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                    minJobId = auxJobs[j].id;
                }else if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == i
                   && auxJobs[j].tasks[auxJobs[j].operationAtual].duration == minJobDuration){
                    if(realStartTime < minRealTime){
                        minRealTime = realStartTime;
                        minMachineId = i;
                        minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                        minJobId = auxJobs[j].id;
                    }
                }
            }
        }


        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = minRealTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = minRealTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].sequencia = auxSeq;
        auxJobs[minJobId].numTasks--;
        auxSeq++;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = minRealTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        if(auxJobs[minJobId].numTasks == 0){
            auxJobs[minJobId].completo = 1;
            jobsCompletos++;
        }

        //minMachineTime = INT_MAX;
        minJobDuration = INT_MAX;
        minMachineId = INT_MIN;
        minJobId = INT_MIN;
        minRealTime = INT_MAX;
    
    }while(jobsCompletos < numJobs);

    solution->numJobs = numJobs;
    solution->jobs = auxJobs;
    solution->numMachines = numMachines;
    solution->machines = auxMachines;

    return solution;
}

Solution* spt_3(Job *jobs, int numJobs, Machine *machines, int numMachines){
/*
    Shortest Processing Time (SPT) v3
*/
    Solution *solution = (Solution*)malloc(sizeof(Solution));
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

    int jobsCompletos = 0;

    //int minMachineTime = INT_MAX;
    int minMachineId = 0;
    
    int minJobDuration = INT_MAX;
    int minJobId = 0;

    int minRealTime = INT_MAX;
    
    int auxSeq = 0;
    do{
        
        for(int i = 0; i < numMachines; i++){

            int machineFreeTime = auxMachines[i].tempoAtual;

            for(int j = 0; j < numJobs; j++){

                int jobFreeTime = (auxJobs[j].operationAtual > 0)                       //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[j].tasks[auxJobs[j].operationAtual - 1].endTime    //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 1;                                                         //else jobFreeTime = 0
                
                int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;
                
                if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == i
                   && auxJobs[j].tasks[auxJobs[j].operationAtual].duration < minJobDuration && realStartTime < minRealTime){

                    minRealTime = realStartTime;
                    minMachineId = i;
                    minJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                    minJobId = auxJobs[j].id;
                }
            }
        }


        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = minRealTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = minRealTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].sequencia = auxSeq;
        auxJobs[minJobId].numTasks--;
        auxSeq++;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = minRealTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        if(auxJobs[minJobId].numTasks == 0){
            auxJobs[minJobId].completo = 1;
            jobsCompletos++;
        }

        //minMachineTime = INT_MAX;
        minJobDuration = INT_MAX;
        minMachineId = INT_MIN;
        minJobId = INT_MIN;
        minRealTime = INT_MAX;
    
    }while(jobsCompletos < numJobs);

    solution->numJobs = numJobs;
    solution->jobs = auxJobs;
    solution->numMachines = numMachines;
    solution->machines = auxMachines;

    return solution;
}

Solution* spt_twkr(Job *jobs, int numJobs, Machine *machines, int numMachines){
/*
    Shortest Processing Time / Total Work Remaining (SPT/TWKR) v4 --> min(Z)

    Z = Duração/Work Total Restante
*/
    Solution *solution = (Solution*)malloc(sizeof(Solution));
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

    int jobsCompletos = 0;

    int minMachineId = 0;    
    int minJobId = 0;

    float minIndZ = INT_MAX;
    
    int auxSeq = 0;  //sequencia das Tasks escolhidas
    do{
        
        minMachineId = maquinaCritica(auxJobs, numJobs, auxMachines);

        for(int j = 0; j < numJobs; j++){
                
            if(auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId){

                float taskIndZ = (float)auxJobs[j].tasks[auxJobs[j].operationAtual].duration/auxJobs[j].workRemaining;

                if(!auxJobs[j].completo && taskIndZ < minIndZ){
                    minIndZ = taskIndZ;
                    minJobId = auxJobs[j].id;    
                }
            }
        }

        int machineFreeTime = auxMachines[minMachineId].tempoAtual;

        int jobFreeTime = (auxJobs[minJobId].operationAtual > 0)                                    //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime  //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 1;                                                                     //else jobFreeTime = 0
                
        int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;

        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = realStartTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = realStartTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].sequencia = auxSeq;
        auxJobs[minJobId].workRemaining -= auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].numTasks--;
        auxSeq++;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = realStartTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        if(auxJobs[minJobId].numTasks == 0){
            auxJobs[minJobId].completo = 1;
            jobsCompletos++;
        }


        minMachineId = INT_MIN;
        minJobId = INT_MIN;
        minIndZ = INT_MAX;
    
    }while(jobsCompletos < numJobs);

    solution->numJobs = numJobs;
    solution->jobs = auxJobs;
    solution->numMachines = numMachines;
    solution->machines = auxMachines;

    return solution;
}

Solution* lpt(Job *jobs, int numJobs, Machine *machines, int numMachines){
/*
    Longest Processing Time (LPT)
*/
    Solution *solution = (Solution*)malloc(sizeof(Solution));
    Job *auxJobs = jobs;
    Machine *auxMachines = machines;

    int jobsCompletos = 0;

    //int minMachineTime = INT_MAX;
    int maxMachineId = 0;
    
    int maxJobDuration = INT_MIN;
    int maxJobId = 0;

    int maxRealTime = INT_MAX;
    do{
        
        for(int i = 0; i < numMachines; i++){

            int machineFreeTime = auxMachines[i].tempoAtual;

            for(int j = 0; j < numJobs; j++){

                int jobFreeTime = (auxJobs[j].operationAtual > 0)                       //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[j].tasks[auxJobs[j].operationAtual - 1].endTime    //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 0;                                                         //else jobFreeTime = 0
                
                int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;
                
                if(!auxJobs[j].completo && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == i
                   && realStartTime <= maxRealTime && auxJobs[j].tasks[auxJobs[j].operationAtual].duration >= maxJobDuration){

                    maxRealTime = realStartTime;
                    maxMachineId = i;
                    maxJobDuration = auxJobs[j].tasks[auxJobs[j].operationAtual].duration;
                    maxJobId = auxJobs[j].id;
                }
            }
        }


        auxJobs[maxJobId].tasks[auxJobs[maxJobId].operationAtual].startTime = maxRealTime;
        auxJobs[maxJobId].tasks[auxJobs[maxJobId].operationAtual].endTime = maxRealTime + auxJobs[maxJobId].tasks[auxJobs[maxJobId].operationAtual].duration;
        auxJobs[maxJobId].numTasks--;

        auxMachines[maxMachineId].tasks[auxMachines[maxMachineId].numTasks] = auxJobs[maxJobId].tasks[auxJobs[maxJobId].operationAtual];
        auxMachines[maxMachineId].tempoAtual = maxRealTime + auxMachines[maxMachineId].tasks[auxMachines[maxMachineId].numTasks].duration;

        auxMachines[maxMachineId].numTasks++;
        auxJobs[maxJobId].operationAtual++;

        if(auxJobs[maxJobId].numTasks == 0){
            auxJobs[maxJobId].completo = 1;
            jobsCompletos++;
        }

        //minMachineTime = INT_MAX;
        maxJobDuration = INT_MIN;
        maxMachineId = INT_MIN;
        maxJobId = INT_MIN;
        maxRealTime = INT_MAX;
    
    }while(jobsCompletos < numJobs);

    solution->numJobs = numJobs;
    solution->jobs = auxJobs;
    solution->numMachines = numMachines;
    solution->machines = auxMachines;

    return solution;
}

void calcularScore(Solution *solution){

    int auxMakespan = 0;

    for(int i = 0; i < solution->numMachines; i++){
        if(solution->machines[i].tempoAtual > auxMakespan){
            auxMakespan = solution->machines[i].tempoAtual;
        }
    }

    solution->maxMakespan = auxMakespan;
    
    int sumFlowtime = 0;

    for(int j = 0; j < solution->numJobs; j++){
        sumFlowtime += solution->jobs[j].tasks[solution->numMachines - 1].endTime;
    }

    solution->mediaFlowtime = (float)sumFlowtime/solution->numJobs;


    solution->totalIdletime = 0;
    for(int i = 0; i < solution->numMachines; i++){
        int auxIdletime = solution->machines[i].tempoAtual;
        for(int j = 0; j < solution->machines[i].numTasks; j++){
            auxIdletime -= solution->machines[i].tasks[j].duration;
        }
        solution->totalIdletime += auxIdletime;
    }
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
    Solution *solution = NULL;

    char *caminho_arquivo = argv[1];

    iniciarInstancias(caminho_arquivo, &numJobs, &numMachines, &jobs, &machines);

    if (jobs == NULL) {
        return 1;
    }

    printJobs(jobs, numJobs, numMachines);

    solution = spt_twkr(jobs, numJobs, machines, numMachines);
    //printMachines(result, numMachines);

    calcularScore(solution);

    char *caminho_saida = argv[2];

    FILE *arquivoSaida = fopen(caminho_saida, "w");

    if (arquivoSaida != NULL) {
        printMachinesToFile(arquivoSaida, solution);
        
        fclose(arquivoSaida);
        
        printf("Resultado salvo com sucesso no arquivo %s!\n", caminho_saida);
    } else {
        printf("Erro ao criar o arquivo de saída.\n");
    }


    clock_t fim = clock();
    double tempo_execucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("Tempo de execucao: %.6f segundos\n", tempo_execucao);

    // Libera memória alocada
    if (machines != NULL && jobs != NULL && solution != NULL){
        for (int i = 0; i < numJobs; i++) {
            free(machines[i].tasks);
            free(jobs[i].tasks);
        }
        free(machines);
        free(jobs);
        free(solution);
    }

    return 0;
}