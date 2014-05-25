#include <libc.h>
#include <perror.h>


char buff[24] = {"Hola desde USER!"};

int pid;

int __attribute__ ((__section__(".text.main"))) main(void) {


	pid = fork();
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
