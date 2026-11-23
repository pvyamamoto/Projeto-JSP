#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>

typedef struct Task{ 
    int operation;
    int machine;
    int duration;
    int jobId;
    int startTime;
    int endTime;
    int sequencia;
} Task;

typedef struct Job{
    int id;
    int numTasks;
    int workRemaining;
    int operationAtual;
    Task* tasks;
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
    int numTasks;
    int maxMakespan;
    int totalIdletime;
    float mediaFlowtime;
    Machine* machines;
    Job* jobs;
    int tamCaminhoCritico;
    Task* caminhoCritico;
    unsigned int seed;
} Solution;

typedef struct ParTask{
    Task op1;
    Task op2;
} ParTask;

typedef struct BlocoCritico{
    int inicio;
    int fim;
    int machine;
    int tam;
} BlocoCritico;

typedef struct Cromossomo{
    int* genes;
    Solution* solucao;
} Cromossomo;

typedef struct Populacao{
    Cromossomo* individuos;
    int numIndividuos;
    Cromossomo *melhoresIndividuosGeracao;
    Solution* melhorSolucao;
} Populacao;

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

void printSolutionToFile(FILE *arquivo, Solution *solution){
    
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
        
        if(auxJobs[i].operationAtual >= auxJobs[i].numTasks) continue;
        
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
                
            if(auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId && auxJobs[j].operationAtual < auxJobs[j].numTasks){

                float taskIndZ = (float)auxJobs[j].tasks[auxJobs[j].operationAtual].duration/auxJobs[j].workRemaining;

                if(taskIndZ < minIndZ){
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
        auxSeq++;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = realStartTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        if(auxJobs[minJobId].operationAtual == auxJobs[minJobId].numTasks){
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
    solution->numTasks = auxSeq + 1;
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
                
                if(auxJobs[j].operationAtual < auxJobs[j].numTasks && auxJobs[j].tasks[auxJobs[j].operationAtual].machine == i
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

        auxMachines[maxMachineId].tasks[auxMachines[maxMachineId].numTasks] = auxJobs[maxJobId].tasks[auxJobs[maxJobId].operationAtual];
        auxMachines[maxMachineId].tempoAtual = maxRealTime + auxMachines[maxMachineId].tasks[auxMachines[maxMachineId].numTasks].duration;

        auxMachines[maxMachineId].numTasks++;
        auxJobs[maxJobId].operationAtual++;

        if(auxJobs[maxJobId].operationAtual == auxJobs[maxJobId].numTasks){
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

Task* obterPredecessorMaquina(Solution *solution, Task *t){
    
    Machine *maq = &solution->machines[t->machine];
    for (int k = 0; k < maq->numTasks; k++) {
        if (maq->tasks[k].jobId == t->jobId && maq->tasks[k].operation == t->operation) {
            if (k > 0) {
                return &maq->tasks[k - 1];
            }
            return NULL;
        }
    }
    return NULL;
}

Task* caminhoCritico(Solution *solution){
    
    if(solution == NULL || solution->numJobs <= 0 || solution->numMachines <= 0){
        return NULL;
    }

    int numJobs = solution->numJobs;
    int numMachines = solution->numMachines;
    int maxTotalTasks = numJobs * numMachines;

    // 1. Encontrar a tarefa terminal que define o Makespan
    Task *tarefaMakespan = NULL;
    int maxEnd = -1;

    for(int j = 0; j < numJobs; j++){
        // A última operação de cada job está no índice (numMachines - 1)
        Task *tFinal = &solution->jobs[j].tasks[numMachines - 1];
        if(tFinal->endTime > maxEnd){
            maxEnd = tFinal->endTime;
            tarefaMakespan = tFinal;
        }
    }

    if(tarefaMakespan == NULL){
        return NULL;
    }

    // 2. Vetor temporário para reconstruir o caminho de trás para frente
    Task **caminhoReverso = (Task**)malloc(maxTotalTasks * sizeof(Task*));
    int tamCaminho = 0;

    Task *atual = tarefaMakespan;
    caminhoReverso[tamCaminho++] = atual;

    // 3. Retroceder até a primeira tarefa do agendamento (tempo base 1 do spt_twkr)
    while(atual != NULL && atual->startTime > 1){
        Task *predJob = NULL;
        Task *predMaq = NULL;

        // Candidato 1: Predecessor no mesmo Job
        if(atual->operation > 0){
            predJob = &solution->jobs[atual->jobId].tasks[atual->operation - 1];
        }

        // Candidato 2: Predecessor na mesma Máquina
        predMaq = obterPredecessorMaquina(solution, atual);

        // Identifica quem causou o gargalo (quem terminou no instante em que a atual começou)
        // Em caso de empate, ambos são críticos; priorizamos a precedência da máquina ou do job
        if(predMaq != NULL && predMaq->endTime == atual->startTime){
            atual = predMaq;
        }else if(predJob != NULL && predJob->endTime == atual->startTime){
            atual = predJob;
        }else{
            // Chegou ao início da cadeia crítica
            break;
        }

        caminhoReverso[tamCaminho++] = atual;
    }

    // 4. Montar o vetor de retorno na ordem cronológica correta (do início ao fim)
    Task *caminho = (Task*)malloc(tamCaminho * sizeof(Task));
    for(int i = 0; i < tamCaminho; i++){
        caminho[i] = *caminhoReverso[tamCaminho - 1 - i];
    }

    solution->tamCaminhoCritico = tamCaminho;
    solution->caminhoCritico = caminho;

    free(caminhoReverso);
    return caminho;
}

void printCaminhoCritico(Task *caminho, int tamCaminho, int makespan){
    
    if (caminho == NULL || tamCaminho <= 0) {
        printf("Caminho critico vazio ou nao encontrado.\n");
        return;
    }
    printf("\n==================== CAMINHO CRITICO ====================\n");
    printf("Total de tarefas no caminho critico: %d\n", tamCaminho);
    
    int duracaoTotal = 0;
    for (int i = 0; i < tamCaminho; i++) {
        printf("  [%02d] Job %2d | Op %2d | Maq %2d | Dur: %2d | Start: %3d | End: %3d\n",
               i + 1,
               caminho[i].jobId,
               caminho[i].operation,
               caminho[i].machine,
               caminho[i].duration,
               caminho[i].startTime,
               caminho[i].endTime);
        duracaoTotal += caminho[i].duration;
    }
    int dif = makespan - (duracaoTotal + 1);
    printf("Duracao acumulada das tarefas: %d | Makespan: %d\n", duracaoTotal+1, makespan);
    if (dif == 0) {
        printf("Status: Caminho critico sem folgas (Valido!)\n");
    } else {
        printf("Status: Folga detectada (Diferenca: %d)\n", dif);
    }
    printf("=========================================================\n\n");
}

void printCaminhoCriticoToFile(FILE *arquivo, Task *caminho, int tamCaminho, int makespan){
    
    if (caminho == NULL || tamCaminho <= 0) {
        fprintf(arquivo, "Caminho critico vazio ou nao encontrado.\n");
        return;
    }
    fprintf(arquivo, "\n==================== CAMINHO CRITICO ====================\n");
    fprintf(arquivo, "Total de tarefas no caminho critico: %d\n", tamCaminho);
    
    int duracaoTotal = 0;
    for (int i = 0; i < tamCaminho; i++) {
        fprintf(arquivo, "  [%02d] Job %2d | Op %2d | Maq %2d | Dur: %2d | Start: %3d | End: %3d\n",
                i + 1,
               caminho[i].jobId,
               caminho[i].operation,
               caminho[i].machine,
               caminho[i].duration,
               caminho[i].startTime,
               caminho[i].endTime);
        duracaoTotal += caminho[i].duration;
    }
    int dif = makespan - (duracaoTotal + 1);
    fprintf(arquivo, "Duracao acumulada das tarefas: %d | Makespan: %d\n", duracaoTotal+1, makespan);
    if (dif == 0) {
        fprintf(arquivo, "Status: Caminho critico sem folgas (Valido!)\n");
    } else {
        fprintf(arquivo, "Status: Folga detectada (Diferenca: %d)\n", dif);
    }
    fprintf(arquivo, "=========================================================\n\n");
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

Task clonarTask(const Task *origem){
    
    Task clone;

    if(origem == NULL){
        memset(&clone, 0, sizeof(Task));
        clone.machine = -1;
        clone.jobId = -1;
        return clone;
    }
    clone.operation  = origem->operation;
    clone.machine    = origem->machine;
    clone.duration   = origem->duration;
    clone.jobId      = origem->jobId;
    clone.startTime  = origem->startTime;
    clone.endTime    = origem->endTime;
    clone.sequencia  = origem->sequencia;
    
    return clone;
}

Job* clonarJob(Job *job){

    if(job == NULL){
        return NULL;
    }

    Job *clone = (Job*)malloc(sizeof(Job));

    if(clone == NULL){
        return NULL;
    }

    clone->id             = job->id;
    clone->numTasks       = job->numTasks;
    clone->operationAtual = job->operationAtual;
    clone->workRemaining  = job->workRemaining;

    if(job->numTasks > 0 && job->tasks != NULL){
        clone->tasks = (Task*)malloc(job->numTasks * sizeof(Task));
        for(int i = 0; i < job->numTasks; i++){
            clone->tasks[i] = clonarTask(&job->tasks[i]);
        }
    }else{
        clone->tasks = NULL;
    }

    return clone;
}

Machine* clonarMachine(Machine *machine){

    if(machine == NULL){
        return NULL;
    }

    Machine *clone = (Machine*)malloc(sizeof(Machine));

    if(clone == NULL){
        return NULL;
    }

    clone->id         = machine->id;
    clone->tempoAtual = machine->tempoAtual;
    clone->numTasks   = machine->numTasks;

    if(machine->numTasks > 0 && machine->tasks != NULL){
        clone->tasks = (Task*)malloc(machine->numTasks * sizeof(Task));
        for(int i = 0; i < machine->numTasks; i++){
            clone->tasks[i] = clonarTask(&machine->tasks[i]);
        }
    }else{
        clone->tasks = NULL;
    }

    return clone;
}

Solution* clonarSolution(Solution *solution){

    if(solution == NULL){
        return NULL;
    }

    Solution *clone = (Solution*)malloc(sizeof(Solution));
    if(clone == NULL){
        return NULL;
    }

    clone->numMachines       = solution->numMachines;
    clone->numJobs           = solution->numJobs;
    clone->numTasks          = solution->numTasks;
    clone->maxMakespan       = solution->maxMakespan;
    clone->mediaFlowtime     = solution->mediaFlowtime;
    clone->totalIdletime     = solution->totalIdletime;
    clone->tamCaminhoCritico = solution->tamCaminhoCritico;
    clone->seed              = solution->seed;

    clone->jobs = (Job*)malloc(clone->numJobs * sizeof(Job));
    for(int i = 0; i < clone->numJobs; i++){
        Job *cloneJob = clonarJob(&solution->jobs[i]);
        clone->jobs[i] = *cloneJob;
        free(cloneJob);
    }

    clone->machines = (Machine*)malloc(clone->numMachines * sizeof(Machine));
    for(int i = 0; i < clone->numMachines; i++){
        Machine *cloneMachine = clonarMachine(&solution->machines[i]);
        clone->machines[i] = *cloneMachine;
        free(cloneMachine);
    }

    if(solution->caminhoCritico != NULL && solution->tamCaminhoCritico > 0){
        clone->caminhoCritico = (Task*)malloc(clone->tamCaminhoCritico * sizeof(Task));
        for(int i = 0; i < clone->tamCaminhoCritico; i++){
            clone->caminhoCritico[i] = clonarTask(&solution->caminhoCritico[i]);
        }
    }else{
        clone->caminhoCritico = NULL;
    }

    return clone;
}

BlocoCritico* clonarBlocoCritico(BlocoCritico *blocos, int numBlocos){

    BlocoCritico *clone = (BlocoCritico*)malloc(numBlocos * sizeof(BlocoCritico));

    for(int i = 0; i < numBlocos; i++){
        clone[i].inicio  = blocos[i].inicio;
        clone[i].fim     = blocos[i].fim;
        clone[i].tam     = blocos[i].tam;
        clone[i].machine = blocos[i].machine;
    }

    return clone;
}

void liberarSolucao(Solution *sol){
    
    if(sol == NULL) return;
    
    if(sol->jobs != NULL){
        for(int i = 0; i < sol->numJobs; i++){
            if(sol->jobs[i].tasks != NULL){
                free(sol->jobs[i].tasks);
            }
        }
        free(sol->jobs);
    }
    
    if(sol->machines != NULL){
        for(int i = 0; i < sol->numMachines; i++){
            if(sol->machines[i].tasks != NULL){
                free(sol->machines[i].tasks);
            }
        }
        free(sol->machines);
    }
    
    if(sol->caminhoCritico != NULL){
        free(sol->caminhoCritico);
    }
    
    free(sol);
}

void liberarCromossomo(Cromossomo *cromossomo, Solution *solucaoPreservada){
    if(cromossomo == NULL) return;
    // Libera os genes
    if(cromossomo->genes != NULL){
        free(cromossomo->genes);
        cromossomo->genes = NULL;
    }
    // Libera a solucao apenas se nao for a que deve ser preservada
    if(cromossomo->solucao != NULL){
        if(cromossomo->solucao != solucaoPreservada){
            liberarSolucao(cromossomo->solucao);
        }
        cromossomo->solucao = NULL;
    }
}

void liberarPopulacao(Populacao *populacao, Solution *solucaoPreservada){
    if(populacao == NULL) return;
    // 1. Libera cada individuo da populacao
    if(populacao->individuos != NULL){
        for(int i = 0; i < populacao->numIndividuos; i++){
            liberarCromossomo(&populacao->individuos[i], solucaoPreservada);
        }
        free(populacao->individuos);
        populacao->individuos = NULL;
    }
    // 2. Libera o vetor de melhores individuos da geracao
    // NOTA: Apenas liberamos o ponteiro do array, pois os cromossomos contidos nele
    // apontam para os mesmos 'genes' e 'solucao' ja liberados no loop acima.
    if(populacao->melhoresIndividuosGeracao != NULL){
        free(populacao->melhoresIndividuosGeracao);
        populacao->melhoresIndividuosGeracao = NULL;
    }
    // 3. Libera a struct Populacao em si
    free(populacao);
}

Solution* iniciarSolucaoVazia(Job *jobs,int numJobs, Machine *machines, int numMachines, int numTasks){

    Solution *solution = (Solution*)malloc(sizeof(Solution));
    solution->numJobs = numJobs;
    solution->numMachines = numMachines;
    solution->numTasks = numTasks;
    solution->maxMakespan = 0;
    solution->mediaFlowtime = 0.0f;
    solution->totalIdletime = 0;
    solution->tamCaminhoCritico = 0;
    solution->caminhoCritico = NULL;
    
    solution->jobs = (Job*)malloc(numJobs * sizeof(Job));
    for(int j = 0; j < numJobs; j++){
        solution->jobs[j].id = j;
        solution->jobs[j].numTasks = numMachines;
        solution->jobs[j].operationAtual = 0;
        solution->jobs[j].workRemaining = jobs[j].workRemaining;
        solution->jobs[j].tasks = (Task*)malloc(numMachines * sizeof(Task));
        for(int op = 0; op < numMachines; op++){
            solution->jobs[j].tasks[op] = clonarTask(&jobs[j].tasks[op]);
        }
    }

    solution->machines = (Machine*)malloc(numMachines * sizeof(Machine));
    for(int m = 0; m < numMachines; m++){
        solution->machines[m].id = m;
        solution->machines[m].tempoAtual = 1;
        solution->machines[m].numTasks = 0;
        solution->machines[m].tasks = (Task*)malloc(numJobs * sizeof(Task));
        solution->machines[m].disponivel = 0;
    }

    return solution;
}

void obterAdjacentesOpMaq(Task *caminhoCritico, int tamCaminho, int idxop, Task **opAnt, Task **opPos){

    *opAnt = NULL;
    *opPos = NULL;

    if(caminhoCritico == NULL || tamCaminho <= 0){
        return;
    }

    if(idxop > 0){
        if(caminhoCritico[idxop - 1].machine == caminhoCritico[idxop].machine){

            *opAnt = &caminhoCritico[idxop - 1];
        }
    }

    if(idxop < tamCaminho - 1){
        if(caminhoCritico[idxop + 1].machine == caminhoCritico[idxop].machine){

            *opPos = &caminhoCritico[idxop + 1];
        }
    }
}

int obterPosicaoNaMaquina(Solution *solution, Task *t) {
    
    if (solution == NULL || t == NULL) return -1;
    
    Machine *maq = &solution->machines[t->machine];
    for (int k = 0; k < maq->numTasks; k++) {
        if (maq->tasks[k].jobId == t->jobId && maq->tasks[k].operation == t->operation) {
            return k;
        }
    }
    return -1;
}

/*
 * Recalculo por Propagacao Seletiva
 * maqId: maquina onde ocorreu a troca
 * posA: indice da primeira tarefa trocada na maquina (posicao k)
 * posB: indice da segunda tarefa trocada na maquina (posicao k+1)
*/
int recalcularSolucaoSeletiva(Solution *solution, int maqId, int posA, int posB){
    
    if(solution == NULL){
        return 0;
    }
    int numJobs = solution->numJobs;
    int numMachines = solution->numMachines;
    int totalTasks = numJobs * numMachines;

    // Estrutura auxiliar de posicao na maquina
    int posNaMaquina[numJobs][numMachines];

    for(int m = 0; m < numMachines; m++){
        for(int k = 0; k < solution->machines[m].numTasks; k++){
            Task *t = &solution->machines[m].tasks[k];
            posNaMaquina[t->jobId][t->operation] = k;
        }
    }
    
    // Fila circular para eventos de propagação
    typedef struct { 
        int jobId; 
        int op; 
    } ParOp;
    // Como uma tarefa nunca é inserida duas vezes simultaneamente (graças à matriz naFila),
    // o tamanho da fila coexistente nunca excede totalTasks.
    int capacidadeFila = totalTasks + 16;
    ParOp *fila = (ParOp*)malloc(capacidadeFila * sizeof(ParOp));
    if (fila == NULL) return 0;
    int inicioFila = 0, fimFila = 0;
    int qtdNaFila = 0;
    
    // Matriz de controle para evitar duplicar o mesmo nó simultaneamente na fila
    int naFila[numJobs][numMachines];
    for(int j = 0; j < numJobs; j++){
        for (int op = 0; op < numMachines; op++) {
            naFila[j][op] = 0;
        }
    }
    
    // Enfileira as tarefas a partir da posição da troca
    Task *t1 = &solution->machines[maqId].tasks[posA];
    Task *t2 = &solution->machines[maqId].tasks[posB];
    
    fila[fimFila] = (ParOp){t1->jobId, t1->operation};
    fimFila = (fimFila + 1) % capacidadeFila;
    qtdNaFila++;
    naFila[t1->jobId][t1->operation] = 1;
    
    fila[fimFila] = (ParOp){t2->jobId, t2->operation};
    fimFila = (fimFila + 1) % capacidadeFila;
    qtdNaFila++;
    naFila[t2->jobId][t2->operation] = 1;
    
    int iteracoes = 0;
    int limiteMaximo = totalTasks * numMachines; // Guarda contra ciclos
    while(qtdNaFila > 0){
        if(++iteracoes > limiteMaximo){
            // Ciclo detectado durante a propagação
            free(fila);
            return 0;
        }
        
        ParOp atual = fila[inicioFila];
        inicioFila = (inicioFila + 1) % capacidadeFila;
        qtdNaFila--;
        int j = atual.jobId;
        int op = atual.op;
        naFila[j][op] = 0;
        
        Task *tJob = &solution->jobs[j].tasks[op];
        int m = tJob->machine;
        int pos = posNaMaquina[j][op];
        
        // 1. Calcula o novo Start Time
        int tempoPredJob = 1;
        if(op > 0){
            tempoPredJob = solution->jobs[j].tasks[op - 1].endTime;
        }
        int tempoPredMaq = 1;
        if(pos > 0){
            tempoPredMaq = solution->machines[m].tasks[pos - 1].endTime;
        }
        
        int novoStart = (tempoPredJob > tempoPredMaq) ? tempoPredJob : tempoPredMaq;
        if(novoStart < 1){
            novoStart = 1;
        }
        
        // Se o horário de início mudou, propaga para os sucessores
        if(novoStart != tJob->startTime){
            tJob->startTime = novoStart;
            tJob->endTime = novoStart + tJob->duration;
            
            // Sincroniza cópia na máquina
            solution->machines[m].tasks[pos].startTime = tJob->startTime;
            solution->machines[m].tasks[pos].endTime = tJob->endTime;
            
            // Enfileira Sucessor no Job
            if(op + 1 < numMachines){
                if(!naFila[j][op + 1]){
                    fila[fimFila] = (ParOp){ j, op + 1 };
                    fimFila = (fimFila + 1) % capacidadeFila;
                    qtdNaFila++;
                    naFila[j][op + 1] = 1;
                }
            }
            
            // Enfileira Sucessor na Máquina
            if(pos + 1 < solution->machines[m].numTasks){
                Task *sucMaq = &solution->machines[m].tasks[pos + 1];
                if(!naFila[sucMaq->jobId][sucMaq->operation]){
                    fila[fimFila] = (ParOp){sucMaq->jobId, sucMaq->operation};
                    fimFila = (fimFila + 1) % capacidadeFila;
                    qtdNaFila++;
                    naFila[sucMaq->jobId][sucMaq->operation] = 1;
                }
            }
        }
    }
    free(fila);
    
    // Atualiza tempoAtual das máquinas
    for(int m = 0; m < numMachines; m++){
        int ultimoIdx = solution->machines[m].numTasks - 1;
        solution->machines[m].tempoAtual = solution->machines[m].tasks[ultimoIdx].endTime;
    }
    
    // Recalcula score e novo caminho crítico
    calcularScore(solution);

    if(solution->caminhoCritico != NULL){
        free(solution->caminhoCritico);
        solution->caminhoCritico = NULL;
    }

    caminhoCritico(solution);

    return 1;
}

Solution* swap(Solution *solution, int pos1, int pos2, int m){

    Task temp = solution->machines[m].tasks[pos1];
    solution->machines[m].tasks[pos1] = solution->machines[m].tasks[pos2];
    solution->machines[m].tasks[pos2] = temp;

    return solution;
}

int obterNumTrocasdeMaquina(Task *caminhoCritico, int tamCaminho){

    int numBlocos = 0;

    for(int i = 0; i < tamCaminho - 1; i++){
        if(caminhoCritico[i].machine == caminhoCritico[i+1].machine){
            numBlocos++;
            while(i < tamCaminho - 1 && caminhoCritico[i].machine == caminhoCritico[i+1].machine){
                i++;
            }
        }
    }

    return numBlocos;
}

BlocoCritico* obterBlocos(Task *caminhoCritico, int tamCaminho, int *numBlocos){

    int numBlocosT = obterNumTrocasdeMaquina(caminhoCritico, tamCaminho);

    if(numBlocosT == 1){
        *numBlocos = INT_MAX;
        return NULL;
    }

    BlocoCritico *blocos = (BlocoCritico*)malloc(numBlocosT * sizeof(BlocoCritico));

    int blocoIdx = 0;
    for(int i = 0; i < tamCaminho - 1; i++){

        if(caminhoCritico[i].machine == caminhoCritico[i+1].machine){
            int inicioBloco = i;
            int maquinaAtual = caminhoCritico[i].machine;
            while(i < tamCaminho - 1 && caminhoCritico[i].machine == caminhoCritico[i+1].machine){
                i++;
            }
            int fimBloco = i;
            int tamBloco = fimBloco - inicioBloco + 1;

            if(tamBloco >= 2){
                blocos[blocoIdx].inicio = inicioBloco;
                blocos[blocoIdx].fim = fimBloco;
                blocos[blocoIdx].machine = maquinaAtual;
                blocos[blocoIdx].tam = tamBloco;
                blocoIdx++;
            }
        }
    }

    if(blocoIdx == 0){
        free(blocos);
        blocos = NULL;

        if(numBlocos != NULL){
            *numBlocos = 0;
        }
    }

    if(numBlocos != NULL){
        *numBlocos = blocoIdx;
    }

    return blocos;
}

ParTask* obterParesTaskN5(Task *caminhoCritico, int tamCaminho, BlocoCritico *blocosC, int numBlocos, int *numPares){
    
    if(caminhoCritico == NULL || tamCaminho <= 1 || blocosC == NULL || numBlocos <= 0){
        printf("Caminho critico invalido ou numero de blocos invalido");
        return NULL;
    }

    int numParesMax = 2 * numBlocos - 2;

    ParTask *pares = (ParTask*)malloc(numParesMax * sizeof(ParTask));

    if(pares == NULL){
        if(numPares != NULL) *numPares = 0;

        return NULL;
    }

    int j = 0;

    for(int i = 0; i < numBlocos ; i++){
        if(i == 0){
            pares[j].op1 = clonarTask(&caminhoCritico[blocosC[i].fim - 1]);
            pares[j].op2 = clonarTask(&caminhoCritico[blocosC[i].fim]);
            j++;
        }else if(i == numBlocos - 1){
            pares[j].op1 = clonarTask(&caminhoCritico[blocosC[i].inicio]);
            pares[j].op2 = clonarTask(&caminhoCritico[blocosC[i].inicio + 1]);
            j++;
        }else{
            pares[j].op1 = clonarTask(&caminhoCritico[blocosC[i].inicio]);
            pares[j].op2 = clonarTask(&caminhoCritico[blocosC[i].inicio + 1]);
            j++;
            
            if(blocosC[i].tam > 2){
                pares[j].op1 = clonarTask(&caminhoCritico[blocosC[i].fim - 1]);
                pares[j].op2 = clonarTask(&caminhoCritico[blocosC[i].fim]);
                j++;    
            }
        }
    }


    if(numPares != NULL){
        *numPares = j;
    }

    return pares;
}

Solution* trocaN5(Solution *solution, ParTask *pares){

    if(solution == NULL || pares == NULL){
        printf("Erro: ponteiros nulos na funcao trocaN5\n");
        return NULL;
    }
    
    if(pares->op1.machine != pares->op2.machine) return NULL;

    int m = pares->op1.machine;
    Solution *auxSolution = clonarSolution(solution);

    int pos1 = obterPosicaoNaMaquina(auxSolution, &pares->op1);
    int pos2 = obterPosicaoNaMaquina(auxSolution, &pares->op2);

    if(pos1 == -1 || pos2 == -1){
        liberarSolucao(auxSolution);
        return NULL;
    }

    int dif = pos1 - pos2;
    if(dif != 1 && dif != -1){
        liberarSolucao(auxSolution);
        return NULL;
    }

    int posA = (pos1 < pos2) ? pos1 : pos2;
    int posB = (pos1 > pos2) ? pos1 : pos2;

    swap(auxSolution, posA, posB, m);

    int viavel = recalcularSolucaoSeletiva(auxSolution, m, posA, posB);

    if(!viavel){
        liberarSolucao(auxSolution);
        return NULL;
    }

    return auxSolution;
    
}

Solution* buscaLocalN5exaustiva(Solution *solution, int iter){

    if(solution == NULL || solution->caminhoCritico == NULL || solution->tamCaminhoCritico <= 0){
        return solution;
    }

    int numBlocos = 0;
    BlocoCritico *blocos = obterBlocos(solution->caminhoCritico, solution->tamCaminhoCritico, &numBlocos);

    if(blocos == NULL || numBlocos <= 0){
        printf("Blocos nulos ou numero de blocos igual a 0!");
        return solution;
    }else if(blocos == NULL && numBlocos == INT_MAX){
        printf("Limite inferior atingido para a vizinhanca N5");
        return solution;
    }

    int numPares = 0;
    ParTask *pares = obterParesTaskN5(solution->caminhoCritico, solution->tamCaminhoCritico, blocos, numBlocos, &numPares);

    free(blocos);

    if(pares == NULL){
        printf("Pares nulos\n!");
        return solution;
    }else if(numPares <= 0){
        printf("Numero de pares igual a 0!\n");
        return solution;
    }

    Solution *melhorSolution = NULL;
    ParTask melhorPar;
    int melhorMakespan = solution->maxMakespan;

    for(int i = 0; i < numPares; i++){
        Solution *auxSolution = trocaN5(solution, &pares[i]);

        if(auxSolution != NULL){
            if(auxSolution->maxMakespan < melhorMakespan){
                if(melhorSolution != NULL){
                    liberarSolucao(melhorSolution);
                }

                melhorSolution = auxSolution;
                melhorMakespan = auxSolution->maxMakespan;
                melhorPar = pares[i];
            }else{
                liberarSolucao(auxSolution);
            }
        }
    }

    free(pares);

    if(melhorSolution != NULL && melhorMakespan < solution->maxMakespan){
        printf("[Iter %d] Melhoria encontrada (Swap T%dJ%d -> T%dJ%d): Makespan %d -> %d\n",
                    iter, melhorPar.op1.operation, melhorPar.op1.jobId, melhorPar.op2.operation, 
                    melhorPar.op2.jobId, solution->maxMakespan, melhorSolution->maxMakespan);
        
        
        liberarSolucao(solution);
        return melhorSolution;
    }

    if(melhorSolution != NULL){
        liberarSolucao(melhorSolution);
    }

    return solution;
}

Cromossomo* inicializarCromossomoSPTTWKR(Job *jobs, int numJobs, Machine *machines, int numMachines){

    if(jobs == NULL || machines == NULL || numJobs <= 0 || numMachines <= 0){
        return NULL;
    }

    Job *auxJobs = (Job*)malloc(numJobs * sizeof(Job));
    for(int j = 0; j < numJobs; j++){
        auxJobs[j].id = j;
        auxJobs[j].numTasks = numMachines;
        auxJobs[j].operationAtual = 0;
        auxJobs[j].workRemaining = jobs[j].workRemaining;
        auxJobs[j].tasks = (Task*)malloc(numMachines * sizeof(Task));
        for(int op = 0; op < numMachines; op++){
            auxJobs[j].tasks[op] = clonarTask(&jobs[j].tasks[op]);
        }
    }

    Machine *auxMachines = (Machine*)malloc(numMachines * sizeof(Machine));
    for(int m = 0; m < numMachines; m++){
        auxMachines[m].id = m;
        auxMachines[m].tempoAtual = 1;
        auxMachines[m].numTasks = 0;
        auxMachines[m].disponivel = 0;
        auxMachines[m].tasks = (Task*)malloc(numJobs * sizeof(Task)); // <-- ALOCA ESPAÇO PARA AS TAREFAS
    }
    
    int tamGenes = numJobs * numMachines;

    Cromossomo *cromossomo = calloc(1, sizeof(Cromossomo));
    cromossomo->genes = (int*)malloc(tamGenes * sizeof(int));

    int numCandidatos = numJobs/2;
    int minMachineId = INT_MIN;  
    int minJobId = INT_MIN;  
    int topJobId[numCandidatos];
    float topIndZ[numCandidatos];

    for(int a = 0; a < numCandidatos; a++){
        topIndZ[a] = INT_MAX;
        topJobId[a] = INT_MIN;
    }
    

    int i = 0;
    while(i < tamGenes){
        
        minMachineId = maquinaCritica(auxJobs, numJobs, auxMachines);

        for(int j = 0; j < numJobs; j++){
                
            if(auxJobs[j].tasks[auxJobs[j].operationAtual].machine == minMachineId && auxJobs[j].operationAtual < auxJobs[j].numTasks){

                float taskIndZ = (float)auxJobs[j].tasks[auxJobs[j].operationAtual].duration/auxJobs[j].workRemaining;
                for(int k = 0; k < numCandidatos; k++){
                    if(taskIndZ < topIndZ[k]){

                        for(int l = numCandidatos - 1; l > k; l--){
                            topIndZ[l] = topIndZ[l - 1];
                            topJobId[l] = topJobId[l - 1];
                        }
                        topIndZ[k] = taskIndZ;
                        topJobId[k] = auxJobs[j].id;
                    }
                }
            }
        }

        int countCandidatos = 0;
        for(int c = 0; c < numCandidatos; c++){
            if(topJobId[c] != INT_MIN){
                countCandidatos++;
            }
        }
        if(countCandidatos == 0){
            printf("Erro: Nenhum candidato encontrado para a maquina %d\n", minMachineId);
            break;
        }

        int randId = rand() % countCandidatos;
        while(topJobId[randId] == INT_MIN){
            randId = rand() % countCandidatos;
        };

        minJobId = topJobId[randId];

        int machineFreeTime = auxMachines[minMachineId].tempoAtual;

        int jobFreeTime = (auxJobs[minJobId].operationAtual > 0)                                    //if(auxJobs[minJobId].operationAtual > 0) 
                           ? auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime  //jobFreeTime = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual - 1].endTime
                           : 1;                                                                     //else jobFreeTime = 0
                
        int realStartTime = (machineFreeTime > jobFreeTime) ? machineFreeTime : jobFreeTime;

        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].startTime = realStartTime;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].endTime = realStartTime + auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;
        auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].sequencia = i;
        auxJobs[minJobId].workRemaining -= auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual].duration;

        auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks] = auxJobs[minJobId].tasks[auxJobs[minJobId].operationAtual];
        auxMachines[minMachineId].tempoAtual = realStartTime + auxMachines[minMachineId].tasks[auxMachines[minMachineId].numTasks].duration;

        auxMachines[minMachineId].numTasks++;
        auxJobs[minJobId].operationAtual++;

        cromossomo->genes[i] = minJobId;

        i++;

        minMachineId = INT_MIN;
        minJobId = INT_MIN;
        randId = INT_MIN;
        for(int b = 0; b < numCandidatos; b++){
            topIndZ[b] = INT_MAX;
            topJobId[b] = INT_MIN;
        }
    }

    for(int j = 0; j < numJobs; j++){
        free(auxJobs[j].tasks);
    }
    free(auxJobs);

    for(int m = 0; m < numMachines; m++){
        free(auxMachines[m].tasks);
    }
    free(auxMachines);

    return cromossomo;
}

void fisherYatesShuffle(int *array, int size){

    if(array == NULL || size <= 1) return;

    for (int i = size - 1; i > 0; i--){
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

Cromossomo* inicializarCromossomoRandom(Job *jobs, int numJobs, Machine *machines, int numMachines){

    if(jobs == NULL || machines == NULL || numJobs <= 0 || numMachines <= 0){
        return NULL;
    }

    int tamGenes = numJobs * numMachines;

    Cromossomo *cromossomo = calloc(1, sizeof(Cromossomo));
    cromossomo->genes = (int*)malloc(tamGenes * sizeof(int));

    for(int i = 0; i < numJobs; i++){
        for(int j = 0; j < numMachines; j++){
            cromossomo->genes[i * numMachines + j] = i;
        }
    }

    fisherYatesShuffle(cromossomo->genes, tamGenes);

    return cromossomo;
}

Solution* decodificacaoGT(Job *jobs, int numJobs, Machine *machines, int numMachines, Cromossomo *cromossomo){

    if(jobs == NULL || machines == NULL || cromossomo == NULL || numJobs <= 0 || numMachines <= 0){
        return NULL;
    }

    int numTasks = numJobs * numMachines;

    Solution *solution = iniciarSolucaoVazia(jobs, numJobs, machines, numMachines, numTasks);

    int opAtual[numJobs];
    int tempoFimJob[numJobs];

    for(int j = 0; j < numJobs; j++){
        opAtual[j] = 0;
        tempoFimJob[j] = 1; // Base de tempo 1 do projeto
    }
    int tempoFimMaq[numMachines];
    for(int m = 0; m < numMachines; m++){
        tempoFimMaq[m] = 1;
    }
    int geneConsumido[numTasks];
    for(int g = 0; g < numTasks; g++){
        geneConsumido[g] = 0;
    }

    int ponteiroGene = 0;

    for(int scheduled = 0; scheduled < numTasks; scheduled++){
        // Passo A: Identificar o conjunto S e calcular ES e EC de cada operação pronta
        int es[numJobs];
        int ec[numJobs];
        int minEC = INT_MAX;
        int minMachine = -1;

        for(int j = 0; j < numJobs; j++){
            if(opAtual[j] < numMachines){
                int op = opAtual[j];
                int m = jobs[j].tasks[op].machine;
                int dur = jobs[j].tasks[op].duration;
                int startJob = tempoFimJob[j];
                int startMaq = tempoFimMaq[m];
                es[j] = (startJob > startMaq) ? startJob : startMaq;
                ec[j] = es[j] + dur;
                if(ec[j] < minEC){
                    minEC = ec[j];
                    minMachine = m;
                }
            }else{
                es[j] = INT_MAX;
                ec[j] = INT_MAX;
            }
        }

        // Passo B: Formar o conjunto de conflito C na máquina minMachine
        // emC[j] = 1 se a próxima operação do Job j concorre nessa máquina e pode iniciar antes de minEC
        int emC[numJobs];
        for(int j = 0; j < numJobs; j++){
            emC[j] = 0;
            if(opAtual[j] < numMachines){
                int op = opAtual[j];
                if(jobs[j].tasks[op].machine == minMachine && es[j] < minEC){
                    emC[j] = 1;
                }
            }
        }

        // Passo C: Desempate por Prioridade no Cromossomo
        // Encontra o primeiro gene NÃO consumido que pertença ao conjunto C
        int jobEscolhido = -1;
        int idxGeneEscolhido = -1;

        for(int g = ponteiroGene; g < numTasks; g++){
            if(!geneConsumido[g]){
                int jCandidato = cromossomo->genes[g];
                if(emC[jCandidato]){
                    jobEscolhido = jCandidato;
                    idxGeneEscolhido = g;
                    break;
                }
            }
        }

        // Marca o gene selecionado como consumido
        geneConsumido[idxGeneEscolhido] = 1;
        while(ponteiroGene < numTasks && geneConsumido[ponteiroGene]){
            ponteiroGene++;
        }

        // Passo D: Agendar a operação escolhida
        int op = opAtual[jobEscolhido];
        int m = jobs[jobEscolhido].tasks[op].machine;
        int dur = jobs[jobEscolhido].tasks[op].duration;
        int realStart = es[jobEscolhido];
        int realEnd = realStart + dur;

        // Atualiza a Task no Job da Solution
        solution->jobs[jobEscolhido].tasks[op].startTime = realStart;
        solution->jobs[jobEscolhido].tasks[op].endTime = realEnd;
        solution->jobs[jobEscolhido].tasks[op].sequencia = scheduled;

        // Registra a Task na Máquina da Solution
        int idxNaMaq = solution->machines[m].numTasks;
        solution->machines[m].tasks[idxNaMaq] = clonarTask(&solution->jobs[jobEscolhido].tasks[op]);
        solution->machines[m].numTasks++;
        solution->machines[m].tempoAtual = realEnd;

        // Atualiza estados locais
        tempoFimJob[jobEscolhido] = realEnd;
        tempoFimMaq[m] = realEnd;
        opAtual[jobEscolhido]++;
        solution->jobs[jobEscolhido].operationAtual++;
    }

    // 4. Calcula scores (Makespan, Flowtime, Idletime) e constrói o Caminho Crítico
    calcularScore(solution);
    caminhoCritico(solution);

    // 5. Atualiza o fitness do cromossomo (Makespan minimizado)
    return solution;
}

Populacao* inicializarPopulacao(int numIndividuos, int numCandidatos, Job *jobs, int numJobs, Machine *machines, int numMachines){

    Populacao* populacao = (Populacao*)malloc(sizeof(Populacao));
    populacao->individuos = (Cromossomo*)malloc(numIndividuos * sizeof(Cromossomo));
    populacao->melhoresIndividuosGeracao = (Cromossomo*)calloc(numCandidatos, sizeof(Cromossomo)); 
    populacao->melhorSolucao = NULL;
    populacao->numIndividuos = numIndividuos;

    int numRandom = numIndividuos * 0.7;
    int i = 0;

    for(i = 0; i < numRandom; i++){
        Cromossomo *cromossomo = inicializarCromossomoRandom(jobs, numJobs, machines, numMachines);
        populacao->individuos[i] = *cromossomo;
        free(cromossomo);
    }
    for(i = numRandom; i < numIndividuos; i++){
        Cromossomo *cromossomo = inicializarCromossomoSPTTWKR(jobs, numJobs, machines, numMachines);
        populacao->individuos[i] = *cromossomo;
        free(cromossomo);
    }

    return populacao;
}

// Função comparadora para o qsort (ordem crescente de makespan)
int compararCromossomos(const void *a, const void *b){
    const Cromossomo *c1 = (const Cromossomo*)a;
    const Cromossomo *c2 = (const Cromossomo*)b;
    
    // Tratamento de segurança caso algum indivíduo não tenha solução decodificada
    if(c1->solucao == NULL && c2->solucao == NULL) return 0;
    if(c1->solucao == NULL) return 1;   // Indivíduos sem solução vão para o final
    if(c2->solucao == NULL) return -1;
    
    int ms1 = c1->solucao->maxMakespan;
    int ms2 = c2->solucao->maxMakespan;
    
    if(ms1 < ms2) return -1;
    if(ms1 > ms2) return 1;
    return 0;
}
// Função para ordenar o vetor de indivíduos da população
void ordenarPopulacao(Populacao *populacao){
    if(populacao == NULL || populacao->individuos == NULL || populacao->numIndividuos <= 1){
        return;
    }
    
    qsort(populacao->individuos, populacao->numIndividuos, sizeof(Cromossomo), compararCromossomos);
}

Cromossomo* crossoverPOX(Cromossomo *pai1, Cromossomo *pai2, Job *jobs, int numJobs, Machine *machines, int numMachines){

    if(pai1 == NULL || pai2 == NULL || jobs == NULL || machines == NULL || numJobs <= 0 || numMachines <= 0){
        return NULL;
    }

    int tamGenes = numJobs * numMachines;

    Cromossomo *filho = (Cromossomo*)malloc(sizeof(Cromossomo));
    filho->genes = (int*)malloc(tamGenes * sizeof(int));
    for(int f = 0; f < tamGenes; f++){
        filho->genes[f] = -1;
    }

    int *genesP1 = pai1->genes;
    int *genesP2 = pai2->genes;
    

    int *jobset = malloc(numJobs * sizeof(int));
    for(int i = 0; i < numJobs; i++){
        jobset[i] = i;
    }

    int jobsetSize1 = numJobs / 2;
    int *jobset1 = (int*)malloc(jobsetSize1 * sizeof(int));
    
    int jobsetSize2 = numJobs - jobsetSize1;
    int *jobset2 = (int*)malloc(jobsetSize2 * sizeof(int));

    int i = 0;
    int j = 0;
    int randJ;
    while(i < jobsetSize1){
        randJ = rand() % numJobs;
        if(jobset[randJ] != -1){
            jobset1[i] = jobset[randJ];
            jobset[randJ] = -1;
            i++;
        }
    }

    for(i = 0, j = 0; i < numJobs; i++){
        if(jobset[i] != -1){
            jobset2[j] = jobset[i];
            j++;
        }
    }

    for(i = 0; i < tamGenes; i++){
        for(j = 0; j < jobsetSize1; j++){
            if(genesP1[i] == jobset1[j]){
                filho->genes[i] = genesP1[i];
                break;
            }
        }
    }

    for(i = 0; i < tamGenes; i++){
        if(filho->genes[i] == -1){
            for(j = 0; j < jobsetSize2; j++){
                if(genesP2[i] == jobset2[j]){
                    filho->genes[i] = genesP2[i];
                    genesP2[i] = -1;
                    break;
                }
            }
        }
    }
    
    return filho;
}

Cromossomo* crossoverLOX(Cromossomo *pai1, Cromossomo *pai2, Job *jobs, int numJobs, Machine *machines, int numMachines){
    
    if(pai1 == NULL || pai2 == NULL || jobs == NULL || machines == NULL || numJobs <= 0 || numMachines <= 0){
        return NULL;
    }

    int tamGenes = numJobs * numMachines;

    Cromossomo *filho = (Cromossomo*)malloc(sizeof(Cromossomo));
    filho->genes = (int*)malloc(tamGenes * sizeof(int));
    for(int f = 0; f < tamGenes; f++){
        filho->genes[f] = -1;
    }

    int *genesP1 = pai1->genes;
    int *genesP2 = pai2->genes;

    int pc1 = rand() % ((tamGenes/2) - 1);

    double perc2 = (40 + rand() % 21) / 100.0;
    int pc2 = (tamGenes*perc2 + pc1 > tamGenes) ? tamGenes - 1 : (tamGenes*perc2 + pc1);

    for(int i = pc1; i <= pc2; i++){
        filho->genes[i] = genesP1[i];
        for(int j = 0; j < tamGenes; j++){
            if(genesP2[j] == genesP1[i]){
                genesP2[j] = -1;
                break;
            }
        }
    }

    for(int i = 0; i < tamGenes; i++){
        if(filho->genes[i] == -1){
            for(int j = 0; j < tamGenes; j++){
                if(genesP2[j] != -1){
                    filho->genes[i] = genesP2[j];
                    genesP2[j] = -1;
                    break;
                }
            }
        }
    }
    
    return filho;
}

Solution* algoritmoMemetico(Job *jobs, int numJobs, Machine *machines, int numMachines, int numIndividuos, int numIteracoes){
    
    int numCandidatos = numIndividuos * 0.2;
    float limCross = 0.50f; // Limite de probabilidade de crossover entre POX e LOX (POX se probCross < limCross / LOX se probCross > limCross / POX e LOX se probCross = limCross)

    Populacao *populacao = inicializarPopulacao(numIndividuos, numCandidatos, jobs, numJobs, machines, numMachines);

    for(int i = 0; i < numIndividuos; i++){

        if(populacao->individuos[i].genes == NULL){
            printf("Erro na inicializacao do individuo %d\n", i);
            return NULL;
        }

        if(populacao->individuos[i].solucao == NULL){
            populacao->individuos[i].solucao = decodificacaoGT(jobs, numJobs, machines, numMachines, &populacao->individuos[i]);
            if(populacao->individuos[i].solucao == NULL){
                printf("Erro na decodificacao do individuo %d\n", i);
                return NULL;
            }
        }else continue;
    }

    ordenarPopulacao(populacao);

    Cromossomo *pais = (Cromossomo*)malloc(numCandidatos * sizeof(Cromossomo));
    Cromossomo *filhosPOX = (Cromossomo*)malloc(numCandidatos * 2 * sizeof(Cromossomo));
    Cromossomo *filhosLOX = (Cromossomo*)malloc(numCandidatos * 2 * sizeof(Cromossomo));

    for(int c = 0; c < numCandidatos; c++){
        pais[c] = populacao->individuos[c];
    }
    for(int px = 0; px < numCandidatos * 2; px++){
        filhosPOX[px].solucao = NULL;
    }
    for(int lx = 0; lx < numCandidatos * 2; lx++){
        filhosLOX[lx].solucao = NULL;
    }

    int numPais = numCandidatos / 2;
    int p = 0;
    
    while(p < numPais){
        for(int i = 0; i < numCandidatos; i++){
            if(pais[i].solucao != NULL){
                Cromossomo *pai1 = &pais[i];
                pais[i].solucao = NULL;

                int randP2 = rand() % numCandidatos;
                while(pais[randP2].solucao == NULL){
                    randP2 = rand() % numCandidatos;
                }
                Cromossomo *pai2 = &pais[randP2];
                pais[randP2].solucao = NULL;

                float probCross = (float)(rand()) / (float)RAND_MAX;

                if(probCross < limCross){
                    Cromossomo *filho1 = crossoverPOX(pai1, pai2, jobs, numJobs, machines, numMachines);
                    Cromossomo *filho2 = crossoverPOX(pai2, pai1, jobs, numJobs, machines, numMachines);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho1);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho2);
                    for(int k = 0; k < numCandidatos * 2; k++){
                        if(filhosPOX[k].solucao == NULL){
                            filhosPOX[k] = *filho1;
                            free(filho1);

                            filhosPOX[k + 1] = *filho2;
                            free(filho2);
                            
                            break;
                        }
                    }
                }else if(probCross > limCross){
                    Cromossomo *filho1 = crossoverLOX(pai1, pai2, jobs, numJobs, machines, numMachines);
                    Cromossomo *filho2 = crossoverLOX(pai2, pai1, jobs, numJobs, machines, numMachines);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho1);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho2);
                    for(int k = 0; k < numCandidatos * 2; k++){
                        if(filhosLOX[k].solucao == NULL){
                            filhosLOX[k] = *filho1;
                            free(filho1);

                            filhosLOX[k + 1] = *filho2;
                            free(filho2);
                            
                            break;
                        }
                    }
                }else{// probCross == limCross
                    Cromossomo *filho1 = crossoverPOX(pai1, pai2, jobs, numJobs, machines, numMachines);
                    Cromossomo *filho2 = crossoverLOX(pai2, pai1, jobs, numJobs, machines, numMachines);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho1);
                    decodificacaoGT(jobs, numJobs, machines, numMachines, filho2);
                    for(int k = 0; k < numCandidatos * 2; k++){
                        if(filhosPOX[k].solucao == NULL){
                            filhosPOX[k] = *filho1;
                            free(filho1);
                            
                            break;
                        }
                    }
                    for(int k = 0; k < numCandidatos * 2; k++){
                        if(filhosLOX[k].solucao == NULL){
                            filhosLOX[k] = *filho2;
                            free(filho2);
                            
                            break;
                        }
                    }
                }
            }

        }


    }

    if(populacao->melhorSolucao == NULL || populacao->melhoresIndividuosGeracao[0].solucao->maxMakespan < populacao->melhorSolucao->maxMakespan){
        populacao->melhorSolucao = populacao->melhoresIndividuosGeracao[0].solucao;
    }


    Solution *melhorSolucao = populacao->melhorSolucao;
    
    liberarPopulacao(populacao, melhorSolucao);
    
    return melhorSolucao;
}

int main (int argc, char *argv[]){
    
    clock_t inicio = clock();
    srand(time(NULL));

    if (argc < 3) {
        printf("Uso incorreto!\n");
        printf("Sintaxe esperada: %s <caminho_do_arquivo> <caminho_saida>\n", argv[0]);
        return 1;
    }

    int numJobs = 0;
    int numMachines = 0;
    Job *jobs = NULL;
    Machine *machines = NULL;
    Solution *solution = NULL;

    char *caminho_arquivo = argv[1];

    iniciarInstancias(caminho_arquivo, &numJobs, &numMachines, &jobs, &machines);

    if(jobs == NULL) {
        return 1;
    }

    //printJobs(jobs, numJobs, numMachines);

    solution = algoritmoMemetico(jobs, numJobs, machines, numMachines, 1000, 1000);
    //printMachines(result, numMachines);

    printf("Makespan decodificado via Populacao Inicial Memetico: %d\n", solution->maxMakespan);
    printCaminhoCritico(solution->caminhoCritico, solution->tamCaminhoCritico, solution->maxMakespan);

    //unsigned int seed = (unsigned int)time(NULL);

    char *caminho_saida = argv[2];

    FILE *arquivoSaida = fopen(caminho_saida, "w");

    if (arquivoSaida != NULL) {
        printSolutionToFile(arquivoSaida, solution);
        printCaminhoCriticoToFile(arquivoSaida, solution->caminhoCritico, solution->tamCaminhoCritico, solution->maxMakespan);

        fclose(arquivoSaida);
        
        printf("Resultado salvo com sucesso no arquivo %s!\n", caminho_saida);
    } else {
        printf("Erro ao criar o arquivo de saida.\n");
    }


    clock_t fim = clock();
    double tempo_execucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("Tempo de execucao: %.6f segundos\n", tempo_execucao);

    // Libera memória alocada
    if (solution != NULL){
        liberarSolucao(solution);
    }

    return 0;
}