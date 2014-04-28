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
	int sender_id;
	int receiver_id;
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


//Mutexes and condition variables
pthread_mutex_t request_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t request_buffer_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t message_container_mutex[NUM_USERS] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t message_container_cv[NUM_USERS] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER,PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t message_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t message_buffer_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_overflow_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t message_handler_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t message_handler_blocked_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t message_handler_blocked_cv = PTHREAD_COND_INITIALIZER;

//shared data
queue<message> request_buffer;
vector<message> message_buffer(MAX_BUFFER_SIZE);
int buffer_size;
int receive_ready[NUM_USERS];
message message_container[NUM_USERS];
int message_handler_blocked;

//user_process
void* user_process(void* arg){
	process_data *my_data;
   	my_data = (process_data *) arg;
   	int thread_id = my_data->thread_id;
   	string file_name = my_data->file_name;
   	//cout << __LINE__ << endl;
   	//read file line by line
   	//cout << "user process = " << thread_id << endl;
	string line;
	ifstream infile;
	infile.open(file_name.c_str());
	while(getline(infile, line)){
		//cout << __LINE__ << endl;
		vector<string> v = split(line, ',');
		message request;
		request.type = atoi(v[0].c_str());
		request.msg = v[1];
		request.sender_id = thread_id;
		request.receiver_id = atoi(v[2].c_str());

		//send request
		if(request.type == 0){
			cout << "line-0 " << line << endl;
			pthread_mutex_lock(&request_buffer_mutex);
			request_buffer.push(request);
			if(request_buffer.size() == 1){
				pthread_cond_signal(&request_buffer_cv);
			}
			pthread_mutex_unlock(&request_buffer_mutex);
		}
		//receive request
		else if(request.type == 1){
			cout << "line-1 " << line << endl;
			pthread_mutex_lock(&request_buffer_mutex);
			//cout << "receive request " << endl;
			request_buffer.push(request);
			if(request_buffer.size() == 1){
				pthread_cond_signal(&request_buffer_cv);
			}
			pthread_mutex_unlock(&request_buffer_mutex);

			pthread_mutex_lock(&message_container_mutex[thread_id]);
			receive_ready[thread_id] = 1;
			cout << "waiting  = " << thread_id << endl;
			pthread_cond_wait(&message_container_cv[thread_id], &message_container_mutex[thread_id]);
			//cout << __LINE__ << endl;
			receive_ready[thread_id] = 0;
			cout << "waiting later = " << thread_id << endl;
			//cout"received message = %d, %s\n", thread_id, (message_container[thread_id].msg).c_str());
			pthread_mutex_unlock(&message_container_mutex[thread_id]);
		}

	}
	pthread_exit(NULL);
}

//ipc process that reads from request_buffer
void* ipc_request_handler(void* arg){
	//cout << __LINE__ << endl;
	while(1){
		pthread_mutex_lock(&request_buffer_mutex);
		if(request_buffer.size() == 0){
			//cout << __LINE__ << endl;
			pthread_cond_wait(&request_buffer_cv, &request_buffer_mutex);
		}
		else{
			//cout << __LINE__ << endl;
			message rqst = request_buffer.front();
			//cout << __LINE__ << endl;
			request_buffer.pop();
			if(rqst.type == 0){
				pthread_mutex_lock(&message_buffer_mutex);
				if(buffer_size >= 20){
					//cout << __LINE__ << endl;
					pthread_cond_wait(&buffer_overflow_cv, &message_buffer_mutex);
				}
				//cout << "buffer_size prev= " << buffer_size << endl;
				message_buffer[buffer_size] = rqst;
				buffer_size++;
				//cout << "buffer_size later= " << buffer_size << endl;
				cout << "Message sent: " << rqst.sender_id << ",";
				cout << rqst.receiver_id << "," << rqst.msg << endl;
				if(buffer_size == 1){
					//cout << __LINE__ << endl;
					pthread_cond_signal(&message_handler_cv);
				}
				pthread_mutex_unlock(&message_buffer_mutex);
			}
			else if(rqst.type == 1){
				//cout << "endl " << endl;
				pthread_mutex_lock(&message_handler_blocked_mutex);
				if(message_handler_blocked == 1){
					message_handler_blocked = 0;
					pthread_cond_signal(&message_handler_blocked_cv);
				}
				pthread_mutex_unlock(&message_handler_blocked_mutex);
			}
		}
		pthread_mutex_unlock(&request_buffer_mutex);
	}
	pthread_exit(NULL);
}

//ipc process that reads from message_buffer
void* ipc_message_handler(void* arg){
	//cout << __LINE__ << endl;
	int flag = 0;
	pthread_mutex_lock(&message_handler_blocked_mutex);
	message_handler_blocked = 0;
	pthread_mutex_unlock(&message_handler_blocked_mutex);
	while(1){
		//cout << __LINE__ << endl;
		pthread_mutex_lock(&message_buffer_mutex);
		if(buffer_size == 0){
			//cout << __LINE__ << endl;
			pthread_cond_wait(&message_handler_cv, &message_buffer_mutex);
			//cout << __LINE__ << endl;
		}
		for(int i = 0; i < buffer_size; i++){
			int recvId = message_buffer[i].receiver_id;
			//cout << __LINE__ << endl;
			pthread_mutex_lock(&message_container_mutex[recvId]);
			if(receive_ready[recvId] == 1){
				cout << "Message received: " << message_buffer[i].sender_id << ",";
				cout << recvId << "," << message_buffer[i].msg << endl;
				message_container[recvId] = message_buffer[i];
				message_buffer.erase(message_buffer.begin()+i);
				buffer_size--;
				//cout << << endl;
				//printf("recvId = %d\n", recvId);
				cout << "recvId = " << recvId << endl;
				pthread_cond_signal(&message_container_cv[recvId]);
				//printf("recvId later = %d\n", recvId);
				cout << "recvId later= " << recvId << endl;
				flag = 1;
				break;
			}
			pthread_mutex_unlock(&message_container_mutex[recvId]);
		}
		if(flag == 0){
			cout << "flag-0 " << endl;
			//cout << __LINE__ << endl;
			pthread_mutex_lock(&message_handler_blocked_mutex);
			message_handler_blocked = 1;
			pthread_cond_wait(&message_handler_blocked_cv, &message_handler_blocked_mutex);
			pthread_mutex_unlock(&message_handler_blocked_mutex);
		}
		flag = 0;
		pthread_mutex_unlock(&message_buffer_mutex);
	}

	pthread_exit(NULL);
}

int main (){
	pthread_t thread[NUM_USERS];
	pthread_t thread_req, thread_msg;
   	pthread_attr_t attr;

   	for(int i = 0; i < NUM_USERS; i++){
   		receive_ready[i] = 0;
   	}

	buffer_size = 0;

	// Initialize and set thread detached attribute
   	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   	//Initialize mutexes

  	//create threads
   	for(int t=0; t < NUM_USERS; t++) {
   		string file_name;
   		//cout << "Enter file name for thread " << t << " : ";
   		cin >> file_name;
   		process_data_array[t].thread_id = t;
   		process_data_array[t].file_name = file_name;
      	int rc = pthread_create(&thread[t], &attr, user_process, (void *) &process_data_array[t]);
      	if (rc) {
         	cout << "ERROR; return code from pthread_create() is "<< rc << endl;
         	exit(-1);
        }
    }

    int rc = pthread_create(&thread_req, &attr, ipc_request_handler, NULL);
  	if (rc) {
     	cout << "ERROR; return code from pthread_create() is "<< rc << endl;
     	exit(-1);
    }

    rc = pthread_create(&thread_msg, &attr, ipc_message_handler, NULL);
  	if (rc) {
     	cout << "ERROR; return code from pthread_create() is "<< rc << endl;
     	exit(-1);
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
    rc = pthread_join(thread_req, NULL);
    if (rc) {
     	cout << "ERROR; return code from pthread_join() is "<< rc << endl;
     	exit(-1);
    }

    rc = pthread_join(thread_msg, NULL);
    if (rc) {
     	cout << "ERROR; return code from pthread_join() is "<< rc << endl;
     	exit(-1);
    }
}