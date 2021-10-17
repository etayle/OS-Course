#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>
#include <fstream>
#include <algorithm>
#include "Commands.h"
#include <signal.h>
#define FIRST true
#define LAST false
#define MAXPATH 4096
#include "signals.h"
using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const std::string DONT_OVERRIDE = ">>";
const std::string STDERROR_SIGN = "|&";


#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    if(*it == '-'){
        ++it;
    }
    if(it == s.end() ){
        return false;
    }
    while (it != s.end() && (std::isdigit(*it))) ++it;
    return !s.empty() && it == s.end();
}

int stringToInt(const string& a){
    stringstream job_string(a);
    int job_num;
    job_string >> job_num;
    return job_num;
}

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trimFirstWord(const std::string& s){
    size_t end = s.find_first_of(WHITESPACE);
    return (end == std::string::npos) ? s : s.substr(0, end);
}


string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

string _removeFirstWord(const std::string& s){
    size_t start = s.find_first_of(WHITESPACE);
    return (start == std::string::npos) ? s : _ltrim(s.substr(start));
}

bool _isOverride(const char* cmd_line){
    const string str(cmd_line);
    return str.find(DONT_OVERRIDE) == string::npos;
}
bool _isStdError(const char* cmd_line){
    const string str(cmd_line);
    return !(str.find(STDERROR_SIGN) == string::npos);
}


int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

string _returnPart(char* cmd_line, string seperate, bool first){
    string add_in_the_end;
    if(_isBackgroundCommand(cmd_line))
        add_in_the_end = "&";
    string part;
    if (first) {
        part = string(cmd_line).substr(0, string(cmd_line).find(seperate));
        part += add_in_the_end;
    }
    else{
        part = string(cmd_line).substr(string(cmd_line).find(seperate)+seperate.length());
    }

    return _trim(part);
}

void _removeBackgroundSign(char* cmd_line) {
  // find last character other than spaces
  string str(cmd_line);
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (cmd_line[idx] == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
string _removeBackgroundSign(string s){
    string a;
    for(unsigned int i =0;i<s.length();i++){
        if(s[i]!= '&'){
            a += s[i];
        }
    }
    return a;
}

SmallShell::SmallShell() :name_shell("smash> "),shell_joblist(new JobsList()),
                        fg_pid(-1), timeout(new Timeout()){
    mypid = getpid();
    is_pipe_father_process = false;
    is_son = false;
    special_process =-1;
    need_wait_to_pipe = false;
}
SmallShell::~SmallShell() {
    delete shell_joblist;
    delete timeout;
    free(act);
    free(last_directory);
}

pid_t SmallShell::getmypid() {
    return mypid;
}
string SmallShell::name() {
    return this->name_shell;
}
void SmallShell::changeName(string name) {
    this->name_shell = name ;

}
JobsList* SmallShell::getJobList(){
    shell_joblist->removeFinishedJobs();
    return this->shell_joblist;
}


void SmallShell::setFgPid(pid_t pid) {
    fg_pid = pid;
}

pid_t SmallShell::getFgPid() {
    return fg_pid;
}

void SmallShell::setFgCommand(Command *cmd) {
    fg_cmd = cmd;
}

Command* SmallShell::getFgCommand() {
    return fg_cmd;
}
void SmallShell::setIsApipeFather(bool bolean){
    is_pipe_father_process = bolean;
};
void  SmallShell::setIsAson(bool bolean) {
    is_son = bolean;
}
void SmallShell::setSpecialProcess(pid_t pid) {
    special_process = pid;
}
bool SmallShell::getIsAfather() {
    return is_pipe_father_process;
}
bool SmallShell::getIsAson() {
    return is_son;
}
pid_t SmallShell::getSpecialProcess() {
    return special_process;
}
void SmallShell::setSons(pid_t son1, pid_t son2) {
    this->son1 = son1;
    this->son2 = son2;
}
void SmallShell::getSons(pid_t * son1,pid_t * son2){
    *(son1) = this->son1;
    *(son2) = this->son2;
}
void SmallShell::setNeedWait(bool bolean){
    this->need_wait_to_pipe = bolean;
}
bool SmallShell::needWaitPipe() {
    return need_wait_to_pipe;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = string(cmd_line);
  // ignoring any spaces in the beginning of the cmd_line
  cmd_s = _ltrim(cmd_s);
  if (cmd_s.find(">") != string::npos){
      return new RedirectionCommand(cmd_line);
  }
  else if (cmd_s.find("|") != string::npos){
      return new PipeCommand(cmd_line);
  }
  else if (cmd_s.find("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (cmd_s.find("chprompt") == 0) {
      return new  Chprompt(cmd_line);
  }
  else if (cmd_s.find("pwd") == 0) {
      return new  GetCurrDirCommand(cmd_line);
  }
  else if (cmd_s.find("jobs") == 0) {
      return new  JobsCommand(cmd_line,this->getJobList());
  }
  else if (cmd_s.find("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &last_directory);
  }
  else if (cmd_s.find("kill") == 0) {
      return new KillCommand(cmd_line, getJobList());
  }
  else if (cmd_s.find("quit") == 0) {
      return new QuitCommand(cmd_line, this->getJobList());
  }
  else if (cmd_s.find("bg") == 0) {
      return new BackgroundCommand(cmd_line, getJobList());
  }
  else if (cmd_s.find("fg") == 0) {
      return new ForegroundCommand(cmd_line, getJobList());
  }
  else if(cmd_s.find("cp") == 0){
      return new CopyCommand(cmd_line, getJobList());
  }
  else if(cmd_s.find("timeout") == 0){
      return new TimeoutCommand(cmd_line, getJobList());
  }
  // external command
  else{
      return new ExternalCommand(cmd_line, getJobList());
  }
}

void SmallShell::executeCommand(const char *cmd_line) {

  Command* cmd = CreateCommand(cmd_line);
  if (cmd == nullptr){
      return;
  }
  cmd->execute();
  if(cmd->to_delete_imm){
      delete cmd;
  }
}

void SmallShell::executeCommand(const char *cmd_line, const char *string_cmd) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd == nullptr){
        return;
    }

    cmd->set_cmd(string_cmd);
    cmd->execute();
    if(cmd->to_delete_imm){
        delete cmd;
    }

}

//Command

// Parses the command and saves it's arguments
Command::Command(const char *cmd_line):args(new char*[20]) {
    external_command = false;
    char* new_cmd = new char[string(cmd_line).length()+1];
    strcpy(new_cmd, cmd_line);
    char* new_cmd2 = new char[string(cmd_line).length()+1];
    strcpy(new_cmd2, cmd_line);
    string_cmd = new_cmd;
    cmd = new_cmd2;
    // in the arguments it deletes the &
    char* arg_cmd = new char[string(cmd_line).length()+1];
    strcpy(arg_cmd, new_cmd);
    if(_isBackgroundCommand(new_cmd)){
        _removeBackgroundSign(arg_cmd);
    }
    size_of_args = _parseCommandLine(arg_cmd, args);
    delete[] arg_cmd;
}

Command::~Command(){
    for(int i=0;i<size_of_args;i++){
        free(args[i]);
    }
    delete[] args;
    delete[] string_cmd;
    delete[] cmd;
}

void Command::set_error(const string error) {
    cerr << get_error_begin() + ": " + error << endl;
}

string Command::get_error_begin() {
    return "smash error: " + name;
}

//Gets the string of the whole command
string Command::get_cmd_string() {
    return string(string_cmd);
}

string Command::get_cmd_name() {
    return name;
}
void Command::setIsExternalCommand(bool bolean) {
    external_command = bolean;
}
bool Command::isExternalCommand() {
    return external_command;
}

void Command::set_cmd(const char* new_cmd) {
    char* new_cmd_string = new char[string(new_cmd).length()+1];
    strcpy(new_cmd_string, new_cmd);
    delete[] string_cmd;
    string_cmd = new_cmd_string;
}

//BuiltInCommand
BuiltInCommand::BuiltInCommand(const char *cmd_line):Command(cmd_line){}

//ShowPid
ShowPidCommand::ShowPidCommand(const char* cmd_line):BuiltInCommand(cmd_line){
    name = "showpid";
}

// prints the pid of the smash
void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getmypid() << endl;
}
//Chprompt Function::
Chprompt::Chprompt(const char* cmd_line):BuiltInCommand(cmd_line){
    this->name = "chprompt";
}
string Chprompt::nameToReturn(){
    if(this->size_of_args<=1){
        return "smash> ";
    }
    string s;
    s= this->args[1];
    s.append("> ");
    return s;
}
void Chprompt::execute(){
    SmallShell& smash = SmallShell::getInstance();
    smash.changeName(this->nameToReturn());
}

//GetCurrDirCommand

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void GetCurrDirCommand::execute() {
    char* path = getcwd(nullptr,0);
    cout << path << endl;
    free(path);
}

//JobsList
JobsList::JobsList():jobsList(vector<JobsList::JobEntry>()){}

void JobsList::printJobsList(printFunction func){
  for(unsigned int i =0;i<this->jobsList.size();i++){
      jobsList[i].printJob(func);
  }
}
void JobsList::removeFinishedJobs() {
    int status = 10;
    auto it = jobsList.begin();
    while(it!=jobsList.end()){
        pid_t result = waitpid(it->processId,&status,WNOHANG);
        if(result>0){
            delete it->cmd;
            it = jobsList.erase(it);
        }else{
            it++;
        }
    }
}

//JobsEntry command
void JobsList::JobEntry::printJob(printFunction func) {
    if(func == JOBS){
    time_t currentTime = time(nullptr);
    double  seconds_elapsed = difftime(currentTime, this->startTime);
    cout << "[" << this->jobId << "]" << " " << this->cmd->get_cmd_string() << " : " << this->processId << " "
         << seconds_elapsed << " secs";
    if(this->status == STOP){
        cout<< " (stopped)";
    }
    cout<<endl;
    return;
    }
    if(func == QUIT){
        cout<< processId << ": " << this->cmd->get_cmd_string() <<endl;
    }
}

JobsList::JobEntry * JobsList::getJobById(int jobId) {
    for(auto it = jobsList.begin();it!=jobsList.end();it++){
        if(it->jobId == jobId){
            return &*it;
        }
    }
    return nullptr;
}

JobsList::JobEntry * JobsList::getJobByPid(pid_t pid) {
    for(auto it = jobsList.begin();it!=jobsList.end();it++){
        if(it->processId == pid){
            return &*it;
        }
    }
    return nullptr;
}

void JobsList::addJob(Command *cmd, pid_t processId, bool isStopped) {
    JobEntry job;
    job.processId = processId;
    job.cmd = cmd;
    job.commandName = cmd->get_cmd_name();
    job.startTime = time(nullptr);
    jobStatus status = BACKGROUND;
    if(isStopped)
        status = STOP;
    job.status = status;
    int jobId;
    if (jobsList.empty())
        jobId = 1;
    else
        jobId = jobsList.back().jobId + 1;
    job.jobId = jobId;
    jobsList.push_back(job);
}

void JobsList::removeJobById(int jobId) {
    for(auto it = jobsList.begin();it!=jobsList.end();it++){
        if(it->jobId == jobId){
            jobsList.erase(it);
            return;
        }
    }
}

JobsList::JobEntry * JobsList::getLastJob(int *lastJobId) {
    JobEntry* job;
    if(jobsList.empty())
        return nullptr;
    job = &jobsList.back();
    *lastJobId = job->jobId;
    return job;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId) {
    for(auto it=jobsList.rbegin();it!=jobsList.rend();it++){
        if(it->status == STOP){
            *jobId = it->jobId;
            return &*it;
        }
    }
    return nullptr;
}

//JobsCommand
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line),jobs(jobs){
    this->name = "jobs";
}

void JobsCommand::execute() {
    jobs->printJobsList(JOBS);
}

//ChangeDirCommand

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char** plastPwd):BuiltInCommand(cmd_line) {
    name = "cd";
    is_last_dir = false;
    last_dir = plastPwd;
    if(*plastPwd) {
        is_last_dir = true;
    }
}

void ChangeDirCommand::execute() {
    if (size_of_args >= 3){
        set_error("too many arguments");
        return;
    }
    if (size_of_args == 1){
        return;
    }
    string dir;
    char* new_last_dir = getcwd(nullptr, 0);
    if(string(args[1]) == "-"){
        if(!is_last_dir){
            set_error("OLDPWD not set");
            free(new_last_dir);
            return;
        }
        else{
            dir = *last_dir;
        }
    }
    else{
        dir = args[1];
    }
    if(chdir(dir.c_str()) == -1){
        perror("smash error: chdir failed");
        free(new_last_dir);
        return;
    }
    else{
        if(is_last_dir){
            free(*last_dir);
        }
        *last_dir = new_last_dir;
    }
}

//Quit
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line),jobs(jobs){
    this->name = "quit";
}

void QuitCommand::execute() {
    bool kill_exist = false;
    for(int i =0; i<size_of_args;i++){
        if(args[i]==string("kill")){
            kill_exist = true;
            break;
        }
    }
    if(kill_exist){
        for(unsigned int i=0;i<jobs->jobsList.size();i++){
            if(kill(jobs->jobsList[i].processId,SIGKILL)==-1){
                perror("smash error: kill failed");
                delete jobs->jobsList[i].cmd;
                jobs->removeJobById(jobs->jobsList[i].jobId);
            };
        }
        cout<<"smash: sending SIGKILL signal to " << jobs->jobsList.size() << " jobs:"<<endl;
        jobs->printJobsList(QUIT);
    }

    for(unsigned int i=0;i<jobs->jobsList.size();i++){
        delete jobs->jobsList[i].cmd;
    }

    delete this;
    exit(0);
}

// Kill Command

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),jobs(jobs) {
    name = "kill";
}

void KillCommand::execute() {
    if (size_of_args != 3){
        set_error("invalid arguments");
        return;
    }
    // checks if the first argument starts with -
    if(string(args[1]).find('-')!=0){
        set_error("invalid arguments");
        return;
    }
    if(!is_number(string(args[1]).substr(1))){
        set_error("invalid arguments");
        return;
    }
    int kill_num = stringToInt(string(args[1]).substr(1));
    if(!is_number(string(args[2]))){
        set_error("invalid arguments");
        return;
    }
    int job_num = stringToInt(string(args[2]));
    JobsList::JobEntry* job = jobs->getJobById(job_num);
    if(!job){
        string error = "job-id " + string(args[2]) + " does not exist";
        set_error(error);
        return;
    }
    if(killpg(getpgid(job->processId), kill_num)==-1){
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << kill_num << " was sent to pid " << job->processId << endl;
}

// Background Command

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),jobs(jobs) {
    name = "bg";
}

void BackgroundCommand::execute() {
    int jobId;
    if(size_of_args>2){
        set_error("invalid arguments");
        return;
    }
    // There is no specific job id
    JobsList::JobEntry* job;
    if(size_of_args==1){
        job = jobs->getLastStoppedJob(&jobId);
        if(!job){
            set_error("there is no stopped jobs to resume");
            return;
        }
    }
    else{
        if(!is_number(string(args[1]))){
            set_error("invalid arguments");
            return;
        }
        int job_num = stringToInt(string(args[1]));
        job = jobs->getJobById(job_num);
        if(!job){
            string error = "job-id " + string(args[1]) + " does not exist";
            set_error(error);
            return;
        }
    }
    if(job->status == BACKGROUND){
        string error = "job-id " + string(args[1]) + " is already running in the background";
        set_error(error);
        return;
    }
    if(kill(job->processId, SIGCONT)==-1){
        perror("smash error: kill failed");
        return;
    }
    job->status = BACKGROUND;
    cout << job->cmd->get_cmd_string() << " : " << job->processId << endl;
}

//Foreground Command

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),jobs(jobs) {
    name = "fg";
}

void ForegroundCommand::execute() {
    //check the valid of input
    if(size_of_args>2){
        set_error("invalid arguments");
        return;
    }
    int jobId;
    JobsList::JobEntry* job;
    if(size_of_args == 1){
        job = jobs->getLastJob(&jobId);
        if(!job) {
            set_error("jobs list is empty");
            return;
        }
    }
    //occur when size of args =2;
    else {
        if (!(is_number(string(args[1])))) {
            set_error("invalid arguments");
            return;
        }
        jobId = stringToInt(string(args[1]));
        job = jobs->getJobById(jobId);
        if (!job) {
            string error = "job-id " + string(args[1]) + " does not exist";
            set_error(error);
            return;
        }
    }
    int status = -1;
    string name_to_print = job->cmd->get_cmd_string();
    int pid_to_print = job->processId;
    int pid = job->processId;
    Command* to_delete_cmd = job->cmd;
    SmallShell::getInstance().setFgPid(pid);
    SmallShell::getInstance().setFgCommand(to_delete_cmd);
    if(kill(pid,SIGCONT)==-1){
        perror("smash error: kill failed");
    }
    cout<< name_to_print << " : "<< pid_to_print << endl;

    pid_t result = waitpid(pid,&status,WUNTRACED);
    if(result==-1){
        perror("smash error: waitpid failed");
        return;
    }
    if(!WIFSTOPPED(status)){
        SmallShell& smash = SmallShell::getInstance();
        JobsList* jobs = smash.getJobList();
        Command* cmd = smash.getFgCommand();
        JobsList::JobEntry* job = jobs->getJobByPid(pid);
        if(job){
            jobs->removeJobById(job->jobId);
            delete cmd;
        }
    }
    SmallShell::getInstance().setFgPid(-1);
}

// External Command
ExternalCommand::ExternalCommand(const char *cmd_line, JobsList* jobs):Command(cmd_line), jobs(jobs) {
    this->setIsExternalCommand(true);
    isBackground = false;
    to_delete_imm = true;
    if(_isBackgroundCommand(cmd_line)) {
        isBackground = true;
        to_delete_imm = false;
    }
}

void ExternalCommand::execute() {

    char* no_bg_sign = new char[string(cmd).length() + 1];
    strcpy(no_bg_sign, cmd);
    if(size_of_args > 0)
        name = args[0];
    if(isBackground)
        _removeBackgroundSign(no_bg_sign);
    string cmd_name = "/bin/bash";
    char* name_cmd = &cmd_name[0];
    string start = "-c";
    char* c = &start[0];
    char * exe_args[] = { name_cmd, c, no_bg_sign , nullptr};
    int pid = fork();
    if(pid==-1){
        perror("smash error: fork failed");
    }
    else if(pid==0){
        SmallShell& smash = SmallShell::getInstance();
        if(!(smash.getIsAson() || smash.getIsAfather())) {
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
            }
        }
        if(execv("/bin/bash", exe_args) == -1){
            perror("smash error: execv failed");
        }
        delete[] no_bg_sign;
    }
    else{
        if(timeout>=0){
            SmallShell& smash = SmallShell::getInstance();
            smash.timeout->addAlarm(pid, timeout, this);
        }
        if(isBackground){
            jobs->addJob(this, pid, false);
        }
        else{
            SmallShell& smash = SmallShell::getInstance();
            int status;
            smash.setFgPid(pid);
            smash.setSpecialProcess(pid); // add this to a pipe of son
            smash.setFgCommand(this);
            if(waitpid(pid, &status, WUNTRACED)==-1){
                perror("smash error: waitpid failed");
            }
            if(WIFSTOPPED(status)){
                to_delete_imm = false;
            }
            smash.setFgPid(-1);
        }
    }
    delete[] no_bg_sign;
}

// Redirection Command
RedirectionCommand::RedirectionCommand(const char *cmd_line):Command(cmd_line) {
    name = "redirect";
    char* no_redirect_sign = new char[string(cmd_line).length()+1];
    strcpy(no_redirect_sign, cmd_line);
    override = _isOverride(cmd_line);
    string seperate = ">>";
    if(override){
        seperate = ">";
    }
    first_part = _returnPart(no_redirect_sign, seperate, FIRST);
    _removeBackgroundSign(no_redirect_sign);
    last_part = _trimFirstWord(_returnPart(no_redirect_sign, seperate, LAST));
    delete[] no_redirect_sign;
}

void RedirectionCommand::execute() {
    int cout_place = dup(1);
    close(1);
    int fd;
    if(override){
        fd = open(last_part.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    }
    else{
        fd = open(last_part.c_str(), O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
    }
    if(fd == -1){
        dup(cout_place);
        close(cout_place);
        perror("smash error: open failed");
        return;
    }
    SmallShell::getInstance().executeCommand(first_part.c_str(), get_cmd_string().c_str());

    close(fd);
    dup(cout_place);
    close(cout_place);
}

PipeCommand::PipeCommand(const char *cmd_line) :Command(cmd_line){
    char* no_bg_sign = new char[string(cmd).length() + 1];
    strcpy(no_bg_sign, cmd);
    name = "pipe";
    to_delete_imm = true;
    need_to_wait = true;
    char* no_redirect_sign = new char[string(cmd_line).length()+1];
    strcpy(no_redirect_sign, cmd_line);
    std_error = _isStdError(cmd_line);
    string seperate = "|";
    if(std_error) {
        seperate = "|&";
    }
    first_part = _returnPart(no_redirect_sign, seperate, FIRST);
    last_part = _returnPart(no_redirect_sign, seperate, LAST);
    if (last_part.find("&") != string::npos){
        need_to_wait = false;
        to_delete_imm = false;
    }
    first_part = _removeBackgroundSign(first_part);
    last_part = _removeBackgroundSign(last_part);
    delete[] no_redirect_sign;
}


void PipeCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int status;
    pid_t pipe_father_process = fork();
    if(pipe_father_process ==-1 ){
        perror("smash error: fork failed");
    }

    // prevent to get a ctrlz/c signal
    //enter the -main- shell the pipe_job to joblist,if need to run at background;
    if((!need_to_wait)&&(pipe_father_process > 0)){
        smash.getJobList()->addJob(this, pipe_father_process, false);
    }
    else if (pipe_father_process > 0)  {
        //enter the jon to fg process
        smash.setFgPid(pipe_father_process);
        smash.setFgCommand(this);
    }
    // create the main pipe process
    if (pipe_father_process == 0) {
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
            }
        int my_grp = getpgrp();
        smash.setIsApipeFather(true);
        int my_pipe[2];
        int cin_place;
        int cout_place;
        int cout_or_cerr = 1;
        //backup std in and stdout/err and close the relevant fdt
        if (std_error) {
            cout_or_cerr = 2;
        }
        cin_place = dup(0);
        cout_place = dup(cout_or_cerr);
        close(0);
        close(cout_or_cerr);
        pipe(my_pipe);
        pid_t son1 = -1;
        pid_t son2 = -1;
        //make first sons
        son1 = fork();
        //also detach the son1 from ctrlz/c handler
        if(son1==-1){
            perror("smash error: fork failed");
            return;
        }
        if (son1 == 0) {
            smash.setIsApipeFather(false);
            smash.setIsAson(true);
            //son for command1;
            close(my_pipe[0]); // close the read chanel of san 2;
            dup(cin_place);
            close(cin_place);
            smash.executeCommand(first_part.c_str());
            close(my_pipe[1]); // close the write channel of the son 1 pipe/
            close(cout_place);
            smash.executeCommand("quit");
            return;
        } else {
            son2 = fork();
            if (son2 == -1) {
                perror("smash error: fork failed");
            }
            if (son2 == 0) {
                smash.setIsApipeFather(false);
                smash.setIsAson(true);
                //son for command2;
                close(my_pipe[1]); // close the write channel
                dup(cout_place);
                close(cout_place);
                smash.executeCommand(last_part.c_str());
                close(my_pipe[0]);
                smash.executeCommand("quit");
                return;
            }
        }
        //set the sons
        smash.setSons(son1,son2);
        //close the pipe of the pipe_father_process
        close(my_pipe[0]);
        close(my_pipe[1]);

        //wait for son1 and son2
        while ((waitpid(son1, &status, WUNTRACED) !=-1) ||
                (waitpid(son2, &status, WUNTRACED) !=-1)){};

        //REMEMBER KILL THE SONS
        smash.executeCommand("quit");
        return;
    }
    // Now we're back in the smash process
    else if((need_to_wait)) {
        waitpid(pipe_father_process, &status, WUNTRACED);
    };
    smash.setFgPid(-1);
}

CopyCommand::CopyCommand(const char *cmd_line, JobsList* jobs):ExternalCommand(cmd_line, jobs){
    name = "cp";
}

void CopyCommand::execute() {
    if(size_of_args<3){
        return;
    }
    // Checks if the paths are equal
    char* from_path = new char[MAXPATH];
    char* to_path = new char[MAXPATH];
    realpath(args[1], from_path);
    realpath(args[2], to_path);
    if(string(from_path).compare(string(to_path))==0){
        int from_file = open(args[1],  O_RDONLY);
        if(from_file !=-1){
            cout<<"smash: " << args[1] <<" was copied to "<< args[2] <<endl;
            close(from_file);
        }else{
            perror("smash error: open failed");
        }
        delete[] from_path;
        delete[] to_path;
        return;
    }
    delete[] from_path;
    delete[] to_path;
    pid_t pid = fork();
    if(pid==-1){
        perror("smash error: fork failed");
        return;
    }
    if(pid==0){
        SmallShell& smash = SmallShell::getInstance();
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        int from_file = open(args[1],  O_RDONLY);
        if(from_file ==-1){
            perror("smash error: open failed");
            return;
        }
        int to_file = open(args[2], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        if(to_file ==-1){
            close(from_file);
            perror("smash error: open failed");
            return;
        }
        char buffer[4096];
        int num_of_bytes =0;
        do{
            num_of_bytes = read(from_file, buffer, 4096);
            if(num_of_bytes == -1){
                perror("smash error: read failed");
                return;
            }
            if(num_of_bytes == 0){
                break;
            }
            if(write(to_file, buffer, num_of_bytes)==-1){
                perror("smash error: write failed");
                return;
            }
        }while (num_of_bytes>0);
        close(from_file);
        close(to_file);
        cout<<"smash: " << args[1] <<" was copied to "<< args[2] <<endl;
        smash.executeCommand("quit");
    }
    else{
        SmallShell& smash = SmallShell::getInstance();
        if(isBackground){
            jobs->addJob(this, pid, false);
        }
        else{
            int status;
            smash.setFgPid(pid);
            smash.setSpecialProcess(pid); // add this to a pipe of son
            smash.setFgCommand(this);
            if(waitpid(pid, &status, WUNTRACED)==-1){
                perror("smash error: waitpid failed");
            }
            if(WIFSTOPPED(status)){
                to_delete_imm = false;
            }
            smash.setFgPid(-1);
        }
    }
}

TimeoutCommand::TimeoutCommand(const char *cmd_line, JobsList *jobs):ExternalCommand(cmd_line, jobs) {
    name = "timeout";
}

void TimeoutCommand::execute() {
    if(size_of_args<3){ // TODO check if 2 or 3
        this->set_error("invalid arguments");
        return;
    }
    if (!(is_number(string(args[1])))) {
        set_error("invalid arguments");
        return;
    }
    int timeout = stringToInt(string(args[1]));
    if (timeout <= 0){
        set_error("invalid arguments");
        return;
    }
    char* internal_cmd = new char[string(cmd).length() + 1];
    strcpy(internal_cmd, cmd);
    string cutout_int_cmd = string(internal_cmd);
    cutout_int_cmd = _removeFirstWord(cutout_int_cmd);
    cutout_int_cmd = _removeFirstWord(cutout_int_cmd);
    SmallShell& smash = SmallShell::getInstance();

    Command* cmd_to_execute = smash.CreateCommand(cutout_int_cmd.c_str());
    cmd_to_execute->set_cmd(internal_cmd);
    cmd_to_execute->timeout = timeout;
    cmd_to_execute->execute();

    // This means it's not an external command
    if(dynamic_cast<ExternalCommand*>(cmd_to_execute) == nullptr){
        smash.timeout->addAlarm(-1, timeout, nullptr);
    }

    if(cmd_to_execute->to_delete_imm){
        delete cmd_to_execute;
    }
    delete[] internal_cmd;
}

//Timeout object
time_t Timeout::TimeoutEntry::getRemainingTime() {
    return alarm_time - (time(nullptr) - start_time);
}

Timeout::Timeout(): time_list(new list<Timeout::TimeoutEntry>()){}
Timeout::~Timeout() {
    delete time_list;
}

Timeout::TimeoutEntry::TimeoutEntry(int alarm_time, pid_t pid, Command *cmd):alarm_time(alarm_time),
start_time(time(nullptr)),pid(pid), cmd(cmd){}


bool compareTimeout(Timeout::TimeoutEntry e1, Timeout::TimeoutEntry e2) {
    return e1.getRemainingTime() < e2.getRemainingTime();
}

void Timeout::addAlarm(pid_t pid, time_t time, Command* cmd) {
    time_list->push_back(Timeout::TimeoutEntry(time, pid, cmd));
    time_list->sort(compareTimeout);
    alarm(time_list->front().getRemainingTime());
}