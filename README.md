# smallsh

- a Unix shell application written in C
- implements custom exit, status, and cd commands
- all other commands are handled by forking a new process and using exec functions
- supports foreground and background processes, PID expansion, and input/output redirection
- implements custom signal handlers

### COMPILE:

```
gcc -std=c99 -Wall -g3 main.c -o smallsh
```

### EXECUTE:

```
./smallsh
```

#### Command Format:

```
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
```

& indicates the process should be executed in the background  
< or > followed by a filename indicates standard input or ouput should be redirected  
Lines beginning with # are treated as comments and are ignored  
The variable $$ is expanded to the process ID of smallsh

#### Set Test Script Permissions:

```
chmod +x ./p3testscript
```

#### Execute Test Script:

```
./p3testscript > mytestresults 2>&1
```
