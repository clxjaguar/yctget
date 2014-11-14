/*
  Name:        YCTget
  Copyright:   GPL v3
  Author:      http://clx.freeshell.org/
  Date:        27/10/14
  Description: We got a YC-727D datalogger, easy to use with a convenent RS232
               PC link, but the provided VB6 software is really too much crappy
			   to be used (in fact, we never managed to save a single data file)
			   and we did some reverse engineering to use that device "normally".
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "serial.h"

#ifdef WIN32
#include <windows.h>
#define delay_ms(t) Sleep(t);
#else
#include <unistd.h>
#define delay_ms(t) usleep(1000*t);
#endif

unsigned int receive(serial &port, unsigned char *buf, size_t bufsize){
	unsigned int tries = 100;
	unsigned int n;
	n = port.binreceive(buf, bufsize);

	while(n<bufsize){
		delay_ms(10);
		n+= port.binreceive(buf+n, bufsize-n);
		tries--;
		if (!tries) { fprintf(stderr, "Warning: Timeout reading serial port...\n"); break; }
	}
	return n;
}

void hexdump(FILE *fd, unsigned char *buf, unsigned int n){
	unsigned int i;
	for (i=0; i<n; i++){
		fprintf(fd, "%02X", buf[i]);
	}
}

time_t import_hex_timedate(unsigned char yy, unsigned char mo, unsigned char dd, unsigned char hh, unsigned char mm, unsigned char ss){
	struct tm timestruct;
	time_t result;
	memset(&timestruct, 0, sizeof(timestruct));
	timestruct.tm_year = (((yy>>4)) & 0x0F)*10 + (yy & 0x0F) + ((yy<0x70)?100:0);
	timestruct.tm_mon  = ((((mo>>4)) & 0x0F)*10 + (mo & 0x0F)) - 1;
	timestruct.tm_mday = (((dd>>4)) & 0x0F)*10 + (dd & 0x0F);
	timestruct.tm_hour = (((hh>>4)) & 0x0F)*10 + (hh & 0x0F);
	timestruct.tm_min  = (((mm>>4)) & 0x0F)*10 + (mm & 0x0F);
	timestruct.tm_sec  = (((ss>>4)) & 0x0F)*10 + (ss & 0x0F);
	result = mktime(&timestruct);
	if (result<0) { fprintf(stderr, "Warning: Incorrect date/time : %02x-%02x-%02x %02x:%02x:%02x !\n", yy, mo, dd, hh, mm, ss); }
	return result;
}

int main(int argc, char *argv[]) {
	serial port1;
	unsigned char buf[24];
	unsigned int n, i, j;
	double value;
	FILE *fdout = NULL;
	
	fprintf(stderr, "YCTget (compilation date: %s)\n", __DATE__);

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <serial interface>\n\n", argv[0]);
		return -1;
	}

	port1.open(argv[1], 9600);
	if (!port1.isopened()) {
		fprintf(stderr, "Unable to open RS232 interface - ");
		perror(argv[1]);
		return -2;
	}

	for(i=0; i<3; i++){
		if (i) { fprintf(stderr, "Retrying...\n"); }
		port1.clear_buffer();
		port1.send(0xAA);
		delay_ms(5);
		port1.send(0xC5);
		port1.send(0x10);
		port1.send(0xAB);

		n = receive(port1, buf, 1);
		if (n) { break; }
		fprintf(stderr, "Error: Data link not detected.\n");
	}
	if (!n) { return -1; }
	if (buf[0] != 0xAA) {
		fprintf(stderr, "Error: Bad 0xAA header !\n");
		return -1;
	}

	unsigned int time_interval, mesurements_nbr, channels;
	time_t start_time, end_time, tmp_time;

	char strftimebuf[31];
	for(;;){
		n = receive(port1, buf, 12);
		if (buf[0]==0xAB && buf[1]==0xAB && buf[2] ==0xAB &&buf[3]==0xAB &&
		    buf[4]==0xAB && buf[5]==0xAB && buf[6] ==0xAB &&buf[7]==0xAB &&
			buf[8]==0xAB && buf[9]==0xAB && buf[10]==0xAB &&buf[11]==0xAB){
			fprintf(stderr, "End of Transmission !\n");
			break;
		}
		n+= receive(port1, buf+n, 12);
		if (n != 24) {
			fprintf(stderr, "Header incomplete... Exiting...\n");
			hexdump(stderr, buf, n); fprintf(stderr, "\n");
			return -1;
		}
		//buf[0]      & 0x0F // thermocouple 1(?) type
		//(buf[0]>>4) & 0x0F // thermocouple 2(?) type
		//buf[1]      & 0x0F // thermocouple 3(?) type
		//(buf[1]>>4) & 0x0F // thermocouple 4(?) type
		//buf[2] != 0x56 // unknow value.
		channels = ((buf[2]&0x30)>>4)+1;
		time_interval = ((buf[4]>>4)&0x0F)*600 + (buf[4]&0x0F)*60 + ((buf[3]>>4)&0x0F)*10 + (buf[3]&0x0F);
		mesurements_nbr = buf[15]*256 + buf[14];
		start_time = import_hex_timedate(buf[13], buf[12], buf[11], buf[10], buf[9],  buf[8]);
		end_time   = import_hex_timedate(buf[21], buf[20], buf[19], buf[18], buf[17], buf[16]);

		fprintf(stderr, "Channels:   %u\n", channels);
		fprintf(stderr, "Interval:   %us\n", time_interval);
		fprintf(stderr, "Start time: %s", ctime(&start_time));
		fprintf(stderr, "End time:   %s", ctime(&end_time));

		if (channels>4 || time_interval<1 || start_time<1 || end_time<1 || mesurements_nbr<1){
			fprintf(stderr, "Woo. We got some very strange parameters and we prefer to stop before it gets weirder.\n");
			return -1;
		}

		strftime(strftimebuf, sizeof(strftimebuf), "%Y%m%d-%H%M%S.csv", localtime(&start_time));
		fdout = fopen(strftimebuf, "w");
		if (!fdout) { perror(strftimebuf); }
		else {
			fprintf(stderr, "Writing to file : %s\n", strftimebuf);
			hexdump(fdout, buf, n);
			fprintf(fdout, "\n");
			strftime(strftimebuf, sizeof(strftimebuf), "%Y-%m-%d %H:%M:%S", localtime(&start_time));
			fprintf(fdout, "Start time :            %s\n", strftimebuf);
			strftime(strftimebuf, sizeof(strftimebuf), "%Y-%m-%d %H:%M:%S", localtime(&end_time));
			fprintf(fdout, "End time :              %s\n", strftimebuf);
			fprintf(fdout, "Time interval (s) :     %u\n", time_interval);
			fprintf(fdout, "Number of mesurements : %u\n", mesurements_nbr);

			fprintf(fdout, "\nDatestamp\tTime (min)");
			for (j=0; j<channels; j++){
				fprintf(fdout, "\tT%u", j+1);
			}
			fprintf(fdout, "\n");
		}

		/* Apparementy, the bytes orders are differents for each channel
		   on our 2 channels datalogger. We could not test the byte order
		   of the remaining two. If you have a 3 or 4 channels model, maybe
		   you'll have to change the last two lines of this array. */

		const unsigned char bytes_orders[4][3] = {
			{1,  0,  2}, // channel 1
			{5,  4,  3}, // channel 2
			{8,  7,  6}, // channel 3 (untested)
			{11, 10, 9}  // channel 4 (untested)
		};

		for(i=0;i<mesurements_nbr;i++){
			tmp_time = start_time+time_interval*i;
			strftime(strftimebuf, sizeof(strftimebuf), "%Y-%m-%d %H:%M:%S", localtime(&tmp_time));

			n = receive(port1, buf, 12);
			//hexdump(stdout, buf, n); printf(stderr, "\n"); // uncomment for the debugging
			if (!fdout) { fprintf(stderr, "\rSkipping %u/%u...", i+1, mesurements_nbr); continue; }
			fprintf(stderr, "\rReceiving %u/%u - %s", i+1, mesurements_nbr, strftimebuf);
			fprintf(fdout, "%s", strftimebuf);
			fprintf(fdout, "\t%.3f", (time_interval*i)/60.0);
			for (j=0; j<channels; j++) {
				fprintf(fdout, "\t");
				value =                     (((buf[bytes_orders[j][0]]&0x80)?-1.0:1.0) * (double)(
				               ((unsigned int)(buf[bytes_orders[j][0]]&0x7F)<<16)
				             + ((unsigned int)(buf[bytes_orders[j][1]]     )<<8 )
				             +  (unsigned int)(buf[bytes_orders[j][2]]     )    )) / 2560.0;
				if (buf[j*3+2] == 0xFF && buf[j*3+1] == 0xFF && buf[j*3] == 0xFF) { continue; }
				fprintf(fdout, "%.2f", value);
			}
			fprintf(fdout, "\n");
		}
		if (fdout) { fclose(fdout); fdout=NULL; }
		fprintf(stderr, "\n\n");
	}

	return 0;
}
