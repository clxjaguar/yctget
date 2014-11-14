/*
   Name:            serial.cpp
   Author:          cLx
   Description:     Pour utiliser les ports séries sous nux et windows
   Date:            08/06/08 - premiere ébauche version windows
					14/09/09 - on continue le boulot sous linux...
					27/10/14 - ajout de nouvelles fonctions et mise au propre
   Copyright:       cLx <clx.kat@gmail.com> 
*/

#ifdef WIN32
// Window$
#include <windows.h>
#define DEFAULTPORT "COM1"
#define FDTYPE HANDLE
#else
// Linux
#include <iostream>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <limits.h>
#define DEFAULTPORT "/dev/ttyUSB0"
#endif

#include "serial.h"

#ifdef WIN32
int serial::open(const char *portname, unsigned int baud, int bits=8, char parity='N', int stopbits=1){
	close();
	fd = CreateFile(portname?portname:DEFAULTPORT, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING, NULL);

	if(fd == INVALID_HANDLE_VALUE){
		opened = 0;
		return 0;
	}
	else { opened = 1; }

	// clear buffers
	PurgeComm(fd,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

	myDCB.DCBlength = sizeof(DCB);
	// get the actual port's configuration
	GetCommState(fd, &myDCB);
	myDCB.BaudRate=baud;
	myDCB.ByteSize=bits;

	switch(parity){
		case 'o':
		case 'O':
			myDCB.Parity=ODDPARITY;
			break;
		case 'e':
		case 'E':
			myDCB.Parity=EVENPARITY;
			break;
		case 'm':
		case 'M':
			myDCB.Parity=MARKPARITY;
			break;
		case 'n':
		case 'N':
		default:
			myDCB.Parity=NOPARITY;
			break;
	}
	switch(stopbits){
		case 15:
			myDCB.StopBits=ONE5STOPBITS;
			break;
		case 2:
			myDCB.StopBits=TWOSTOPBITS;
			break;
		case 1:
		default:
			myDCB.StopBits=ONESTOPBIT;
			break;
	}

	myDCB.fDtrControl=DTR_CONTROL_DISABLE;

	if(!SetCommState(fd,&myDCB)){
		return 0;
	}

	return 1;
}

void serial::close(void) {
	if (opened){
		CloseHandle(fd);
		opened = 0;
		fd = NULL;
	}
}

int serial::send(char car){
	DWORD NumBytes=0;
	char str[2] = {car, '\0'};
	if (!opened){ return 0; }
	return WriteFile(fd, str, 1, &NumBytes, NULL);
}

int serial::send(char* str){
	DWORD NumBytes=0;
	if (!opened){ return 0; }
	return WriteFile(fd, str, strlen(str), &NumBytes, NULL);
}

int serial::send(char* str, size_t len){
	DWORD NumBytes=0;
	if (!opened){ return 0; }
	return WriteFile(fd, str, len, &NumBytes, NULL);
}

unsigned int serial::nbqueue(void){
	COMSTAT Stat;
	DWORD err;
	ClearCommError(fd, &err, &Stat);
	return Stat.cbInQue;
}

void serial::rts(int sw){
	EscapeCommFunction(fd, sw?SETRTS:CLRRTS);
}

void serial::dtr(int sw){
	EscapeCommFunction(fd, sw?SETDTR:CLRDTR);
}

void serial::brk(int sw){
	EscapeCommFunction(fd, sw?SETBREAK:CLRBREAK);
}


unsigned int serial::receive(char *buf, size_t buf_size){
	unsigned int toread = nbqueue();
	DWORD nbreaded;
	if (!toread) { return 0; }
	if (buf_size<toread) { toread = buf_size; }
	if(ReadFile(fd, buf, toread-1, &nbreaded, NULL)==0){
		return 0; // awww
	}
	buf[nbreaded] = '\0';
	return nbreaded;
}

unsigned int serial::binreceive(unsigned char *buf, size_t buf_size){
	unsigned int toread = nbqueue();
	DWORD nbreaded;
	if (!toread) { return 0; }
	if (buf_size<toread) { toread = buf_size; }
	if(ReadFile(fd, buf, toread, &nbreaded, NULL)==0){
		return 0;
	}
	return nbreaded;
}

#else
// VERSION LINUX ICI !

int serial::open(const char *portname, unsigned int baud, int bits=8, char parity='N', int stopbits=1){
	struct termios options;
	speed_t spd;

	fd = ::open(portname, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if((fd == -1)||(!fd)) {
		fd = 0;
		return 0;
	}
	opened = 1;
	
	if(tcgetattr(fd, &options)<0){perror("tcgetattr()");}
	cfmakeraw(&options);
	switch(baud){
		case     50: spd =     B50; break;
		case     75: spd =     B75; break;
		case    110: spd =    B110; break;
		case    134: spd =    B134; break;
		case    150: spd =    B150; break;
		case    200: spd =    B200; break;
		case    300: spd =    B300; break;
		case    600: spd =    B600; break;
		case   1200: spd =   B1200; break;
		case   1800: spd =   B1800; break;
		case   2400: spd =   B2400; break;
		case   4800: spd =   B4800; break;
		case   9600: spd =   B9600; break;
		case  19200: spd =  B19200; break;
		case  38400: spd =  B38400; break;
		case  57600: spd =  B57600; break;
		case 115200: spd = B115200; break;
#ifdef B230400
		case 230400: spd = B230400; break;
#endif
#ifdef B460800
		case 460800: spd = B460800; break;
#endif
#ifdef B921600
		case 921600: spd = B921600; break;
#endif
		default:
			return 0;
			break;
	}

	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_iflag = 0;
	options.c_cflag = 0;
	
	if (cfsetospeed(&options, spd)<0){ perror("cfsetospeed()"); }
	if (cfsetispeed(&options, spd)<0){ perror("cfsetispeed()"); }

	
	options.c_oflag &= ~OPOST; // raw mode for output
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw mode for input

	options.c_iflag &= ~(BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
	options.c_iflag |= IGNBRK; // ignore breaks
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // no software flow control

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~(CSIZE | PARENB);
	options.c_cflag |= CS8;

	if (stopbits==1) {
		options.c_cflag &= ~CSTOPB;
	}
	else {
		options.c_cflag |= CSTOPB;
	}
	options.c_cflag &= ~CSIZE; // mask the character size bits
	options.c_cflag |= CS8; // 8 bits

	if (parity=='E'){ // parity even
		options.c_cflag |= PARENB;
	}
	else if (parity=='O'){ // parity odd
		options.c_cflag |= (PARENB | PARODD);
	}
	else { // no parity
		if (parity!='N') { fprintf(stderr, "Parity should be E, O or N (defaut)\n"); }
		options.c_cflag &= (~PARENB & ~PARODD);
	}


	if(tcsetattr(fd, TCSANOW, &options)){perror("tcsetattr()");};
	if(tcsetattr(fd, TCSAFLUSH, &options)){perror("tcsetattr()");};
	clear_buffer();
	return 0;
}

void serial::close(void) {
	if (opened){
		::close(fd);
		opened = 0;
		fd = 0;
	}
}

int serial::send(char car){
	char str[2] = {car, '\0'};
	if (!opened){ return 0; }
	return write(fd, str, 1);
}

int serial::send(char* str){
	if (!opened){ return 0; }
	return write(fd, str, strlen(str));
}

int serial::send(char* str, size_t len){
	if (!opened){ return 0; }
	return write(fd, str, len);
}

/* TODO: Adapting that code for linux ! */

unsigned int serial::nbqueue(void){
	fprintf(stderr, "nbqueue() is not yet implemented for Linux !\n");
	return 0;
}

unsigned int serial::receive(char *buf, size_t buf_size){
	signed int nbreaded;
	if (buf_size>SSIZE_MAX) { buf_size = SSIZE_MAX; }
	if (!opened){ return 0; }

	nbreaded = read(fd, buf, buf_size-1);
	if (nbreaded<1) {
		return 0;
	}
	buf[nbreaded] = '\0';
	return nbreaded;
}

unsigned int serial::binreceive(unsigned char *buf, size_t buf_size){
	signed int nbreaded;
	if (!opened){ return 0; }

	nbreaded = read(fd, buf, buf_size);
	if (nbreaded<1) { return 0; }
	return nbreaded;
}

//TODO: The following three functions are untested yet!

void serial::rts(int sw){
	//int ioctl( fd, COMMAND, (int *)flags );
	ioctl(fd, TIOCM_RTS, sw);
}
void serial::dtr(int sw){
	ioctl(fd, TIOCM_DTR, sw);
}
void serial::brk(int sw){
	ioctl(fd, TCSBRK, sw);
	//int tcsendbreak(fd, sw); // POSIX, not act exactly like ioctl (delays)
}

#endif
/// et ensuite tout ce qui est commun aux deux !

int serial::isopened(void){
	return opened;
}

serial::~serial(){
	close();
}

serial::serial(){
	opened = 0;
	fd = 0; //NULL;
}

serial::serial(const char *portname, unsigned int baud=9600){
	serial();
	if (portname) { open(portname, baud, 8, 'N', 1); }
}

int serial::open(const char *portname, unsigned int baud=9600){
	if (portname) { open(portname, baud, 8, 'N', 1); }
	return opened;
}

void serial::clear_buffer(void){
	char fakebuf[512];
	if (!opened) { return; }
	while(receive(fakebuf, sizeof(fakebuf)));
}
