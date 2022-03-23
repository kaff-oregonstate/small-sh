# small-sh
### CS 344: Operating Systems I
### Instructor Lewis
### David Kaff
### 10 February 2021
A small shell written in C-lang. Utilizes Unix process API, has ability to run commands in foreground or background, control and check status of child processes, and use custom SIGTSTP keyboard shortcut.

## Compiling
To compile and test the program, run the following two lines of code in a directory containing `main.c`:
```
gcc --std=gnu99 -o smallsh main.c -lm
./smallsh
```
