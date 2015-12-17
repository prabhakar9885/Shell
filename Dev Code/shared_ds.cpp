
#ifndef SHARED_DS
#define SHARED_DS 1
#endif

typedef struct {
    char* cmds[1024];
    int cmdCount;
    int exitStatus;
}pipedCommand;


typedef struct {
    char** tokens;
    int tokenCount;
    int exitStatus;
}singleCommand;
