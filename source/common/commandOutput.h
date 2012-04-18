#ifndef __COMMAND_OUTPUT_H__
#define __COMMAND_OUTPUT_H__

#include <stdio.h>

#define START_COMMAND_OUTPUT_LOOP(command, buffer, size)         \
{                                                                \
   FILE *commandOutputLoopPipe = popen(command, "r");            \
   if (!commandOutputLoopPipe) {                                 \
      exit(1);                                                   \
   }                                                             \
                                                                 \
   while (!feof(commandOutputLoopPipe)) {                        \
      if (fgets(buffer, size, commandOutputLoopPipe) == NULL) {  \
         break;                                                  \
      }

#define END_COMMAND_OUTPUT_LOOP                                  \
   }                                                             \
   pclose(commandOutputLoopPipe);                                \
}

#endif // __COMMAND_OUTPUT_H__
