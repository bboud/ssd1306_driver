#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <ctype.h>

#include "font.h"

static int device;
static const char* interface = "/dev/i2c-1";
static const int addr = 0x3c;
static bool initialized = false;
static const unsigned char initSeq[] = { 0xae, 0xa8, 0x35, 0xd3, 0x00, 0x40, 0xa1, 0xc0, 0xda, 0x12, 0x81, 0x7f, 0xa4, 0xa6, 0xd5, 0x00, 0x8d, 0x14, 0x20, 0x00, 0xaf};
static unsigned char frameBuffer[8][128] = {0};

//This error handling should be better handled later..
extern int errno;

static void error_handle(){
	fprintf(stderr, "Error: %s\n", strerror(errno));
}

int i2c_write(unsigned char *msg, int nmsg, bool command){

	if(!initialized){
		return -1;
	}

	device = open(interface, O_RDWR);

	if(device < 1){
		error_handle();
		return -1;
	}

	if( ioctl(device, I2C_SLAVE, addr) < 0){
		error_handle();
	}

	unsigned char buffer[nmsg+1];
	if(command){
		buffer[0] = 0x00;
	}else{
		buffer[0] = 0x40;
	}

	for(int i = 0; i < nmsg; i++){
		buffer[1 + i] = msg[i];
	}


	if(write(device, buffer, nmsg+1) != nmsg+1){
		error_handle();
	}

	if(close(device) < 0){
		error_handle();
	}

	return 0;
}

void initialize(){
	initialized = true;

	i2c_write((unsigned char*)initSeq, sizeof(initSeq), true);
}

void displayOff(){
	unsigned char message[] = {0xae};
	i2c_write(message, 1, true);
}

void displayOn(){
	unsigned char message[] = {0xaf};
	i2c_write(message, 1, true);
}

void clearDisplay(){
	for(int page=0; page<8; page++){
		unsigned char pap[] = {page+0xb0};
		i2c_write(pap, 1, true);

		unsigned char clear[128] = {0};
		i2c_write(clear, 128, false);
	}
}

void writeChar(char c, int page, int x){
	c = toupper(c);

	//Clamp the col offset from char width
	if(x > 123){
		x = 123;
	}else if(x < 0){
		x = 0;
	};

	if(page > 7 || page < 0){
		printf("Page out of bound.. Writing to page 1\n");
		page = 1;
	}

	
	//Look up the character on the table
	//Loop through the 5 columns of the table
	
	for(int i = 0; i<5; i++){
		frameBuffer[page][x+i] = characters[c-32][i];
	}
}

void writeLine(char *line, int page, int offset){
	for(int i=0; line[i] != '\0'; i++){
		writeChar(line[i], page, offset+i*6);
	}
}

void updateDisplay(){
	unsigned char fullRange[] = { 0x21, 0x00, 0x7f, 0x22, 0x00, 0x07 };
	i2c_write(fullRange, 6, true);

	for(int page = 0; page < 8; page++){
		i2c_write(frameBuffer[page], 128, false);
	}
}


void dumpFrameBuffer(){
	for(int i = 0; i < 8; i++){
		printf("PAGE %d\n", i);
		for(int j = 0; j < 128; j++){
			printf("%d", frameBuffer[i][j]);
		}
		printf("\n");
	}
}

int main(){

	initialize();

	clearDisplay();

	for(int i = 0; i < 8; i++){
                writeLine("PAGE", i, 0);
        }

	updateDisplay();

	dumpFrameBuffer();

	return 0;
}
