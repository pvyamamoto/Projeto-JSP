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

Solution* swap(Solution *solution, Task *t1, Task *t2, int pos1, int pos2, int m){

    Task temp = solution->machines[m].tasks[pos1];
    solution->machines[m].tasks[pos1] = solution->machines[m].tasks[pos2];
    solution->machines[m].tasks[pos2] = temp;

    return solution;
}

int obterNumBlocosCriticos(Task *caminhoCritico, int tamCaminho){

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

BlocoCritico* obterBlocosCriticos(Task *caminhoCritico, int tamCaminho, int *numBlocos){

    int numBlocosT = obterNumBlocosCriticos(caminhoCritico, tamCaminho);

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

    numBlocos = blocoIdx;

    return blocos;
}

ParTask* obterParesTaskN5(Task *caminhoCritico, int tamCaminho, BlocoCritico *blocosC, int numBlocos, int *numParesC){
    
    if(caminhoCritico == NULL || tamCaminho <= 1 || blocosC == NULL || numBlocos <= 0){
        printf("Caminho critico invalido ou numero de blocos invalido");
        return NULL;
    }

    BlocoCritico *blocos = (BlocoCritico*)malloc(numBlocos * sizeof(BlocoCritico));

    blocos = clonarBlocoCritico(blocosC, numBlocos);

    int numPares = 0;

    if(numBlocos < 2){
        printf("Makespan da soluçao já atingiu o limite inferior da maquina!");
        return NULL;
    }else if(numBlocos == 2){
        numPares = 2;
    }else{
        numPares = (pow(2, numBlocos - 2) + 2);
    }

    ParTask *pares = (ParTask*)malloc(numPares * sizeof(ParTask));

    if(numBlocos > 2){
        for(int i = 0, j = 0; i < numBlocos, j < numPares ; i++){
            if(i == 0){
                pares[j].op1 = clonarTask(&caminhoCritico[blocos[i].fim - 1]);
                pares[j].op2 = clonarTask(&caminhoCritico[blocos[i].fim]);
                j++;
            }else if(i == numBlocos - 1){
                pares[j].op1 = clonarTask(&caminhoCritico[blocos[i].inicio]);
                pares[j].op2 = clonarTask(&caminhoCritico[blocos[i].inicio + 1]);
                j++;
            }else{
                pares[j].op1 = clonarTask(&caminhoCritico[blocos[i].inicio]);
                pares[j].op1 = clonarTask(&caminhoCritico[blocos[i].inicio + 1]);
                j++;
                
                pares[j].op1 = clonarTask(&caminhoCritico[blocos[i].fim - 1]);
                pares[j].op2 = clonarTask(&caminhoCritico[blocos[i].fim]);
                j++;
            }
        }    
    }else{
        pares[0].op1 = clonarTask(&caminhoCritico[blocos[0].inicio]);
        pares[0].op2 = clonarTask(&caminhoCritico[blocos[0].inicio + 1]);

        pares[1].op1 = clonarTask(&caminhoCritico[blocos[1].fim - 1]);
        pares[1].op2 = clonarTask(&caminhoCritico[blocos[1].fim]);
    }

    free(blocos);

    numParesC = &numPares;

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

    if(pos1 == -1 || pos2 == -1) return NULL;

    int dif = pos1 - pos2;
    if(dif != 1 && dif != -1){
        printf("Erro: posicoes nao adjacentes na funcao trocaN1\n");
        return NULL;
    }

    int posA = (pos1 < pos2) ? pos1 : pos2;
    int posB = (pos1 > pos2) ? pos1 : pos2;

    swap(auxSolution, &pares->op1, &pares->op2, posA, posB, m);

    int viavel = recalcularSolucaoSeletiva(auxSolution, m, posA, posB);

    if(!viavel){
        free(auxSolution);
        return NULL;
    }

    return auxSolution;
    
}

Solution* buscaLocalN5(Solution *solution, int iter){

    if(solution == NULL || solution->caminhoCritico == NULL || solution->tamCaminhoCritico <= 0){
        return solution;
    }

    int numBlocos = 0;

    BlocoCritico *blocos = obterBlocosCriticos(solution->caminhoCritico, solution->tamCaminhoCritico, &numBlocos);

    if(blocos == NULL || numBlocos <= 0){
        return NULL;
    }

    int numPares = 0;
    ParTask *pares = obterParesTaskN5(solution->caminhoCritico, solution->tamCaminhoCritico, blocos, numBlocos, &numPares);

    for(int i = 0; i < numPares; i++){
        Solution *auxSolution = trocaN5(solution, &pares[i]);
    }
}

Solution* trocaN1(Solution *solution, Task *t1, Task *t2){

    if(solution == NULL || t1 == NULL || t2 == NULL){
        printf("Erro: ponteiros nulos na funcao trocaN1\n");
        return NULL;
    }

    if(t1->machine != t2->machine){
        printf("Erro: maquinas diferentes na funcao trocaN1\n");
        return NULL;
    }

    int m = t1->machine;
    Solution *auxSolution = clonarSolution(solution);

    int pos1 = obterPosicaoNaMaquina(auxSolution, t1);
    int pos2 = obterPosicaoNaMaquina(auxSolution, t2);

    if(pos1 == -1 || pos2 == -1){
        return NULL;
    }

    int dif = pos1 - pos2;
    if(dif != 1 && dif != -1){
        printf("Erro: posicoes nao adjacentes na funcao trocaN1\n");
        return NULL;
    }

    int posA = (pos1 < pos2) ? pos1 : pos2;
    int posB = (pos1 > pos2) ? pos1 : pos2;

    swap(auxSolution, t1, t2, posA, posB, m);

    int viavel = recalcularSolucaoSeletiva(auxSolution, m, posA, posB);

    if(!viavel){
        free(auxSolution);
        return NULL;
    }

    return auxSolution;
}

Solution* buscaLocalN1(Solution *solution, unsigned int seed, int iter){

    if(solution == NULL || solution->caminhoCritico == NULL || solution->tamCaminhoCritico <= 0){
        return solution;
    }

    srand(seed);
    int minRange = 0;
    int maxRange = solution->tamCaminhoCritico - 1;

    int idxopAtu = minRange + rand() % (maxRange - minRange + 1);
        
    Task *opAtu = &solution->caminhoCritico[idxopAtu];
    Task *opAnt = NULL;
    Task *opPos = NULL;

    obterAdjacentesOpMaq(solution->caminhoCritico, solution->tamCaminhoCritico, idxopAtu, &opAnt, &opPos);

    Solution *solutionAnt = (opAnt != NULL) ? trocaN1(solution, opAtu, opAnt) : NULL;
    Solution *solutionPos = (opPos != NULL) ? trocaN1(solution, opAtu, opPos) : NULL;

    ParTask swapAnt = (opAnt != NULL) ? (ParTask){*opAtu, *opAnt} : (ParTask){0};
    ParTask swapPos = (opPos != NULL) ? (ParTask){*opAtu, *opPos} : (ParTask){0};

    Solution *melhorCandidato = NULL;
    ParTask melhorSwap = {0};

    if(solutionAnt != NULL && solutionPos != NULL){
        if(solutionAnt->maxMakespan < solutionPos->maxMakespan){
            melhorCandidato = solutionAnt;
            melhorSwap = swapAnt;

            liberarSolucao(solutionPos);
        }else{
            melhorCandidato = solutionPos;
            melhorSwap = swapPos;
            liberarSolucao(solutionAnt);
        }
    }else if(solutionAnt != NULL){
        melhorCandidato = solutionAnt;
        melhorSwap = swapAnt;
    }else if(solutionPos != NULL){
        melhorCandidato = solutionPos;
        melhorSwap = swapPos;
    }else{
        liberarSolucao(solutionAnt);
        liberarSolucao(solutionPos);
        liberarSolucao(melhorCandidato);
        return solution;
    }
    
    if(melhorCandidato != NULL){
        if(melhorCandidato->maxMakespan < solution->maxMakespan){
            printf("[Iter %d] Melhoria encontrada (Swap T%dJ%d -> T%dJ%d): Makespan %d -> %d\n", iter, melhorSwap.op1.operation, melhorSwap.op1.jobId, melhorSwap.op2.operation, melhorSwap.op2.jobId, solution->maxMakespan, melhorCandidato->maxMakespan);
            liberarSolucao(solution);
            return melhorCandidato;
        } else {
            liberarSolucao(melhorCandidato);
            return solution;
        }
    }
    return solution;
}

int main (int argc, char *argv[]){
    
    clock_t inicio = clock();

    if (argc < 4) {
        printf("Uso incorreto!\n");
        printf("Sintaxe esperada: %s <caminho_do_arquivo> <caminho_saida> <num_iteracoes>\n", argv[0]);
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

    //printJobs(jobs, numJobs, numMachines);

    solution = spt_twkr(jobs, numJobs, machines, numMachines);
    //printMachines(result, numMachines);

    calcularScore(solution);
    caminhoCritico(solution);

    int numIter = atoi(argv[3]);
    unsigned int seed = (unsigned int)time(NULL);

    for(int iter = 0; iter < numIter; iter++){
        solution = buscaLocalN1(solution, seed + iter, iter);
    }

    char *caminho_saida = argv[2];

    FILE *arquivoSaida = fopen(caminho_saida, "w");

    if (arquivoSaida != NULL) {
        printSolutionToFile(arquivoSaida, solution);
        printCaminhoCriticoToFile(arquivoSaida, solution->caminhoCritico, solution->tamCaminhoCritico, solution->maxMakespan);

        fclose(arquivoSaida);
        
        printf("Resultado salvo com sucesso no arquivo %s!\n", caminho_saida);
    } else {
        printf("Erro ao criar o arquivo de saída.\n");
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