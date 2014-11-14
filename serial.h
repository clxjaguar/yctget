/*
   Name:            serial.h
   Author:          cLx - http://clx.freeshell.org
   Description:     A little lib to access to serial ports the same way in linux and windows
   Date:            08/06/08 (first windows version, wip)
                    14/09/09 (now the linux adaptation)
   Copyright:       cLx / RJS <clx.kat@gmail.com>
*/

#ifdef WIN32
// windows
#include <windows.h>
#define FDTYPE HANDLE
#else
// linux
#define FDTYPE int
#define DCB int
#endif

class serial {
	public:
		serial(void);
		serial(const char*, unsigned int);
		~serial();
		int open(const char*, unsigned int, int, char, int);
		int open(const char*, unsigned int);
		int isopened(void);
		void close(void);
		int send(char);
		int send(char*);
		int send(char*, size_t);
		unsigned int nbqueue(void);
		char getc(void);
		unsigned int receive(char*, size_t);
		unsigned int binreceive(unsigned char*, size_t);
		void clear_buffer();
		void rts(int);
		void dtr(int);
		void brk(int);

	private:
		FDTYPE fd;
		DCB myDCB;
		int opened;
};

