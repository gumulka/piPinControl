#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

#define PIN_OUTPUT 5
#define BUFFER_SIZE 16

#define STATE_FILE "/home/pi/light_state"
#define LOG_FILE "/home/pi/light_log"

const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

int main(int argc, char** argv) {
	wiringPiSetupSys();
	if(wiringPiSPISetup (1,500000)==-1) {
		printf("ERROR on initialising SPI");
		return EXIT_FAILURE;
	}

	size_t bytes_read = 0;
	unsigned char buffer[BUFFER_SIZE];
	unsigned char rbuffer[BUFFER_SIZE];
	FILE *file_ptr;


	file_ptr = fopen(STATE_FILE,"rb");
	if (!file_ptr) {
		printf("Unable to open file!\n");
		return EXIT_FAILURE;
	}
	/* Read in 256 8-bit numbers into the buffer */
	bytes_read = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, file_ptr);
	fclose(file_ptr);

	int before = buffer[0];

	int i = 0;
	if(argc==3) {
		int a = 0;
		int on = 0;
		if(!strcmp("ON",argv[2])) {
			a = atoi(argv[1]);
			on = 1;
		} else
		if(!strcmp("OFF",argv[2])) {
			a = atoi(argv[1]);
		} else {
			printf("Usage: %s PIN [ON|OFF]\n",argv[0]);
			return EXIT_FAILURE;
		}

		while(a>=8) {
			a -= 8;
			i++;
		}
		if(on) {
			buffer[i] |= (1<<a);
		} else {
			buffer[i] &= ~(1<<a);
		}

		file_ptr = fopen(STATE_FILE,"wb");
		if (!file_ptr) {
			printf("Unable to open file for write!\n");
			return EXIT_FAILURE;
		}
		fwrite(buffer,1,bytes_read,file_ptr);
		fclose(file_ptr);

	}
	bytes_read = MAX(i+1,bytes_read);
	for(i=0;i<bytes_read;i++) {
		rbuffer[i] = buffer[bytes_read-i-1];
		printf("byte value %2d: %s%s\n",i,bit_rep[buffer[0]>>4], bit_rep[buffer[0] & 0x0F]);
	}

	/* Write the data */
	wiringPiSPIDataRW(1, rbuffer,bytes_read);

	/* shift the inside registers to the outside */
	digitalWrite(PIN_OUTPUT,HIGH);
	usleep(500);
	digitalWrite(PIN_OUTPUT,LOW);

	/* write changes to logfile */
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime(&rawtime);

	file_ptr = fopen(LOG_FILE,"a");
	if (!file_ptr) {
		printf("Unable to open log file!\n");
		return EXIT_FAILURE;
	}

	char logbuffer[100];
	strftime(logbuffer,100,"%d.%m.%Y %H:%M:%S",timeinfo);
	fprintf(file_ptr,"%s | %s%s | %s%s\n",logbuffer,bit_rep[before>>4], bit_rep[before & 0x0F],bit_rep[buffer[0]>>4], bit_rep[buffer[0] & 0x0F]);

	fclose(file_ptr);

	return EXIT_SUCCESS;
}
