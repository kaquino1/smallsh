# smallsh

- a bash-like shell application written in C
- implements custom exit, status, and cd commands
- all other commands are handled by forking a new process and using exec functions
- supports background processes, PID expansion, and input/output redirection
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

#### Test Script Permissions:
```
chmod +x ./p3testscript  
```

#### Execute Test Script:
```
./p3testscript > mytestresults 2>&1  
```
