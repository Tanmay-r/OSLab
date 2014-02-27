110050010	Tanmay Randhavane
110050043	Alok Yadav

Assignment 6

1)	The code that performs boot actions-
->	The file guestos.c contains the main function.
->	The main function after initializing options, calls a boot function which has following definition,

	void boot(void) {
		// to install system calls
		install_signals();
		set_defaults();
	}

-->	The first function install_signals() installs the necessary signals and specifies their handler function.

	void install_signals(void){
		signal(SIGINT, &sim_signal_handler);
		signal(SIGABRT, &sim_signal_handler);
		signal(SIGFPE, &sim_signal_handler);
		signal(SIGUSR2, &sim_signal_handler);
	}

--> After install_signals(), set_defaults() function is called. This call sets up a virtual disk by using the configuration parameters concerning number of disk blocks and thus booting process is completed.


	void set_defaults(void) {
		char param_value[LINE_MAX+1];
		get_param("INSTR_SLICE", param_value);
		instr_slice = atoi(param_value);

		int heads, tracks, sectors;
		char command[1000];
		get_param("NUM_HEADS",param_value);
		heads=atoi(param_value);
		get_param("NUM_TRACKS",param_value);
		tracks=atoi(param_value);
		get_param("NUM_SECTORS",param_value);
		sectors=atoi(param_value);
		sprintf(command,"(dd if=/dev/zero of=Sim_disk bs=%dx%dx%db count=1) 2> /dev/zero",heads,tracks,sectors);
		system(command);
	}

2)
	In the .config file, we can change the following parameters:
	INSTR_SLICE=10
	NUM_HEADS=2
	NUM_TRACKS=128
	NUM_SECTORS=32

	Initially sym_disk file size was 1.5 MB and after the changes it became 4.5 MB.

3)
	a)	How does a system call handler in the Guest OS get invoked?
		->	A function system_do is called and according to the syscode of the system call, (if it is greater than 325), handle_guest_syscalls() function is called.
	b)	How is a system call passes on to the Host OS?
		All the system calls are listed in the file syscall.dat file, total 327 system calls are present in the guest OS.
	c)	How is a system call passes on to the Host OS?
		->	When syscall_do function is called in the Guest OS, according to the syscode, decision is taken whether to pass the syscall to the Host OS or to take the necessary action in the Guest OS itself.
		->	In case of syscode > 325 (currently there exists only one syscall(get_pid) with syscode > 325) the Guest OS calls a function handle_guest_syscalls() is called. There is no need to pass the syscall to the Host OS in this case.
		->	In case of syscode < 325, then according to the type of the syscall, action is taken.
		->	For example, in case of exit syscall, a function ctx_finish is called, which finishes the context of a process.
		->	Thus, passing the control to the host OS, is done inside t
	d)	How does a system call handler access some part of the address space of a process?
		->	In case of a system call which requires the access to some part of the address space of a process, the parameters in ebx, ecx, edx, esi, edi, ebp. The parameter in ebx contains guest file descriptor, which is passed to the function fdt_entry_get which takes the file descriptor and returns a host file descriptor which then is used to access the address space of the program.

4)
	a)	
	PCB of a process is stored in ctx_t struct along with other process information. PCBS are linked to each other through pointer forming a linked list. When a pocess is created a create_ctx function is called which created  context for the process and set the parent for newly created  ctx(PCBs). We can get list of PCBs by accessing  pointer to head stored in named context_list_head and tail pointer named context_list_tail, both are field of m2skernel.
	b)
	"instr_slice" (defined in m2skernel.h) is the field that contains the instruction-slice of a process.
	3)
	Guest OS scheduler executes instruction one by one  from each process in robin-round way.when execution of instructions of a process completed, kernel iterate over context list to get instructions of next process in list.


5)	

