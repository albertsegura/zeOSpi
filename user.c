#include <libc.h>
#include <perror.h>


char buff[24] = {"Hola desde USER!"};

int pid=0;
char stack[1024];

void cloneHello() {
	//char pidc[11];
	//pid = getpid();
	/*write(1,"Surto del clone!\n",17);
	int ret = read(0,pidc,5);
	write(1,"Lectura: ",9);
	write(1,pidc,5);
	itoa(ret,pidc);
	write(1," Size: ",7);
	write(1,pidc,strlen(pidc));
	write(1,"\n",1);*/
	while(1);
}

int __attribute__ ((__section__(".text.main"))) main(void) {
	//pid = fork();
	clone(cloneHello, &stack[1024]);
	debug_task_switch();
	if (pid == 0) { // fill

	}
	else {
		debug_task_switch();
	}
	while(1);


	unsigned int old_time, tmp;
	write (1, buff, strlen(buff));

	old_time = gettime();
	while(1) {
		tmp = gettime();
		if (old_time != tmp) {
			old_time = tmp;

			itoa(old_time,buff);
			write(1,buff,strlen(buff));
			write(1,"\n",1);
		}
	}

	while(1) { }


	return 0;
}
