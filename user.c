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


/*
	unsigned int old_time = clock_get_time();
	while(1) {
		if (old_time != clock_get_time()) {
			old_time = clock_get_time();
			printint(old_time);
			printc('\n'); printc(13);
		}
	}
*/


int __attribute__ ((__section__(".text.main"))) main(void) {
	char aux = 'a';
	pid = fork();
	//clone(cloneHello, &stack[1024]);
	//debug_task_switch();
	if (pid == 0) { // fill
		buff[0] = 'f';
		//debug_task_switch();

	}
	else {
		buff[0] = 'p';
		//debug_task_switch();
	}
	//while(1);

	while(1) {

		//read(0,&aux,1);
		write(1,&buff[0],1);
		//debug_task_switch();
	}

	/*

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
	 */

	while(1) { }

	return 0;
}
