#ifndef BUILTIN_H
#define BUILTIN_H

#include "parse.h"

int is_builtin(char *cmd);
void which(Task T);
void jobs();
void builtin_execute(Task T);
int builtin_which(Task T);

#endif
