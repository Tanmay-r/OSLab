#include <iostream>
#include <map>
#include <queue>
#include <cstdlib>
#include <cstring>
#include <fstream>

using namespace std;

#define NUM_USERS 4
#define MAX_MESSAGES 1000
#define MAX_BUFFER_SIZE 20

//create a struct for messages
struct message{
	int type;
	string msg;
	pthread_t sender_id;
	pthread_t receiver_id;
};

//create a struct for process data
struct process_data{
   int  thread_id;
   string file_name;
};

vector<string> split(string input , char c){
	vector<string> list;
	string st="";
	int size= input.size();
	for(int i=0;i<size;i++){
		if(input[i]==c || i==size-1){
			if(i==size-1)
				st+=input[i];
			list.push_back(st);
			st="";
		}
		else{
			st+=input[i];
		}
	}
	return list;
}

process_data process_data_array[NUM_USERS];

pthread_mutex_t buffer_mutex;
pthread_cond_t buffer_cv;

pthread_cond_t receive_cv[NUM_USERS];

pthread_mutex_t receive_status_mutex[NUM_USERS];
int receive[NUM_USERS];

vector<message> buffer(MAX_BUFFER_SIZE);
int buffer_size;

void* userProcess(void* arg){
	process_data *my_data;
   	my_data = (process_data *) arg;
   	int thread_id = my_data->thread_id;
   	string file_name = my_data->file_name;
   	//read file line by line
	string message1;
	ifstream infile;
	infile.open(file_name.c_str());
	while(getline(infile, message1)){
		vector<string> v = split(message1, ',');
		message request;
		request.type = atoi(v[0].c_str());
		request.msg = v[1];
		request.sender_id = thread_id;
		request.receiver_id = atoi(v[2].c_str());

		//cout << "Message bana = "<< request.msg << endl;
		//lock this buffer
		if(request.type == 0){

			pthread_mutex_lock(&buffer_mutex);

			if(buffer_size >= MAX_BUFFER_SIZE){
				pthread_cond_wait(&buffer_cv, &buffer_mutex);
			}
			else{
				buffer[buffer_size] = request;
				//cout << "buffer message = " << buffer[buffer_size].msg << endl;
				buffer_size++;
			}
			pthread_mutex_unlock(&buffer_mutex);
		}
		else{
			pthread_mutex_lock(&receive_status_mutex[thread_id]);
			receive[thread_id] = 1;
			pthread_mutex_unlock(&receive_status_mutex[thread_id]);
			pthread_cond_wait(&receive_cv[thread_id], &buffer_mutex);
			pthread_mutex_lock(&buffer_mutex);
			for(int i = 0; i < buffer_size; i++){
				if(buffer[i].receiver_id == thread_id){
					buffer.erase(buffer.begin() + i);
					break;
				}
			}
			buffer_size--;
			pthread_cond_broadcast(&buffer_cv);
			pthread_mutex_unlock(&buffer_mutex);
		}

	}
	pthread_exit(NULL);
}

int main (){
	pthread_t thread[NUM_USERS];
   	pthread_attr_t attr;

   	for(int i = 0; i < NUM_USERS; i++){
   		receive[i] = 0;
   	}

	buffer_size = 0;
	int messages_handled = 0;


	// Initialize and set thread detached attribute
   	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   	//Initialize mutexes
   	pthread_mutex_init(&buffer_mutex, NULL);
  	pthread_cond_init (&buffer_cv, NULL);
  	for(int i = 0; i < NUM_USERS; i++){
  		pthread_cond_init(&receive_cv[i], NULL);
  	}

   	for(int t=0; t < NUM_USERS; t++) {
   		string file_name;
   		//cout << "Enter file name for thread " << t << " : ";
   		cin >> file_name;
   		process_data_array[t].thread_id = t;
   		process_data_array[t].file_name = file_name;
      	int rc = pthread_create(&thread[t], &attr, userProcess, (void *) &process_data_array[t]);
      	if (rc) {
         	cout << "ERROR; return code from pthread_create() is "<< rc << endl;
         	exit(-1);
        }
    }

    //handle ipc job
    while(messages_handled < MAX_MESSAGES){
    	message request;
    	pthread_mutex_lock(&buffer_mutex);
    	for(int i=0; i < buffer_size; i++){
    		if(buffer[i].type == 0){
    			if(receive[buffer[i].receiver_id] == 1){
					pthread_mutex_lock(&receive_status_mutex[buffer[i].receiver_id]);
					receive[buffer[i].receiver_id] = 0;
					cout << "Message received: " << buffer[i].sender_id << ", " << buffer[i].receiver_id << ", " << buffer[i].msg << endl;
					messages_handled++;
					pthread_mutex_unlock(&receive_status_mutex[buffer[i].receiver_id]);
    				pthread_cond_signal(&receive_cv[buffer[i].receiver_id]);
    			}
    			else{
    				continue;
    			}
    		}
    	}
		pthread_mutex_unlock(&buffer_mutex);
    }

    // Free attribute and wait for the other threads
   	pthread_attr_destroy(&attr);
   	for(int t=0; t<NUM_USERS; t++) {
      	int rc = pthread_join(thread[t], NULL);
      	if (rc) {
         	cout << "ERROR; return code from pthread_join() is "<< rc << endl;
         	exit(-1);
        }
    }



}