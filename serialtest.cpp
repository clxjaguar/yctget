/* 
   Name:        serialtest
   Author:      cLx / RJS <clx.kat@gmail.com>
   Description: Send datas to ... whatever...
   Copyright:   CC http://clx.freeshell.org/ 
*/

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <cstdlib>

#include "serial.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef WIN32
#define DEFAULTSERIALPORT "COM1"
#else
#define DEFAULTSERIALPORT "/dev/ttyUSB0"
#include <unistd.h>

void Sleep(unsigned int ms) {
     usleep(1000*ms);
}
#endif

struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"serialport", 1, NULL, 's'},
    {0, 0, 0, 0}
};

void usage(char *prog){  
    int i = 0;
    fprintf(stderr, "usage: %s\n", prog);
    while(long_options[i].name){
        printf("-%c, --%-15s\n", long_options[i].val, long_options[i].name);
        i++;
    }
    exit(1);
}

int main(int argc, char *argv[]){
    char data[300] = {0};
    unsigned int dataout[300] = {0};
    int c;
    bool err = FALSE, printusage = FALSE;
    
    char *serialport = NULL;
    serial port1;
    
    int option_index = 0;      
    
    while ((c = getopt_long(argc, argv, "abc:d:012", long_options, &option_index)) != -1){
        switch (c){
            case 'h': 
                printusage = TRUE;
                break;

            case '?': // print usage
                printusage = TRUE;
                break;
                
            case 's': // serialport
                serialport = new char(strlen(optarg)+1);
                strcpy(serialport, optarg);
                break;

            default:
                fprintf(stderr, "Sorry, --%s (-%c) is not yet implemented.\n", long_options[option_index].name, c);
                break;
        }
    }

    if (!serialport){
        serialport = new char(strlen(DEFAULTSERIALPORT)+1);
        strcpy(serialport, DEFAULTSERIALPORT);    
    }

    if (err || printusage) { usage(argv[0]); }
    
    if (!err){

        if (!port1.isopened()){
	    printf("Opening [%s]...\n", serialport);
            port1.open(serialport, 9600);
        }
        if (!port1.isopened()){
            perror("Got some kind of problems with that %%#@!* serial port");
            return 1;
        }
        printf("Now sending... ^C to stop !\n");

        {
            unsigned int i=0, j=0, k=0;
            char buf[8], minibuf[10];
            char sendstr[] = "Hello serial world !";

            for(;;){
                port1.send(sendstr[i++]); 
                if (sendstr[i] == 0) { 
                    i=0, k++; 
                    sprintf(minibuf, " %3d", k);
                    port1.send(minibuf);
                    port1.send('\n');
                }
                //printf("%s\n", sendstr);
                Sleep(30);
                
                
                if (++j<50){
                    if (port1.receive(buf, sizeof(buf))){
                        printf("%s", buf);
                        fflush(stdout);
                    }
                }
                
                
                j++;
                if (j>150){j=0;};
                
            }
        }
    
    }
    // maintenant qu'on a terminé, on range tout bien proprement...
    if (port1.isopened()) { port1.close(); }
    if (serialport) { delete serialport; serialport = NULL; }
    return err;
}
