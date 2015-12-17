#ifndef PIPEDCOMMAND_H
#define PIPEDCOMMAND_H

typedef struct {
    char* cmds[1024];
    int cmdCount;
    int exitStatus;
} pipedCommand;