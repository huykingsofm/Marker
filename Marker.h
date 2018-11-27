/*
file marker.h
Author : Le Ngoc Huy - UIT 2017
Contact: 17520074@gm.uit.edu.vn
         huykingsofm@gmail.com

Purpose : Provide a platform to mark a solution

Feature: run the solution with
    + timer or no-timer
    + test sample or no-test
    + input and output by console or file

STRUCTURE:
    struct __MARKER_RESULT
    struct __MARKER_PROCESS
    struct __READ_OUTPUT_DATA
    class Marker
    See below.....
        
*/


#pragma once
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include "utilities.h"

const int bufferSize = 1024;
const int MAX_ALIVE_TIME_COMPARISOR = 10000; // 10s

struct __MARKER_RESULT{

    const static char* resultMessage[4];
    const static char* errorMessage[9];
    const static char* modeMessage[4];

    int exitCode;
    int mode;
    int result;
    time_t runningTime;
    
    void display(){
        printf("exitcode : %d(%s)    result : %s    time : %fs\n",
                    exitCode, 
                    errorMessage[exitCode], 
                    resultMessage[result], 
                    (float)runningTime/1000
            );
    }
};


struct __MARKER_PROCESS{
    const static int BAD_PIPE = 1;

    int exitCode;
    STARTUPINFOA siStartupInfo_Solution;
    HANDLE hInputRead;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
    HANDLE hOutputWrite; 
};

typedef struct __READ_OUTPUT_DATA{
    int exitcode;
    __DATA data;
    HANDLE pipe;
} __READ_OUTPUT_DATA, *P__READ_OUTPUT_DATA;

class Marker{
public:
    // Start marking
    __MARKER_RESULT mark();             
    
public:
// mode identify how marker work
    const static int MODE_NONE      = 0;
    const static int MODE_TIMER     = 1;    // turn on timer
                                            // require set limited time

    const static int MODE_TEST      = 2;    // checking test
                                            // require set test sample 

    const static int MODE_ACCURACY  = 4;    // return value is a accuracy of predicted output versus ground truth 
                                            // require mode_TEST is turn on

    // all mode
    const static int MODE_ALL = MODE_ACCURACY | MODE_TEST | MODE_TIMER;
    
    // Mode which is usually used
    const static int MODE_COMMON = MODE_TEST | MODE_TIMER;

// CONST result return when marker finishs successfully
    const static int RES_CORRECT             = 0;
    const static int RES_INCORRECT           = 1;
    const static int RES_TIME_LIMIT_EXCEED   = 2;
    const static int RES_UNKNOWN             = 3;

// CONST error return when the program is fail
    const static int ERR_NONE             = 0;
    const static int ERR_NO_SAMPLE        = 1;
    const static int ERR_NO_SOLUTION      = 2;
    const static int ERR_RUNTIME          = 3;
    const static int ERR_BAD_PIPE         = 4;
    const static int ERR_BAD_FORMAT       = 5;
    const static int ERR_BAD_SOLUTION     = 6;
    const static int ERR_NO_OUTPUT        = 7;
    const static int ERR_BAD_READ_OUTPUT  = 8;

// CONST input and output format
    const static int FORMAT_STD  = 1;
    const static int FORMAT_FILE = 2;
public:
// some constructor
    Marker();
    Marker(const char *sSolution);
    Marker(const char *sSolution, time_t tTime);
    Marker(const char *sSolution, const char *sInpSample, const char *sOutSample);
    Marker(const char *sSolution, const char *sInpSample, const char *sOutSample, time_t tTime);
    ~Marker();

public:
// some activities on mode
    void resetMode();               //reset default mode
    int  addMode(int mode);         //add a mode into marker
    int  removeMode(int mode);      //remove a mode from maker

public:
// add some neccessary parameter for marking
    void setSolution(const char *sSolutionExeFileName);         //set file solution .exe
    void setTestSample(const char *sTestInpFile,                //set testcase to check rightness
                       const char *sTestOutFile);                           
    void setOutPath(const char *sOutPath);                      //set path of OutPath
    int  setIOFormat(int inputFormat, int outputFormat);        //set format of reading and writing
    int  setTimer(time_t LimitedTime);                          //set timer to check time_exceed
    void setArgument(const char *sArgument);                    //set some arguments into your solution
    void setCompareFunc(int (* compare)(char [][255], int,        //set compare function to compare 2 results
                                      char[][255], int));

public:
// remove Argument
    void removeArgument();                                  // remove argument from command 

private:
    // Launch solution file to start marking
    __MARKER_PROCESS    initializeMarkerProcess();
    PROCESS_INFORMATION launchProcess(LPSTR commandLine,STARTUPINFOA &siStartupInfo);

private:
    bool isMode(int mode);

private:
    // mode decide how marker work
    int mode;

    // path of program which is a solution
    LPSTR SolutionPath;

    // [Option] Arguments of that solution
    LPSTR Arguments;
    
    // testcase
    int inpFormat;
    int outFormat;
    LPSTR InpSample;
    LPSTR OutSample;
    LPSTR OutPath;

    // timer
    time_t LimitedTime;

    int (*compare)(char[][255], int, char[][255], int);
};

DWORD WINAPI ReadOutput(LPVOID data);
int stdCompare(char sample[][255], int nSample, char out[][255], int nOut);
 