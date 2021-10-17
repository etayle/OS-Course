#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;


void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    if(smash.getIsAson()){
        ctrlZforkSonHandler(sig_num);
        return;
    }
    if(smash.getIsAfather()){
        ctrlZforkFatherHandler(sig_num);
        return;
    }
    pid_t pid = smash.getFgPid();
    cout << "smash: got ctrl-Z" << endl;
    // checks if there is something running in the fg
    if(pid>0){
        JobsList* jobs = smash.getJobList();
        Command* cmd = smash.getFgCommand();
        JobsList::JobEntry* job = jobs->getJobByPid(pid);
        if(!job){
            jobs->addJob(cmd, pid, true);
        }
        else{
            job->status = STOP;
            job->startTime = time(nullptr);
        }
        smash.getFgCommand()->to_delete_imm = false;
        if(smash.getFgCommand()->get_cmd_name() == "pipe"){
            if(kill(pid, SIGTSTP)==-1){
                perror("smash error: kill failed");
            }
            //the father stop himself
            cout << "smash: process " << pid<< " was stopped" << endl;
            return;
        }
        if (kill(pid, SIGSTOP)==-1){
            perror("smash error: kill failed");
        }
        cout << "smash: process " << pid << " was stopped" << endl;
    }
}

void ctrlZforkFatherHandler(int sig_num){
    int status;
    pid_t son1;
    pid_t son2;
    SmallShell& smash = SmallShell::getInstance();
    smash.getSons(&son1,&son2);
    if(waitpid(son1,&status,WNOHANG)==0){
        if(kill(son1,SIGTSTP)==-1) {
            perror("smash error: kill failed");
        }
    }
    if(waitpid(son2,&status,WNOHANG)==0) {
        if (kill(son2, SIGTSTP) == -1) {
            perror("smash error: kill failed");
        };
    }
    kill(getpid(), SIGSTOP);
}

void ctrlZforkSonHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.getFgCommand()->isExternalCommand()){
        if (kill(smash.getSpecialProcess(), SIGSTOP) == -1) {
            perror("smash error: kill failed");
        }
    }
    if(kill(getpid(),SIGSTOP)==-1){
        perror("smash error: kill failed");
    }
}
void ctrlCforkFatherHandler(int sig_num){
    int status;
    pid_t son1;
    pid_t son2;
    SmallShell& smash = SmallShell::getInstance();
    smash.getSons(&son1,&son2);
    if(waitpid(son1,&status,WNOHANG)==0){
        if(kill(son1,SIGINT)==-1) {
            perror("smash error: kill failed");
        }
    }
    if(waitpid(son2,&status,WNOHANG)==0) {
        if (kill(son2, SIGINT) == -1) {
            perror("smash error: kill failed");
        };
    }
    kill(getpid(), SIGKILL);
}

void ctrlCforkSonHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.getFgCommand()->isExternalCommand()){
        if (kill(smash.getSpecialProcess(), SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    }
    if(kill(getpid(),SIGKILL)==-1){
        perror("smash error: kill failed");
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    if(smash.getIsAson()){
        ctrlCforkSonHandler(sig_num);
         return;
    }
    if(smash.getIsAfather()){
        ctrlCforkFatherHandler(sig_num);
        return;
    }
    cout << "smash: got ctrl-C" << endl;
    pid_t pid = smash.getFgPid();
    if (0 < pid){
        if(smash.getFgCommand()->get_cmd_name() == "pipe") {
            if (kill(pid, SIGINT) == -1) {
                perror("smash error: kill failed");
                return;
            }
        }
        else{
            if(kill(pid, SIGKILL) == -1){
                perror("smash error: kill failed");
                return;
            }
        }
        JobsList* jobs = smash.getJobList();
        Command* cmd = smash.getFgCommand();
        JobsList::JobEntry* job = jobs->getJobByPid(pid);
        if(job){
            jobs->removeJobById(job->jobId);
            delete cmd;
        }
        cout << "smash: process " << pid << " was killed" << endl;
        smash.setFgPid( -1);
    }
}

void alarmHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    smash.getJobList()->removeFinishedJobs();
    cout << "smash: got an alarm" << endl;
    Timeout::TimeoutEntry timeout_obj = smash.timeout->time_list->front();
    if(timeout_obj.pid>=0 && kill(timeout_obj.pid, SIGKILL)>=0){
        cout << "smash: " << timeout_obj.cmd->get_cmd_string() << " timed out!" << endl;
    }

    smash.timeout->time_list->pop_front();
    if(!smash.timeout->time_list->empty()){
        alarm(smash.timeout->time_list->front().getRemainingTime());
    }
}

void sigContHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    int status;
    if (smash.getIsAson()) {
        if (smash.getFgCommand()->isExternalCommand()) {
            if (kill(smash.getSpecialProcess(), SIGCONT) == -1) {
                perror("smash error: kill failed");
            }
        }
    }
    if (smash.getIsAfather()) {
        pid_t son1;
        pid_t son2;
        smash.getSons(&son1, &son2);
        if (waitpid(son1, &status, WNOHANG)==0) {
            if (kill(son1, SIGCONT) == -1) {
                perror("smash error: kill failed");
            }
        }
        if (waitpid(son2, &status, WNOHANG)==0) {
                if (kill(son2, SIGCONT) == -1) {
                    perror("smash error: kill failed");
                }
            }
        }
    }


