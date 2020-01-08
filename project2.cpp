#include <stdio.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;
ofstream outfile; //to write from other threads it is defined globally
int clientNum;//this is the number given in the first line of input. Number of customers
atomic<int> clientCounter{0};//this atomic variable used for count how many customer payment is processed
atomic<int> totals[5];//this array is for variables of total number of bill types
int currentClient[10];//Used to find current customer in an atm
pthread_mutex_t ATM_mutex[10];// these mutexes is used for locking a specific atm
pthread_mutex_t mutexWrite=PTHREAD_MUTEX_INITIALIZER;//Used to make only one atm writes to file in a time
pthread_cond_t cv[10];//condition variables for customers to atms communication
pthread_cond_t cv2[10];//condition variables for atms to customers communication
pthread_mutex_t mutexSignal[10];// mutex for condition variables between customers and atms
struct clientInfo //Struct of customers informations
{
	int id;//customer id
	int sleepTime;// sleep time for customer
	int atmNum;// which atm customer will use
	string billType;//which type of bill will be payed
	int amount;
};
clientInfo arr[300];// all structs of customers informations are kept in an array
void *client(void *input){ // Method for customers thread
	clientInfo *info = (clientInfo*)input;// input struct will be used with info variable
	usleep(info->sleepTime*1000);// a customer will sleep for given miliseconds. Multiplied with 1000 to convert microsecond to milisecond
	pthread_mutex_lock(&ATM_mutex[info->atmNum-1]);// locked an atm for only one customer
	pthread_mutex_lock(&mutexSignal[info->atmNum-1]);//mutex for signaling
	currentClient[info->atmNum-1]=info->id; //current customer in atm is equalized to customer who took the lock
	pthread_cond_signal(&cv[info->atmNum-1]);// gives the signal for wait in atm threads
	pthread_cond_wait(&cv2[info->atmNum-1],&mutexSignal[info->atmNum-1]);// waits back a signal from atm 
	pthread_mutex_unlock(&mutexSignal[info->atmNum-1]);//unlocks the locks
	pthread_mutex_unlock(&ATM_mutex[info->atmNum-1]);
}
void *atm(void*input){//Method for atm threads
	int id=*((int*)input); //id is given by input
	pthread_mutex_lock(&mutexSignal[id-1]);// takes the lock for condition variable
	while(clientCounter<clientNum){ // atm keeps working until all customers are satisfied
		pthread_cond_wait(&cv[id-1],&mutexSignal[id-1]); //waits a signal from client method
		pthread_mutex_lock(&mutexWrite);//mutex locked for only one atm to write
		string finalLine="Customer"; //all informations are gathered from struct array of customers
		string x=to_string(currentClient[id-1]);// they are concataned for a string to write to outfile
		finalLine=finalLine+x;
		x=to_string(arr[currentClient[id-1]-1].amount);
		finalLine=finalLine+","+x+"TL,";
		x=arr[currentClient[id-1]-1].billType;
		finalLine=finalLine+x;
		outfile<<finalLine<<endl;// final string is written to outfile
		if(arr[currentClient[id-1]-1].billType=="cableTV"){ // Total numbers paid for a specific bill type is calculated by adding
			totals[0]=totals[0]+arr[currentClient[id-1]-1].amount;
		}else if(arr[currentClient[id-1]-1].billType=="electricity"){
			totals[1]+=arr[currentClient[id-1]-1].amount;
		}else if(arr[currentClient[id-1]-1].billType=="gas"){
			totals[2]+=arr[currentClient[id-1]-1].amount;
		}else if(arr[currentClient[id-1]-1].billType=="telecommunication"){
			totals[3]+=arr[currentClient[id-1]-1].amount;
		}else{
			totals[4]+=arr[currentClient[id-1]-1].amount;
		}
		pthread_mutex_unlock(&mutexWrite);// unlocks the write mutex
		pthread_cond_signal(&cv2[id-1]);//signals the customers that its operations are ended
		clientCounter++;//counter is increased
	}
	pthread_mutex_unlock(&mutexSignal[id-1]);//unlocks the signal mutex
	
	
}
int main(int argc, char*argv[])
{

	for(int i=0;i<10;i++){// all mutexes and condition variables are initialized
		mutexSignal[i]=PTHREAD_MUTEX_INITIALIZER;
		cv[i]=PTHREAD_COND_INITIALIZER;
		cv2[i]=PTHREAD_COND_INITIALIZER;
		ATM_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
	}
	string inputName=argv[1];
	ifstream myfile(argv[1]);//input is gathered from input file
	string line;
	bool t=true;
	int k=0;
	while(getline(myfile,line)){
		istringstream ss(line);
		if(t){
			ss>>clientNum;
			t=false;
		}else{
			string comma;// all of them is equalized to elements of structs
			ss>>arr[k].sleepTime;
			getline(ss,comma,',');
			ss>>arr[k].atmNum;
			getline(ss,comma,',');
			getline(ss,arr[k].billType,',');
			ss>>arr[k].amount;
			k++;
		}
	}
	string outName=inputName+"_log.txt";//outfile is opened with the specified name
	outfile.open(outName);
	pthread_t atmThreads[10];// atm thread variables are created
	int numArr[10];
	for(int i=0;i<10;i++){
		numArr[i]=i+1;
	}
	for(int i=0;i<10;i++){
		pthread_create(&atmThreads[i],NULL,&atm,&numArr[i]); // 10 atm thread is created and their id is given as argument
	}
	pthread_t clientThreads[300];
	for(int i=0;i<clientNum;i++){
		arr[i].id=i+1;
		pthread_create(&clientThreads[i],NULL,&client,&arr[i]);//clients are created and struct of that customers given as argument
	}
	for(int i=0;i<clientNum;i++){
		pthread_join(clientThreads[i],NULL);// customer threads are joined
	}
	outfile<<"All payments are completed"<<endl; //total number of bill types are printed to outfile
	for(int i=0;i<5;i++){
		if(i==0){
			outfile<<"CableTv: ";
		}else if(i==1){
			outfile<<"Electricity: ";
		}else if(i==2){
			outfile<<"Gas: ";
		}else if(i==3){
			outfile<<"Telecommunication: ";
		}else{
			outfile<<"Water: ";
		}
		outfile<<totals[i]<<"TL"<<endl;
	}
	outfile.close();
	return 0;
}
/*
myShell: project1.cpp
	g++ -o myShell project1.cpp
*/