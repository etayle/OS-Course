
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <cstdlib>     /* srand, rand */
#include <ctime>
using  namespace std;
int stringToInt(const string& a){
    stringstream job_string(a);
    int job_num;
    job_string >> job_num;
    return job_num;
}
string add_paramters(string a){
    if(a == "chprompt"){
        int number = rand() % 4   ;
        string chrompt[4]={" "," "," test"," etayle"};
        a.append(chrompt[number]);
        if(number>3) {
            number = rand() % 4;
            if (number == 4) {
                a.append("zevel");
            }
        }
        return a;
    }
    if(a=="cd"){
       int number = rand() % 3   ;
       if(number == 0){
           a.append(" -");
           return a;
       }
       else{
           a.append(" ..");
       }
    }
    if(a=="kill") {
        int number = rand() % 3;
        int sign_num[3] = {SIGCONT, SIGSTOP, SIGKILL};
        a.append(" -");
        a.append(to_string(sign_num[number]) );
        a.append(" ");
        number = rand() % 3 + 1;
        a.append(to_string(number));
    }
    if(a=="fg"){
        int number = rand()%3;
        a.append(" ");
        if(number==3){
            return a;
        }else{
            a.append(to_string(rand()%5+1));
            return a;
        }
    }
    int zevel = rand()%7;
    if(zevel == 0){
        a.append(" zevel");
    }
    return a;
}
void print_Built_in_commands(){
    string Built_in_commands[8]={"showpid","chprompt","pwd","jobs","cd","kill","bg","fg"};
    int a = rand() % 8 ;
    cout<<add_paramters(Built_in_commands[a])<<endl;
}
void print_External_commands(){
    string External_commands[1] = {"sleep"};
    int a = rand() % 5  +3 ;
    cout<<External_commands[0 ]<<" " << a <<"&" <<endl;
}
int main() {
    for(int i =0;i<3;i++){
        string External_commands[1] = {"sleep"};
        int a = rand() % 5  +3 ;
        cout<<External_commands[0 ]<<" " << a <<"&" <<endl;
    }
    srand(time(NULL));
    for(int i =0;i<1000;i++){
        int random = rand() % 5   ;
        if(random == 0 ){
            print_External_commands();
        } else{
            print_Built_in_commands();
        }

    }

    cout<<"quit kill" <<endl;
   return 0;
}

