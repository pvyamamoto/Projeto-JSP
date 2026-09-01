#include <stdio.h>
#include <stdlib.h>

struct Task { 
    int operation;
    int machine;
    int duration;
} Task;

struct Job {
    int jobId;
    Task* tasks;  
} Job;

