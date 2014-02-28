#include <iostream>
#include <map>
#include <queue>
#include <cstdlib>
#include <cstring>

using namespace std;

#define NUM_USERS 4
#define MAX_MESSAGES 1000
#define MAX_BUFFER_SIZE 20

//create a struct for messages
struct message{
	int type;
	char * message;
	pthread_t sender_id;
	pthread_t receiver_id;
};

//create a struct for process data
struct process_data{
   int  thread_id;
   string file_name;
};

struct process_data process_data_array[NUM_USERS];

void* userProcess(void* arg){
	struct process_data *my_data;
   	my_data = (struct process_data *) arg;
   	pthread_t thread_id = my_data->thread_id;
   	string file_name = my_data->file_name;

	/*//read file line by line
	FILE * ifile = fopen(data_file.c_str(), "r");

	while(!feof(ifile))
    {
        if(fscanf(ifile, "%u,%u,%f,%c,%u,%u,%u", &id, &m.timestamp, &m.value, &m.property, &m.plug_id, &m.household_id, &house_id) < 7)
            continue;

        // send the message
        if ( house_id < subscribers )
        {
        	write(con_map[house_id], &m, sizeof(m));
        	count++;
        }
        if(count == stat)
        {
            ctime = time(NULL);
            cerr<<"Throughput = "<<count/(ctime-ptime+1)<<endl;
            ptime = ctime;
            count = 0;
        }
    }*/
	pthread_exit(NULL);
}

int main (){
	pthread_t thread[NUM_USERS];
   	pthread_attr_t attr;

	int buffer_size = 0;
	int messages_handled = 0;
	//create map for storing messages to be sent
	map<pthread_t, queue<struct message>  > buffer;


	// Initialize and set thread detached attribute
   	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   	for(int t=0; t < NUM_USERS; t++) {
   		string file_name;
   		cout << "Enter file name for thread : ";
   		cin >> file_name;
   		process_data_array[t].thread_id = t;
   		process_data_array[t].file_name = file_name;
      	int rc = pthread_create(&thread[t], &attr, userProcess, (void *) &process_data_array[t]);
      	if (rc) {
         	cout << "ERROR; return code from pthread_create() is "<< rc << endl;
         	exit(-1);
        }
       	buffer[thread[t]] = queue<struct message> ();
    }

    //handle ipc job
    /*while(messages_handled < MAX_MESSAGES){
    	//if next request
	    	//if type == 1
	    		//block the thread
	    	//else
	    		//if buffer is full
	    			//block the thread, indicate it to resend the message
	    		//else
	    			//add the message to the buffer
    	//else if buffer not empty
    		//check who is waiting for receive
    		//if he has a message, send it and unblock the receiver
    		//messages_handled ++
    		//unblock a sender
    }*/

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