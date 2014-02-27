#include <iostream>
#include <sstream>
#include <string>
#include <queue> 
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <cstdlib>
using namespace std;

enum event_type{ADMISSION, IO_OVER, CPU_OVER, TS_OVER};
enum process_state{RUNNING,READY,BLOCKED,TERMINATED};

struct event{	
	int time;
	int p_id;
	event_type type;	
};

struct processData{
	int total_cpu_time;
	int total_io_time;
	int start_time;
	int end_time;
};


struct pcb{

	int p_id;
	int phase_id;
	process_state state;
	int priority;

};

struct eventComparator{
	bool operator()(const event*a ,const event* b){
		return a->time > b->time; 	
	}

};

bool pcbComparator (pcb * a, pcb *b) { return (a->priority > b->priority); }


struct process_phase{
	int iterations;
	int cpu_time;
	int io_time;
};

struct process{
	int p_id;
	int start_priority;
	int admission;
	vector<process_phase> phases;
};

struct sc_level{
	int level_number;
	int priority;
	int time_slice;
};

struct scheduler{
	int no_levels;
	vector<sc_level> levels;
};




string report;
priority_queue<event* , vector<event*> , eventComparator >  event_table;
vector<pcb* > pcb_list;
vector<process> process_list;
scheduler my_scheduler;
bool cpu_idle; 
int system_clock;
map<int,int> cpu_time_reqd;
event temp_event;


queue<pcb* > pcb_list_time_sharing;
map<int, pcb* > blocked_pcbs;
pcb * running_pcb;
int time_slice;


map<int, queue<pcb *> > pcb_list_multilevel;

void process_proc_file(){
	string line, line2;
	int pid, prior;
	int adm;
	int iter;
	int cpu_t, io_t;
	ifstream infile("PROCESS_SPEC");
	while (std::getline(infile, line))
	{
		if(line=="PROCESS"){
			process proc;
			getline(infile, line2);
			std::istringstream iss(line2);
		    if (!(iss >> pid >> prior >> adm)) { break; } // error
		    // cout<<pid<<endl<<prior<<endl;
			
			proc.p_id = pid;
			proc.start_priority = prior;
			proc.admission = adm;

			getline(infile, line2);
			while(line2 != "END"){
				std::istringstream iss(line2);
				process_phase pp;
			    if (!(iss >> iter >> cpu_t >> io_t)) { break; } // error
			    // cout<<cpu_t<<endl<<io_t<<endl;
			    
			    pp.iterations = iter;
			    pp.cpu_time = cpu_t;
			    pp.io_time = io_t;
			    (proc.phases).push_back(pp);
			    getline(infile, line2);
			}
			process_list.push_back(proc);
		}
	}
	// cout<<(process_list[1].phases[1]).io_time<<endl;
	// return 0;
}

void process_scheduler_file(){
	string line, line2;
	int level_count;
	int prior;
	int s_lvl;
	int t_slice;
	ifstream infile("SCHEDULER_SPEC");
	while (std::getline(infile, line))
	{
		if(line=="SCHEDULER"){
			getline(infile, line2);
			std::istringstream iss(line2);
		    if (!(iss >> level_count)) { break; } // error
		    // cout<<pid<<endl<<prior<<endl;
			
			my_scheduler.no_levels = level_count;
			for(int i=0; i<level_count; i++){
				getline(infile, line2);
				std::istringstream iss(line2);
				if (!(iss >> s_lvl >> prior >> t_slice)) { break; } // error
				sc_level scl;
				scl.level_number = s_lvl;
				scl.priority = prior;
				scl.time_slice = t_slice;
				my_scheduler.levels.push_back(scl);
			}
		}
	}
}

void initializeEventTable(){
	int num_of_process = process_list.size();
	for (int i = 0; i < num_of_process ;i++){
		event * newevent = new event ();
		newevent->time = process_list[i].admission;
		newevent->p_id = process_list[i].p_id;
		newevent->type = ADMISSION;
		event_table.push(newevent);
	}
}

void scheduleProcessMultiprogramming(){
	int num_of_pcb = pcb_list.size();
	for (int i = 0; i < num_of_pcb ;i++){
		if(pcb_list[i]->state == RUNNING){
			break;
		}

		if(pcb_list[i]->state == READY){
			if(cpu_idle){
				temp_event.p_id = pcb_list[i]->p_id;
				temp_event.time = system_clock + cpu_time_reqd[pcb_list[i]->p_id];
				report += "At time = ";
				stringstream ss1;
				ss1 << system_clock;
				string temp = ss1.str();
				report += temp;
				report += ", Process with pid = ";
				stringstream ss2;
				ss2 << temp_event.p_id;
				string temp2 = ss2.str();
				report += temp2;
				report += " was scheduled.\n";
				pcb_list[i]->state = RUNNING;
				cpu_idle = 0;
				break;
			}
			else{
				int preempt_id = 0;
				for(int j = 0; j < num_of_pcb; j++){
					if(pcb_list[j]->state == RUNNING){
						int t = temp_event.time - system_clock;
						cpu_time_reqd[pcb_list[j]->p_id] = t ;
						pcb_list[j]->state = READY;
						preempt_id  = pcb_list[j]->p_id;
						break;
					}

				}

				temp_event.p_id = pcb_list[i]->p_id;
				temp_event.time = system_clock + cpu_time_reqd[pcb_list[i]->p_id];
				report += "At time = ";
				stringstream ss1;
				ss1 << system_clock;
				string temp = ss1.str();
				report += temp;
				report += ", process with pid = ";
				stringstream ss2;
				ss2 << temp_event.p_id;
				string temp2 = ss2.str();
				report += temp2;
				report += " was scheduled and process with pid ";
				stringstream ss3;
				ss3 << preempt_id;
				string temp3 = ss3.str();
				report += temp3;
				report += " was pre-empted.\n";
				pcb_list[i]->state = RUNNING;
				cpu_idle = 0;
				break;

			}

		}
	}
}

void admissionhandlerMultiprogramming(event *  e){
	pcb * newpcb = new pcb();
	newpcb->p_id = e->p_id;
	newpcb->phase_id = 0;
	newpcb->state = READY;
	int num_of_process = process_list.size();
	for (int i = 0; i < num_of_process ;i++){
		if(e->p_id == process_list[i].p_id){
			newpcb->priority=process_list[i].start_priority;
			cpu_time_reqd.insert(pair<int,int>(newpcb->p_id,process_list[i].phases[0].cpu_time));
			break;
		}

	}
	pcb_list.push_back(newpcb);
	sort (pcb_list.begin(),pcb_list.end(),pcbComparator);
	scheduleProcessMultiprogramming();
}



void cpuoverhandlerMultiprogramming(){
	cpu_idle = 1;
	event * newevent = new event();
	newevent->type =  IO_OVER;
	newevent->p_id = temp_event.p_id;
	cpu_time_reqd[temp_event.p_id]=0;
	int num_of_pcb = pcb_list.size();
	for (int i = 0; i < num_of_pcb ;i++){
		if(pcb_list[i]->p_id == temp_event.p_id){
			pcb_list[i]->state = BLOCKED;
			int num_of_process = process_list.size();
			for (int j = 0; j < num_of_process ;j++){
				if(pcb_list[i]->p_id == process_list[j].p_id){
					newevent->time = system_clock + (process_list[j]).phases[pcb_list[i]->phase_id].io_time;
					event_table.push(newevent);
					break;
				}

			}
			break;	
		}
	}
	temp_event.time = 100000000;
	temp_event.p_id = -1;
	scheduleProcessMultiprogramming();
}

void iooverhandlerMultiprogramming(event *e){
	int num_of_pcb = pcb_list.size();
	for (int i = 0; i < num_of_pcb ;i++){
		if(pcb_list[i]->p_id == e->p_id){
			int num_of_process = process_list.size();
			for (int j = 0; j < num_of_process ;j++){
				if(pcb_list[i]->p_id == process_list[j].p_id){
					(process_list[j]).phases[pcb_list[i]->phase_id].iterations =  (process_list[j]).phases[pcb_list[i]->phase_id].iterations - 1;
					if((process_list[j]).phases[pcb_list[i]->phase_id].iterations == 0){
						pcb_list[i]->phase_id = pcb_list[i]->phase_id + 1;
					}
					if(pcb_list[i]->phase_id >= (process_list[j]).phases.size()){
						pcb_list[i]->state = TERMINATED;
					}
					else{
						pcb_list[i]->state = READY;
						cpu_time_reqd[pcb_list[i]->p_id] = process_list[j].phases[pcb_list[i]->phase_id].cpu_time;

					}
					break;
				}

			}
			break;	
		}
	}
	scheduleProcessMultiprogramming();
}

void multiprogramming(){
	
	while(!event_table.empty()){
		event_table.pop();
	}

	pcb_list.clear();
	process_list.clear();
	my_scheduler.levels.clear();
	my_scheduler.no_levels=0;
	cpu_time_reqd.clear();

	cpu_idle = 1;
	system_clock = 0;
	temp_event.time = 1000000;
	temp_event.p_id = -1;
	temp_event.type = CPU_OVER;
	process_proc_file();
	process_scheduler_file();
	initializeEventTable();
	while((!event_table.empty()) ||  (temp_event.p_id != -1)){
		if(event_table.empty()){
			if(temp_event.time < 1000000){
				system_clock = temp_event.time;
				cout<<"CPU Event, Time of event = "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
				cpuoverhandlerMultiprogramming();
			}
		}
		else{
			event * top_event = event_table.top();
			if(temp_event.time < top_event->time){
				system_clock = temp_event.time;
				cout<<"cpu "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
				cpuoverhandlerMultiprogramming();
				
			}
			else{
				system_clock = top_event->time;
				if(top_event->type == ADMISSION){
					cout<<"ad " <<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					admissionhandlerMultiprogramming(top_event);
					
				}
				else if(top_event->type == IO_OVER){
					cout<<"io "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					iooverhandlerMultiprogramming(top_event);
					
				}
				event_table.pop();
			}
		}
		
	}
}

void scheduleProcessTimeSharing(){
	if(!pcb_list_time_sharing.empty()){
		if(cpu_idle){
			pcb * front_pcb = pcb_list_time_sharing.front();
			pcb_list_time_sharing.pop();
			event * newevent = new event();
			if (cpu_time_reqd[front_pcb->p_id] <= time_slice){
				newevent->p_id = front_pcb->p_id;
				newevent->time = system_clock + cpu_time_reqd[front_pcb->p_id];
				report += "In time slice ";
				stringstream ss1;
				ss1 << system_clock;
				string temp = ss1.str();
				report += temp;
				report += " to ";
				stringstream ss2;
				ss2 << newevent->time;
				string temp1 = ss2.str();
				report += temp1;
				report += " process with pid = ";
				stringstream ss3;
				ss3 << newevent->p_id;
				string temp2 = ss3.str();
				report += temp2;
				report += " was scheduled on CPU.\n";
				newevent->type = CPU_OVER;
				event_table.push(newevent);
				cpu_idle = 0;
				running_pcb = front_pcb;
				running_pcb->state = RUNNING;
			}
			else{
				
				newevent->p_id = front_pcb->p_id;
				newevent->time = system_clock + time_slice;
				report += "In time slice ";
				stringstream ss1;
				ss1 << system_clock;
				string temp = ss1.str();
				report += temp;
				report += " to ";
				stringstream ss2;
				ss2 << newevent->time;
				string temp1 = ss2.str();
				report += temp1;
				report += " process with pid = ";
				stringstream ss3;
				ss3 << newevent->p_id;
				string temp2 = ss3.str();
				report += temp2;
				report += " was scheduled on CPU.\n";
				newevent->type = TS_OVER;
				event_table.push(newevent);
				cpu_idle = 0;
				running_pcb = front_pcb;
				running_pcb->state = RUNNING;
			}
		}
	}
}

void cpuoverhandlerTimeSharing(event *e){
	cpu_idle = 1;
	event *newevent = new event();
	newevent->p_id = e->p_id;
	newevent->type = IO_OVER;

	int num_of_process = process_list.size();
	for(int i = 0; i < num_of_process; i++){
		if(e->p_id == process_list[i].p_id){
			newevent->time = system_clock + process_list[i].phases[running_pcb->phase_id].io_time;
		}
	}

	running_pcb->state = BLOCKED;
	event_table.push(newevent);
	blocked_pcbs.insert(pair<int, pcb *> (newevent->p_id, running_pcb));
	scheduleProcessTimeSharing();
}

void iooverhandlerTimeSharing(event * e){
	pcb * curr_pcb = blocked_pcbs[e->p_id];

	int num_of_process = process_list.size();
	for (int j = 0; j < num_of_process ;j++){
		if(curr_pcb->p_id == process_list[j].p_id){
			(process_list[j]).phases[curr_pcb->phase_id].iterations =  (process_list[j]).phases[curr_pcb->phase_id].iterations - 1;
			if((process_list[j]).phases[curr_pcb->phase_id].iterations == 0){
				curr_pcb->phase_id = curr_pcb->phase_id + 1;
			}
			if(curr_pcb->phase_id >= (process_list[j]).phases.size()){
				curr_pcb->state = TERMINATED;
			}
			else{
				curr_pcb->state = READY;
				cpu_time_reqd[curr_pcb->p_id] = process_list[j].phases[curr_pcb->phase_id].cpu_time;
				pcb_list_time_sharing.push(curr_pcb);
			}
			break;	
		}
	}
	scheduleProcessTimeSharing();
}

void tsoverhandlerTimeSharing(event * e){
	running_pcb->state = READY;
	cpu_time_reqd[running_pcb->p_id] = cpu_time_reqd[running_pcb->p_id] - time_slice;
	pcb_list_time_sharing.push(running_pcb);
	cpu_idle = 1;
	scheduleProcessTimeSharing();
}

void admissionhandlerTimeSharing(event * e){
	pcb * newpcb = new pcb();
	newpcb->p_id = e->p_id;
	newpcb->phase_id = 0;
	newpcb->state = READY;
	newpcb->priority = 0;
	pcb_list_time_sharing.push(newpcb);
	int num_of_process = process_list.size();
	for (int i = 0; i < num_of_process ;i++){
		if(e->p_id == process_list[i].p_id){
			cpu_time_reqd.insert(pair<int,int>(newpcb->p_id, process_list[i].phases[0].cpu_time));
			break;
		}

	}
	scheduleProcessTimeSharing();
}

void timeSharing(){
	time_slice = 4;
	while(!event_table.empty()){
		event_table.pop();
	}
	while(!pcb_list_time_sharing.empty()){
		pcb_list_time_sharing.pop();
	}

	process_list.clear();
	my_scheduler.levels.clear();
	my_scheduler.no_levels=0;
	cpu_time_reqd.clear();

	cpu_idle = 1;
	system_clock = 0;
	process_proc_file();
	process_scheduler_file();
	initializeEventTable();

	while(!event_table.empty()){
		event * top_event = event_table.top();
		system_clock = top_event->time;
		if(top_event->type == CPU_OVER){
			cout<<"cpu "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
			cpuoverhandlerTimeSharing(top_event);
		}
		else if(top_event->type == TS_OVER){
			cout<<"ts "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
			tsoverhandlerTimeSharing(top_event);
		}
		else if(top_event->type == IO_OVER){
			cout<<"io "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
			iooverhandlerTimeSharing(top_event);
		}
		else if(top_event->type == ADMISSION){
			cout<<"ad "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
			admissionhandlerTimeSharing(top_event);
		}
		event_table.pop();
	}

}

void scheduleProcessMultilevel(){

	int num_of_levels = my_scheduler.levels.size();
	int maxLevel = 0;
	int maxPriority = 0;
	int maxLevelTimeSlice = 0;
	for(int i = 0; i < num_of_levels; i++){
		if ((!pcb_list_multilevel[my_scheduler.levels[i].level_number].empty()) && (my_scheduler.levels[i].priority > maxPriority)){
			maxLevel = i;
			maxPriority = my_scheduler.levels[i].priority;
			maxLevelTimeSlice = my_scheduler.levels[i].time_slice;
		}
	}
	if(cpu_idle){

		if (!pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].empty()){
			pcb * front_pcb = pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].front();
			pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].pop();
			temp_event.p_id = front_pcb->p_id;
			if(cpu_time_reqd[front_pcb->p_id] <= maxLevelTimeSlice){
				temp_event.time = system_clock + cpu_time_reqd[front_pcb->p_id];
				temp_event.type = CPU_OVER;
			}
			else{
				temp_event.time = system_clock + maxLevelTimeSlice;
				temp_event.type = TS_OVER;
			}
			cpu_idle = 0;
			running_pcb = front_pcb;
			running_pcb->state = RUNNING;
		}
	}
	else{

		if (!pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].empty()){
			pcb * front_pcb = pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].front();
			if(running_pcb->priority < front_pcb->priority){
				pcb_list_multilevel[my_scheduler.levels[maxLevel].level_number].pop();
				for(int  i = 0; i < num_of_levels; i++){
					if(my_scheduler.levels[i].priority == running_pcb->priority){
						if(temp_event.type == TS_OVER){
							cpu_time_reqd[running_pcb->p_id] = temp_event.time + my_scheduler.levels[i].time_slice - system_clock;
						}
						else if(temp_event.type == CPU_OVER){
							cpu_time_reqd[running_pcb->p_id] = temp_event.time - system_clock;
						}
					}
				}
				
				
				running_pcb->state = READY;
				for(int i = 0; i < num_of_levels; i++){
					if (my_scheduler.levels[i].priority == running_pcb->priority){
						pcb_list_multilevel[my_scheduler.levels[i].level_number].push(running_pcb);
					}
				}

				temp_event.p_id = front_pcb->p_id;
				if(cpu_time_reqd[front_pcb->p_id] <= maxLevelTimeSlice){
					temp_event.time = system_clock + cpu_time_reqd[front_pcb->p_id];
					temp_event.type = CPU_OVER;
				}
				else{
					temp_event.time = system_clock + maxLevelTimeSlice;
					temp_event.type = TS_OVER;
				}
				cpu_idle = 0;
				running_pcb = front_pcb;
				running_pcb->state = RUNNING;
			}
		}
	}
}

void cpuoverhandlerMultilevel(){
	event * newevent = new event();
	newevent->p_id = temp_event.p_id;
	newevent->type = IO_OVER;
	int num_of_process = process_list.size();
	for(int i = 0; i < num_of_process; i++){
		if(temp_event.p_id == process_list[i].p_id){
			newevent->time = system_clock + process_list[i].phases[running_pcb->phase_id].io_time;
			break;
		}
	}
	running_pcb->state = BLOCKED;
	int num_of_levels = my_scheduler.levels.size();
	int maxPriority = 0;
	for(int i = 0; i < num_of_levels; i++){
		if (my_scheduler.levels[i].priority > maxPriority){
			maxPriority = my_scheduler.levels[i].priority;
		}
	}
	int expected_priority = maxPriority;
	
	for(int i = 0; i < num_of_levels; i++){
		if((my_scheduler.levels[i].priority > running_pcb->priority) && (my_scheduler.levels[i].priority < expected_priority)){
			expected_priority = my_scheduler.levels[i].priority;
		}
	}
	if(running_pcb->priority < expected_priority){
		report += "At system clock = ";
		stringstream ss3;
		ss3 << system_clock;
		string temp = ss3.str();
		report += temp;
		report += " process with pid = ";
		stringstream ss2;
		ss2 << running_pcb->p_id;
		string temp2 = ss2.str();
		report += temp2;
		report += " was promoted to priority ";
		stringstream ss1;
		ss1 << expected_priority;
		string temp3 = ss1.str();
		report += temp3;
		report += "\n";
	}
	running_pcb->priority = expected_priority;
	blocked_pcbs.insert(pair<int, pcb*> (newevent->p_id, running_pcb));
	cpu_idle = 1;
	event_table.push(newevent);
	temp_event.time = 1000000;
	temp_event.p_id = -1;
	scheduleProcessMultilevel();
}

void tsoverhandlerMultilevel(){
	running_pcb->state = READY;
	int expected_priority = 0;
	int num_of_levels = my_scheduler.levels.size();

	for(int i = 0; i < num_of_levels; i++){
		if((my_scheduler.levels[i].priority < running_pcb->priority) && (my_scheduler.levels[i].priority > expected_priority)){
			expected_priority = my_scheduler.levels[i].priority;
		}
		if(my_scheduler.levels[i].priority == running_pcb->priority){
			cpu_time_reqd[running_pcb->p_id] = cpu_time_reqd[running_pcb->p_id] - my_scheduler.levels[i].time_slice;
		}
	}
	if((running_pcb->priority > expected_priority) && (expected_priority != 0)){
		report += "At system clock = ";
		stringstream ss1;
		ss1 << system_clock;
		string temp = ss1.str();
		report += temp;
		report += " process with pid = ";
		stringstream ss2;
		ss2 << running_pcb->p_id;
		string temp2 = ss2.str();
		report += temp2;
		report += " was demoted to priority ";
		stringstream ss3;
		ss3 << expected_priority;
		string temp3 = ss3.str();
		report += temp3;
		report += "\n";
	}

	if(expected_priority != 0){
		running_pcb->priority = expected_priority;
	}

	for(int i = 0; i < num_of_levels; i++){
		if(my_scheduler.levels[i].priority == running_pcb->priority){
			//cout << "ts queue size = " << pcb_list_multilevel[my_scheduler.levels[i].level_number].size() << endl;
			pcb_list_multilevel[my_scheduler.levels[i].level_number].push(running_pcb);
			//cout << "ts queue size = " << pcb_list_multilevel[my_scheduler.levels[i].level_number].size() << endl;
			break;
		}
	}

	cpu_idle = 1;
	temp_event.time = 1000000;
	temp_event.p_id = -1;
	scheduleProcessMultilevel();
}

void iooverhandlerMultilevel(event * e){
	pcb * curr_pcb = blocked_pcbs[e->p_id];

	int num_of_process = process_list.size();
	for (int j = 0; j < num_of_process ;j++){
		if(curr_pcb->p_id == process_list[j].p_id){
			(process_list[j]).phases[curr_pcb->phase_id].iterations =  (process_list[j]).phases[curr_pcb->phase_id].iterations - 1;
			if((process_list[j]).phases[curr_pcb->phase_id].iterations == 0){
				curr_pcb->phase_id = curr_pcb->phase_id + 1;
			}
			if(curr_pcb->phase_id >= (process_list[j]).phases.size()){
				curr_pcb->state = TERMINATED;
			}
			else{
				curr_pcb->state = READY;
				cpu_time_reqd[curr_pcb->p_id] = process_list[j].phases[curr_pcb->phase_id].cpu_time;
				int num_of_levels = my_scheduler.levels.size();
				for(int i = 0; i < num_of_levels; i++){
					if(my_scheduler.levels[i].priority == curr_pcb->priority){
						//cout << "push ho ja re "<<curr_pcb->priority<<" id "<<curr_pcb->p_id<<endl;
						//cout << "level_number " << my_scheduler.levels[i].level_number << endl;
						pcb_list_multilevel[my_scheduler.levels[i].level_number].push(curr_pcb);
						//cout << "queue size = " << pcb_list_multilevel[my_scheduler.levels[i].level_number].size() << endl;
						break;
					}
				}
				
			}
			break;	
		}
	}
	scheduleProcessMultilevel();
}

void admissionhandlerMultilevel(event * e){
	pcb * newpcb = new pcb();
	newpcb->p_id = e->p_id;
	newpcb->phase_id = 0;
	newpcb->state = READY;

	int num_of_process = process_list.size();
	for(int i = 0; i < num_of_process; i++){
		if(process_list[i].p_id == newpcb->p_id){
			newpcb->priority = process_list[i].start_priority;
			cpu_time_reqd.insert(pair<int, int> (newpcb->p_id, process_list[i].phases[0].cpu_time));
			break;
		}
	}
	int num_of_levels = my_scheduler.levels.size();
	for(int i = 0; i < num_of_levels; i++){
		if(my_scheduler.levels[i].priority == newpcb->priority){

			if(pcb_list_multilevel.find(my_scheduler.levels[i].level_number) != pcb_list_multilevel.end()){
				pcb_list_multilevel[my_scheduler.levels[i].level_number].push(newpcb);
				
			}
			else{

				queue<pcb *> newqueue;
				newqueue.push(newpcb);
				pcb_list_multilevel.insert(pair<int, queue<pcb *> > (my_scheduler.levels[i].level_number, newqueue));
			}
		}
	}

	scheduleProcessMultilevel();
}

void multilevel(){
	while(!event_table.empty()){
		event_table.pop();
	}

	process_list.clear();
	my_scheduler.levels.clear();
	my_scheduler.no_levels=0;
	cpu_time_reqd.clear();

	cpu_idle = 1;
	system_clock = 0;
	temp_event.time = 1000000;
	temp_event.p_id = -1;
	temp_event.type = CPU_OVER;
	process_proc_file();
	process_scheduler_file();
	initializeEventTable();

	while((!event_table.empty()) || (temp_event.p_id != -1)){
		if(event_table.empty()){
			if(temp_event.time < 1000000){
				system_clock = temp_event.time;
				if(temp_event.type == CPU_OVER){
					cout<<"cpu "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
					cpuoverhandlerMultilevel();
				}
				else if(temp_event.type == TS_OVER) {
					cout<<"ts "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
					tsoverhandlerMultilevel();
				}
			}
		}
		else{
			event * top_event = event_table.top();
			if(temp_event.time < top_event->time){
				system_clock = temp_event.time;
				if(temp_event.type == CPU_OVER){
					cout<<"cpu "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
					cpuoverhandlerMultilevel();
				}
				else if(temp_event.type == TS_OVER){
					cout<<"ts "<<temp_event.time <<" "<<temp_event.type<<" "<< temp_event.p_id << endl;
					tsoverhandlerMultilevel();
				}
			}
			else{
				system_clock = top_event->time;
				if(top_event->type == CPU_OVER){
					cout<<"cpu "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					cpuoverhandlerMultilevel();
				}
				else if(top_event->type == TS_OVER){
					cout<<"ts "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					tsoverhandlerMultilevel();
				}
				else if(top_event->type == IO_OVER){
					cout<<"io "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					iooverhandlerMultilevel(top_event);
				}
				else if(top_event->type == ADMISSION){
					cout<<"ad "<<top_event->time <<" "<<top_event->type<<" "<< top_event->p_id << endl;
					admissionhandlerMultilevel(top_event);
				}
				event_table.pop();
			}
		}
	}

}

int main() {
	report = "";
	int scheduler = 0;
	cout << "Select a scheduler = \n";
	cout << "\t1 : Multiprogramming Scheduler \n";
	cout << "\t2 : Time-sharing Scheduler \n";
	cout << "\t3 : Multi-level Scheduler \n";
	cin >> scheduler;
	if(scheduler == 1){
		multiprogramming();
	}
	else if(scheduler == 2){
		timeSharing();
	}
	else if(scheduler == 3){
		multilevel();
	}
	else{
		cout << "Incorrect Input. Terminating...";
	}
	cout << report;
	return 0;
}