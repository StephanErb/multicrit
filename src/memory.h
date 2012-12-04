#include "stdlib.h"
#include "stdio.h"
#include "string.h"

// Source:
// http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

int parseStatusLine(char* line){
    int i = strlen(line);
    while (*line < '0' || *line > '9') line++;
    line[i-3] = '\0';
    i = atoi(line);
    return i;
}

int getCurrentMemorySize(){ // this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];


    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseStatusLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

int getPeakMemorySize(){ // this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];


    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmHWM:", 6) == 0){
            result = parseStatusLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}