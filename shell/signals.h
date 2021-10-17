#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

void ctrlZHandler(int sig_num);
void ctrlCHandler(int sig_num);
void alarmHandler(int sig_num);
void sigContHandler(int sig_num);
void ctrlZforkFatherHandler(int sig_num);
void ctrlZforkSonHandler(int sig_num);
void ctrlCforkFatherHandler(int sig_num);
void ctrlCforkSonHandler(int sig_num);
#endif //SMASH__SIGNALS_H_
