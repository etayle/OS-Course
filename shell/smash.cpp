#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
       perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(signal(SIGCONT , sigContHandler)==SIG_ERR) {
        perror("smash error: failed to set SIGCONT handler");
    }

    struct sigaction* act = (struct sigaction*)malloc(sizeof(*act));
    act->sa_handler = alarmHandler;
    act->sa_flags = SA_RESTART;
    sigemptyset(&act->sa_mask);

    if(sigaction(SIGALRM, act, nullptr)==-1){
        perror("smash error: failed to set SIGALRM handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    smash.act = act;

    while(true) {
        std::cout << smash.name();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}