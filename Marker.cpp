#include "Marker.h"

const char* __MARKER_RESULT::resultMessage[4] = {
                                    "Correct",
                                    "Incorrect", 
                                    "Time-limit-exceed",
                                    "Unknown"
                                    };

const char* __MARKER_RESULT::errorMessage[9] = {
                                    "No error",                  // ERR_NONE
                                    "Sample not found",          // ERR_NO_SAMPLE
                                    "Solution not found",        // ERR_NO_SOLUTION
                                    "Runtime Error",             // ERR_RUNTIME
                                    "Bad Pipe",                  // ERR_BAD_PIPE
                                    "Bad IO Format",             // ERR_BAD_FORMAT
                                    "Solution can't launch",     // ERR_BAD_SOLUTION
                                    "Output not found",          // ERR_NO_OUTPUT
                                    "Can't read from std output" // ERR_BAD_READ_OUTPUT
                                    };

Marker::Marker(){
    //default Parameter
    this->mode = 0;
    
    this->SolutionPath = NULL;
    this->Arguments     = NULL;
    this->LimitedTime = 1000; // timeout default is 1s

    this->inpFormat = 0;
    this->outFormat = 0;
    this->InpSample = NULL;
    this->OutSample = NULL;
    this->compare = &stdCompare;
}

Marker::Marker(const char *sSolution)  :   Marker(){
    this->SolutionPath = __strcpy(sSolution);
}

Marker::Marker(const char *sSolution,  time_t LimitedTime)   :   Marker(){
    this->SolutionPath = __strcpy(sSolution);
    this->LimitedTime = LimitedTime * 1000;
    this->addMode(MODE_TIMER);
}

Marker::Marker(const char *sSolution, const char *sInpSample, const char *sOutSample) : Marker(sSolution){
    this->InpSample = __strcpy(sInpSample);
    this->OutSample = __strcpy(sOutSample);
    this->addMode(MODE_TEST);
}

Marker::Marker(const char *sSolution, const char *sInpSample, const char *sOutSample, time_t LimitedTime)
    :Marker(sSolution, LimitedTime){
    this->InpSample = __strcpy(sInpSample);
    this->OutSample = __strcpy(sOutSample);
    this->addMode(MODE_TEST);
}

Marker::~Marker(){
    delete[] SolutionPath;
    delete[] Arguments;
    delete[] InpSample;
    delete[] OutSample;
    delete[] OutPath;
}

void Marker::resetMode(){
    this->mode = 0;
}


int Marker::addMode(int mode){
    if (!this->isMode(mode))
        return 1;   // mode is not in DEFINED mode
    this->mode |= mode;
    return 0;
}

int Marker::removeMode(int mode){
     if (!this->isMode(mode))
        return 1;   // mode is not in DEFINED mode
    
    this->mode &= ~mode;
    return 0;
}

void Marker::setSolution(const char *sSolution){
    delete[] this->SolutionPath;
    this->SolutionPath = __strcpy(sSolution);
}

void Marker::setArgument(const char *sArguments){
    delete[] Arguments;
    this->Arguments = __strcat(" ", sArguments);
}

void Marker::removeArgument(){
    this->Arguments = NULL;
}

void Marker::setTestSample(const char *sTestInp, const char *sTestOut){
    delete[] InpSample;
    delete[] OutSample;
    this->InpSample = __strcpy(sTestInp);
    this->OutSample = __strcpy(sTestOut);
}

void Marker::setOutPath(const char *sOutPath){
    delete[] OutPath;
    this->OutPath = __strcpy(sOutPath);
}

int Marker::setIOFormat(int inputFormat, int outputFormat){
    if ((inputFormat == 1 || inputFormat == 2) && (outputFormat == 1 || outputFormat == 2)){
        this->inpFormat = inputFormat;
        this->outFormat = outputFormat;
        return 0;
    }
    return 1;
}

int Marker::setTimer(time_t LimitedTime){
    if (LimitedTime <= 0)
        return 1;
    this->LimitedTime = LimitedTime * 1000;
}

__MARKER_PROCESS Marker::initializeMarkerProcess(){
    __MARKER_PROCESS mpMarkerProcess;
    
    // Set security attribute of pipes
    SECURITY_ATTRIBUTES saSecAtt;
    saSecAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
    saSecAtt.bInheritHandle = TRUE;
    saSecAtt.lpSecurityDescriptor = NULL;


    // Create pipe ================================
    if (!CreatePipe(&mpMarkerProcess.hInputRead, &mpMarkerProcess.hInputWrite, &saSecAtt, 0))
        mpMarkerProcess.exitCode = __MARKER_PROCESS::BAD_PIPE;
    
    if (!CreatePipe(&mpMarkerProcess.hOutputRead, &mpMarkerProcess.hOutputWrite, &saSecAtt, 0))
        mpMarkerProcess.exitCode = __MARKER_PROCESS::BAD_PIPE;

    // End Create pipe =============================

    // Setting up process ==============================
    // Solution Process
    STARTUPINFOA siStartupInfo_Solution;
    ZeroMemory(&siStartupInfo_Solution, sizeof(STARTUPINFOA));
    siStartupInfo_Solution.cb = sizeof(STARTUPINFO);
    
    siStartupInfo_Solution.hStdInput  = mpMarkerProcess.hInputRead;               // read data from input
    siStartupInfo_Solution.hStdOutput = mpMarkerProcess.hOutputWrite;             // write result to output
    siStartupInfo_Solution.hStdError  = mpMarkerProcess.hOutputWrite;             // 
    siStartupInfo_Solution.dwFlags |= STARTF_USESTDHANDLES;

    //End Setting up process=================================

    mpMarkerProcess.siStartupInfo_Solution = siStartupInfo_Solution;
   
    return mpMarkerProcess;
}

PROCESS_INFORMATION Marker::launchProcess(LPSTR commandLine,STARTUPINFOA &siStartupInfo){
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    bool bSuccess = CreateProcessA(NULL,
        commandLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartupInfo,
        &pi);
    if (!bSuccess){
        pi.hProcess = NULL;   
    }
    return pi;
}

DWORD WINAPI ReadOutput(LPVOID param){
    printf("Read Output thread Working....\n");
    __READ_OUTPUT_DATA *data = (P__READ_OUTPUT_DATA) param;

    bool b;
    do{
        char buffer[bufferSize];
        DWORD dwRead;
        b = ReadFile(data->pipe, buffer, sizeof(buffer) - 1, &dwRead, 0);
        
        if (dwRead > 0){
            buffer[dwRead]= '\0';
            data->data.add(buffer);
        }
    }while (b);

    printf("Read output from solution successfully...\n");
}

void Marker::setCompareFunc(int (*compare)(char [][255], int, char[][255], int)){
    this->compare = compare;
}

int stdCompare(char sample[][255], int nSample, char out[][255], int nOut){
    printf("Comparing......\n");
    
    int *ED = new int [nOut + 1];

    for (int i = 0; i<=nOut; i++)
        ED[i] = i;
    
    for (int i = 1; i<=nSample; i++){
        int left = i;
        int dag = i - 1;
        char *s = sample[i - 1];
        for (int j = 1; j <= nOut; j++){
            char *o = out[j - 1];
            
            int diff = strcmp(s, o) == 0 ? 0 : 1;

            int min = 0;
            min = (ED[j] <= left ? ED[j] : left) + 1;
            min = min <= dag + diff? min : dag + diff;

            int cur = min + diff;
            dag = ED[j];
            left = cur;
            ED[j] = cur;
        }
    }
    
    int res = ED[nOut];
    printf("Comparing complete\n");
    delete[] ED;
    return res;
}

__MARKER_RESULT Marker::mark(){
    // initilize Result and set Default Value
    __MARKER_RESULT res;
    ZeroMemory(&res, sizeof(__MARKER_RESULT));

    // result is unknown because solution hasn't runned yet
    //                          or it may be that Test Mode is off --> No-checking
    res.result = RES_UNKNOWN;
    res.mode = this->mode;

      

    // initialize a MARKER PROCESS 
    // by starting PIPEs and setting up process_info of Solution Process
    __MARKER_PROCESS MarkerProcess = initializeMarkerProcess();
    
    if (MarkerProcess.exitCode == __MARKER_PROCESS::BAD_PIPE){
        // If initialization fails, it means that neccessary PIPEs can be created
        // Just exit with a error
        res.exitCode = ERR_BAD_PIPE;
        return res;
    }

    // ===================   START MARKING =========================

    // The order of implementing marking:
    //========================================================================
    // 1. Start Solution-Process
    //                        
    //   ---------         start        -----------
    //   \ Marker \-------------------> \ Solution \
    //    ----------                     -----------
    //
    //========================================================================
    //
    // 2. If Test Mode is on  and outFormat is std   
    //      --> run thread of ReadOutput() parallelly with Solution-Process
    //          ..to read ouput which solution write via OUTPUT PIPE
    //          The thread will be close when it read all data from solution process
    //
    //
    //                                                       return result for step 5
    //                                         pipe                ^
    //                                      |--------|             |                   
    //   read    -----------    write       |        |      ReadOutput thread       ---------               
    // ----------\ Solution \-------------->| Output |----------------------------->\ Marker \   
    //   |         -----------         |    |        |                                ---------
    //   |                             |    |--------|                
    //   |                             |
    //   |                             | 
    //   |---------main thread---------|
    //
    //=========================================================================
    //
    // 3. inpFormat                   --> Transmiss data to solution process 
    //
    //      a. File --> ignore      (Solution Process will read data itself via file)
    //                                  
    //                                     some files                                   
    //                                         |
    //                                         |
    //      ---------                      -----------                           
    //      \ Marker \-------------------->\ Solution \----------->
    //        ---------                      -----------               
    //
    //
    //      b. std  --> Marker read data from file InpSample and transmiss it to INPUT PIPE,
    //                  then Solution Process will read data from INPUT PIPE
    //
    //   InpSample                          
    //       |                pipe                  
    //       |             |-------|           
    // ----------   write  |       |  read         -----------                 
    //  \ Marker \-------->| Input |-------------->\ Solution \----------->
    //   ----------        |       |                 -----------               
    //                     |-------|                                            
    //=========================================================================================
    //
    //
    // 4. Timer Mode
    //      a. On   --> start Timer to set up timeout for Solution Process
    //                  if Solution Process run so long, terminate it
    //                  and exit with a error
    //
    //  
    //                                      |------ check timeout--------| 
    //                                      |                            |        
    //                                      |                            |      
    //  ---------                      -----------                       |      
    //  \ Marker \-------------------->\ Solution \-----------END SOLUTION
    //    ---------                      -----------
    //
    //
    //      b. Off  --> ignore
    //
    //=======================================================================================
    //
    // 5. test mode is on         
    //              and outFormat is
    //      a. file -->  Read data from file 
    //      b. std  -->  Read data from ReadOutput thread
    //
    //  --then compare result from OutPath with OutSample 
    //
    //
    //
    
    

    // 1. START SOLUTION PROCESS
    if (this->SolutionPath == NULL){
        // if Solution Path has not been provided, so it can start Solution Process
        // Just exit with ERR_NO_SOLUTION
        res.exitCode = ERR_NO_SOLUTION;
        return res;
    }

    // Combine Solution Name and Arguments of it, then storing it in CommandLineSolution
    // Example:
    //       Solution name: "example.exe"
    //       Arguments    : "arg1 arg2"
    //---> CommandLineSolution : "example arg1 arg2"
    // See "Pass arguments to c program" for further detail....
    // Default argument is null
    LPSTR CommandLineSolution = __strcat(this->SolutionPath, this->Arguments);
    
    // Start Solution Process with a setting (siStartupInfo_Solution) returned by initializeMarkerProcess
    // and assign process_infomation to piSolution
    PROCESS_INFORMATION piSolution = 
        this->launchProcess(CommandLineSolution, MarkerProcess.siStartupInfo_Solution);
    
    if (piSolution.hProcess == NULL){
        // If failure in launch process,
        //      it can be Solution Path is wrong
        //      or Solution can't run as executable file
        //      See "CreateProcess C++" for further detail
        //  
        // Just exit with a error
        res.exitCode = ERR_BAD_SOLUTION;
        return res;
    }

    // Timer should be started when solution have just begun
    // This start_process_time will be used for caculating run time of Solution Process
    time_t start_process_time = clock();

    printf("Launching Solution Completely...\n");

    // 2. START READ OUTPUT THREAD IF STD OUT FORMAT AND TEST MODE IS ON
    // Creating global variables which receive data returned by ReadOutput() thread
    HANDLE hThread;                        // monitor Read Output thread
    P__READ_OUTPUT_DATA pdata = NULL;      // monitor data memory of Read Output thread

    if (this->mode & MODE_TEST){
        if (this->OutSample == NULL){
            // If OutputSample is NULL, it is that user don't provide OutSample Path
            // This results to be impossible to compare output
            // Just exit with a error
            res.exitCode = ERR_NO_SAMPLE;
            return res;
        }

        //2a. Output Format is file --> check OutPath and ignore
        if (this->outFormat == FORMAT_FILE){
            if (this->OutPath == NULL){
                // If OutputSample is NULL, it is that user don't provide Out Path
                // This results to be impossible to read output
                // Just exit with a error
                res.exitCode = ERR_NO_OUTPUT;
                return res;
            }
            // do nothing
        }
        else{
            //2b. Output Format is std  --> run thread of ReadOutput  
            if (MarkerProcess.hOutputRead  == NULL ||
                MarkerProcess.hOutputWrite == NULL)
            {
                // If neccessary PIPEs was not created
                // It is impossible to thread read data from OUTPUT PIPE
                // Just exit with a error
                res.exitCode = ERR_BAD_PIPE;
                return res;
            }


            // Allocate memory for thread
            pdata = (P__READ_OUTPUT_DATA)HeapAlloc(
                GetProcessHeap(),                   //  HANDLE OF PROCESS
                HEAP_ZERO_MEMORY,                   //  HEAD MODE
                sizeof(__READ_OUTPUT_DATA));        //  SIZE OF HEAP

            
            // Pass handle of Output Read into thread 
            pdata->pipe = MarkerProcess.hOutputRead;
            
            // indentifier of Thread
            DWORD Thread_Info;

            // CREATE READ OUTPUT THREAD
            hThread = CreateThread(
                NULL,                   // default SECURITY_ATTRIBUTE 
                0,                      // defaut heap allocation 
                ReadOutput,             // thread function name
                pdata,                  // thread function parameters
                0,                      // creation flags
                &Thread_Info);          // identifier of Thread
            
            if (hThread == NULL){
                // if thread creation fails
                // clean up heap memory 
                // exit with ERR_BAD_READ_OUTPUT
                HeapFree(GetProcessHeap(), 0, pdata);
                res.exitCode = ERR_BAD_READ_OUTPUT;
                return res;
            }
        printf("Launching Read Output Thread Completely...\n");
        } 
        // END CREATE THREAD
    }// END STEP 2

    // 3. Pass data to solution process
    // a. if inputFormat is File --> solution get data itself
    if (this->inpFormat == FORMAT_FILE){
        // do nothing
        CloseHandle(MarkerProcess.hInputWrite);
        printf("Solution Is Getting Data From File\n");
    }
    else if (this->inpFormat == FORMAT_STD){
         // b. if inputFormat is std 
        //      --> read data from InpSample
        //      --> transmiss it to INPUT PIPE
        //
        //   InpSample
        //      |                  pipe
        //      |               |-------|
        //  ---------           |       |           ----------- 
        //  \ Marker \ -------->| Input |---------->\ Solution \
        //   ---------          |       |            ------------
        //                      |-------|
        //
        //
        printf("Transmissing data begin....\n");
        if (MarkerProcess.hInputRead == NULL || MarkerProcess.hInputWrite == NULL){
            // If PIPE is error, it is impossible to write and read data from INPUT PIPE
            // Just exit with a error
            res.exitCode = ERR_BAD_PIPE;
            return res;
        }

        if (this->InpSample == NULL){
            // If InpSample path has not provided, it can be read data from InpSample
            // Exit with a error
            res.exitCode = ERR_NO_SAMPLE;
            return res;
        }
        
        //Open InpSample File
        FILE * inpFile = fopen(this->InpSample, "r");
        if (inpFile == NULL){
            // If InpSample do not exist
            // Exit with a error
            res.exitCode = ERR_NO_SAMPLE;
            return res;
        }

        // Repeate until read all data of file
        char buffer[bufferSize];
        while (!feof(inpFile)){
            // Read data from InpSample
            fscanf(inpFile, "%s", buffer);
            buffer[strlen(buffer)] = '\n';
            
            // Write this data to Input Pipe
            DWORD dwInputWrite; 
            WriteFile(MarkerProcess.hInputWrite, buffer, strlen(buffer), &dwInputWrite, 0);
        }
        // Close InpSample when Transmissing is complete
        fclose(inpFile);
        
        // Close INPUT PIPE if writing done
        CloseHandle(MarkerProcess.hInputWrite);
        printf("Transmissing Data Successfully....\n");
    }
    // END Step 3 : Transmissing data
    else{
        // if format do not belongs to any formats code(FORMAT_FILE or FORMAT_STD)
        //  --> exit with ERR_BADFORMAT
        res.exitCode = ERR_BAD_FORMAT;
        return res;
    }
    // 4. start timer to set timeout for Solution Process
    //
    //      |------ check timeout--------| 
    //      |                            |      
    //      |                            |      
    //      |              -----------   |      
    // ------------------->\ Solution \----------->
    //                      -----------                                                
    //
    //  OR stop program to wait until Solution complete if Timer Mode is off
    

    DWORD STATUS = WaitForSingleObject(piSolution.hProcess, this->LimitedTime);
    CloseHandle(MarkerProcess.hInputRead);
    CloseHandle(MarkerProcess.hOutputWrite);

    // END TIMER WHEN SOLUTION STOP
    time_t end_process_time = clock();
    res.runningTime = end_process_time - start_process_time;
  
    if (STATUS == WAIT_TIMEOUT){
        // If Solution Process is non-signaled after time interval
        // Terminate it and exit with a non-correct result
        TerminateProcess(piSolution.hProcess, 0);
        CloseHandle(piSolution.hProcess);
        CloseHandle(hThread);
        res.result = RES_TIME_LIMIT_EXCEED;
        return res;
    }
    CloseHandle(piSolution.hProcess);
    
    printf("Solution Finished.....\n");

    // 5. TEST MODE IS ON?
    //      --> Read data from Out or ReadOutput Thread
    if (this->mode & MODE_TEST){
        // Read OutSample to get result sample
        // store it into "sample"
        
        // Open file
        FILE *outFile = fopen(this->OutSample, "r");
        if (outFile == NULL){
            // Opening file is fails
            // Exit with a error
            res.exitCode = ERR_NO_SAMPLE;
            return res;
        }

        // Starting Read OutSample
        char sample[bufferSize][255];
        int nSample = 0;
        while (!feof(outFile)){
            fscanf(outFile, "%s", sample[nSample]);
            nSample++;
        }
        fclose(outFile);

        // Read data which solution generate and store it into outSol
        char outSol[bufferSize][255];
        int nSol = 0;

       
        // a. if outformat is file  --> read data from OutPath
        if (this->outFormat == FORMAT_FILE){
            
            // Open Out from OutPath
            outFile = fopen(this->OutPath, "r");
            if (outFile == NULL){
                // If Opening fails
                // Exit with a error
                res.exitCode = ERR_NO_OUTPUT;
                return res;
            }

            // Starting Read Out
            while (!feof(outFile)){
                fscanf(outFile, "%s", outSol[nSol]);
                nSol++;
            }
            fclose(outFile);
        }

        // b. if outFormat is std --> read data from ReadOutput
        else if (this->outFormat == FORMAT_STD){
            // Wait until ReadOutput Complete
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            // Data which ReadOut read from OUTPUT PIPE is store in __DATA::data
            // data is a struct defined in utilities.h
            

            // Starting reading
            pdata->data.open();  
            while(1){
                pdata->data.scanf(outSol[nSol]);
                if (outSol[nSol][0] == '\0')
                    break;   
                nSol++;    
            }
            //free memory which is allocated for thread
            HeapFree(GetProcessHeap(), 0, pdata);
            pdata = NULL;
        }   
        else{
            // If no-format is matched
            // Exit with a error
            res.exitCode = ERR_BAD_FORMAT;
            return res;
        }// END READING OUT
       
        int diff = (*this->compare)(sample, nSample, outSol, nSol);
        if (diff == 0)
            res.result = RES_CORRECT;
        
        else
            res.result = RES_INCORRECT;
    }

    return res;
}

bool Marker::isMode(int mode){
    if (MODE_ALL & mode != 0)
        return true;
    return false;
}