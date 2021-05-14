# Smallsh

a bash-like shell application written in C.  
implements custom exit, status, and cd commands.  
other commands handled by forking and using exec functions.  
supports background processes, PID expansion, and input/output redirection.  
implements custom signal handlers.  

### COMPILE:
gcc -std=c99 -g3 main.c -o smallsh

### EXECUTE:
./smallsh

### Command Format:
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
