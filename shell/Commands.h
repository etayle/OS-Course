#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <time.h>
#include <string>
using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define NO_DIR ""

typedef enum jobStatus {ENDED,BACKGROUND,STOP} jobStatus;
typedef enum printFunction {JOBS,QUIT} printFunction;

class Command {
protected:
    string name;
    char** args;
    int size_of_args;
    const char* cmd;
    const char* string_cmd;
    bool external_command;


 public:
    int timeout = -1;
    bool to_delete_imm = true;
  explicit Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  void set_error(const string error);
  string get_error_begin();
  string get_cmd_string();
  string get_cmd_name();
  void set_cmd(const char* cmd);
  void setIsExternalCommand(bool);
  bool isExternalCommand();
  //virtual void prepare();
  //virtual void cleanup();
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() = default;
};

class JobsList;
class ExternalCommand : public Command {
protected:
    bool isBackground;
    JobsList* jobs;

 public:
  ExternalCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  bool need_to_wait;
  bool std_error;
  string first_part;
  string last_part;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
    bool override;
    string first_part;
    string last_part;

 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    char** last_dir;
   bool is_last_dir;
public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {
  }
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  explicit GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  explicit ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() = default;
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsList {
public:
    class JobEntry {
    public:
       int jobId;
       pid_t processId;
       string commandName;
       jobStatus status;
       time_t startTime;
       Command* cmd;
       void printJob(printFunction func);
    };

    vector<JobEntry> jobsList;
public:
    JobsList();
    ~JobsList(){}
    void addJob(Command* cmd, pid_t processId, bool isStopped = false);
    void printJobsList(printFunction func);
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    JobEntry * getJobByPid(pid_t pid);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobs;

public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs;
public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() = default;
  void execute() override;
};


class CopyCommand : public ExternalCommand {
 public:
  CopyCommand(const char* cmd_line, JobsList* jobs);
  virtual ~CopyCommand() {}
  void execute() override;
};

class TimeoutCommand : public ExternalCommand{
public:
    TimeoutCommand(const char* cmd_line, JobsList* jobs);
    virtual ~TimeoutCommand(){}
    void execute() override;
};


class Chprompt : public BuiltInCommand{
public:
    Chprompt(const char* cmd_line);
    virtual ~Chprompt() {}
    void execute() override;
    string nameToReturn();
};

class Timeout{



public:
    class TimeoutEntry{
    public:
        time_t alarm_time;
        time_t start_time;
        pid_t pid;
        Command* cmd;


        TimeoutEntry(int alarm_time, pid_t pid, Command* cmd);
        ~TimeoutEntry(){}
        time_t getRemainingTime();
    };

    Timeout();
    ~Timeout();
    void addAlarm(pid_t pid, time_t time, Command* cmd);
    list<TimeoutEntry>* time_list;
};

class SmallShell {
 private:
  pid_t  mypid;
  char* last_directory = nullptr;
  string chprompt_string;
  string name_shell;
  JobsList* shell_joblist;


  pid_t fg_pid;
  Command* fg_cmd;
  SmallShell();

  //new varibale to fork signals
  bool is_pipe_father_process;
  bool is_son;
  pid_t special_process;
  pid_t son1;
  pid_t son2;
  bool need_wait_to_pipe;
 public:
    struct sigaction* act;
    Timeout* timeout;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  pid_t getmypid();
  void executeCommand(const char* cmd_line);
  void executeCommand(const char* cmd_line, const char* string_cmd);
  string name();
  void changeName(basic_string<char> name);
  JobsList* getJobList();
  void setFgPid(pid_t pid);
  pid_t getFgPid();
  void setFgCommand(Command* cmd);
  Command* getFgCommand();

  void setIsApipeFather(bool bolean);
  void setIsAson(bool bolean);
  void setSpecialProcess(pid_t pid);
  bool getIsAfather( );
  bool getIsAson();
  pid_t getSpecialProcess();
  void setSons(pid_t son1,pid_t son2);
  void getSons(pid_t * son1,pid_t * son2);
  void setNeedWait(bool boolean);
  bool needWaitPipe();
};


#endif //SMASH_COMMAND_H_
