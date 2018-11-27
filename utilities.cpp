#include <stdio.h>
#include "utilities.h"

char * __strcat(const char *des,const char *source, int bufferSize){
    char *buffer = new char[bufferSize];
    int n = 0;
    while(des != nullptr && *des){
        buffer[n++] = *des++;
    }
    while(source != nullptr && *source){
        buffer[n++] = *source++;
    }

    buffer[n] = '\0';
    
    char *res = new char[n + 1];
    
    n = 0;
    while (buffer[n]){
        res[n] = buffer[n++];
    }
    delete[] buffer;

    res[n] = '\0';
    return res;
}


char * __strcpy(const char *source, int bufferSize){
    
    char *buffer = new char[bufferSize];
    
    int n = 0;
    while (buffer != nullptr && *source){
        buffer[n++] = *source++;
    }
    
    buffer[n] = '\0';
    char *res = new char[n+1];
    n = 0;
    while (buffer[n]){
        res[n] = buffer[n++];
    }
    
    delete[] buffer;
    res[n] = '\0';
    return res;
}