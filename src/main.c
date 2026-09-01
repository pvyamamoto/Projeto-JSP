#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Task { 
    int operation;
    int machine;
    int duration;
} Task;

typedef struct Job {
    int jobId;
    Task* tasks;
} Job;

Job* iniciarJob(const char *filePath){
    int numJobs, numMachines = 0;
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {

        perror("Erro ao abrir o arquivo de benchmark");
        
        return NULL;
    }

    numMachines = fgetc(file);
    numJobs = fgetc(file);

    Task *tasks = (Task*)malloc(numMachines * sizeof(Task));
    Job *jobs = (Job*)malloc(numJobs * sizeof(Job));

    

    fclose(file);
}