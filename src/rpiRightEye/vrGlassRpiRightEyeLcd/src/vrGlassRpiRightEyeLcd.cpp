//============================================================================
// Name        : vrGlassRpiRightEyeLcd.cpp
// Author      : Jörg Angermayer
// Version     :
// Copyright   : MIT
// Description : Client to receive 240x240 RGB images
// Right Eye   : port = 8092
//============================================================================
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

using namespace std;

static int fb_width = 0;
static int fb_height = 0;
static int fb_bpp = 0;
static int fb_bytes = 0;
static int fb_data_size = 0;
static char *fbdata = 0;

static uint8_t ctrlFormat = 1; // 0 = BGR888 ; 1 = BGR565

typedef union {
   uint16_t val;
   struct {
        uint8_t msb_0;
        uint8_t msb_1;
   } bytes;
} union2Bytes_TYPE;
union2Bytes_TYPE unionMsgPkgCounter;
union2Bytes_TYPE unionConvertDummy;

static int msgPkgCounterLimit = 60;
const int msgSizeRecv = 2880;
const int msgSizeSend = 6;

const int memoryProImg = 172800; //240x240x3 byte
const int blkSize = 2880;
uint8_t bufferImg[172800];


int hSocket, read_size;
struct sockaddr_in server;
char SendBuf[msgSizeSend] = {0};
char rescvBuf[msgSizeRecv] = {0};

int ServerPort = 8092;
//char* g_ipaddress = "127.0.0.1";
char* g_ipaddress = "192.168.1.104"; //PC

void deserializeImageDataRightEye(){
    unsigned long int bufCounter;
    bufCounter = 0;

    if(ctrlFormat == 0){
            //Format 888
            int y=0;
            while(y<240){
                    int x=0;
                    while(x<240){
                            int offset = (y * fb_width + x) * 4;
                            fbdata[offset+0] = bufferImg[bufCounter];//blue
                            bufCounter++;
                            fbdata[offset+1] = bufferImg[bufCounter];//green
                            bufCounter++;
                            fbdata[offset+2] = bufferImg[bufCounter];//red
                            bufCounter++;
                            fbdata[offset+3] = 0x00;//Alpha
                    x++;
                    }
            y++;
            }
    }else{
            //Format 565
            int y=0;
            while(y<240){
                    int x=0;
                    while(x<240){
                            unionConvertDummy.bytes.msb_0 = bufferImg[bufCounter];
                            bufCounter++;
                            unionConvertDummy.bytes.msb_1 = bufferImg[bufCounter];
                            bufCounter++;

                            uint8_t blue =  ((unionConvertDummy.val & 0x1f) << 3);
                            uint8_t green = ((unionConvertDummy.val & 0x07E0) >> 3);
                            uint8_t red = ((unionConvertDummy.val & 0xf800) >> 8);

            int offset = (y * fb_width + x) * 4;
                            fbdata[offset+0] = blue;//blue
                            fbdata[offset+1] = green;//green
                            fbdata[offset+2] = red;//red
                            fbdata[offset+3] = 0x00;//Alpha

                    x++;
                    }
            y++;
            }
    }

}

void printRecvBuffer(){
	int i=msgSizeRecv-4;
	while(i<msgSizeRecv){//msgSizeSend
		unsigned int number = (unsigned int)rescvBuf[i];
		printf("rescvBuf[%d]: %d\n",i,number);
	i++;
	}
}

//Create a Socket for server communication
short SocketCreate(void)
{
    short hSocket;
    //printf("Create the socket\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    return hSocket;
}
//try to connect with server
int SocketConnect(int hSocket)
{
    int iRetval=-1;
    struct sockaddr_in remote= {0};
    //remote.sin_addr.s_addr = inet_addr("127.0.0.1"); //Local Host
    remote.sin_addr.s_addr = inet_addr(g_ipaddress); //Local Host
    remote.sin_family = AF_INET;
    remote.sin_port = htons(ServerPort);
    iRetval = connect(hSocket,(struct sockaddr *)&remote,sizeof(struct sockaddr_in));
    return iRetval;
}
// Send the data to the server and set the timeout of 20 seconds
int SocketSend(int hSocket,char* Rqst,short lenRqst)
{
    int shortRetval = -1;

    struct timeval tv;
    tv.tv_sec = 10;  // 20 Secs Timeout
    tv.tv_usec = 0;
    if(setsockopt(hSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("Time Out - client right\n");
        return -1;
    }
    shortRetval = write(hSocket, Rqst, msgSizeSend);

    close(shortRetval);
    return shortRetval;
}
//receive the data from the server
int SocketReceive(int hSocket,char* Rsp,short RvcSize)
{
    int shortRetval = -1;

    struct timeval tv;
    tv.tv_sec = 10;  // 20 Secs Timeout
    tv.tv_usec = 0;
    if(setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("Time Out - client right\n");
        return -1;
    }

    //shortRetval = recv(hSocket, Rsp, RvcSize, 0);//original
    shortRetval = recv(hSocket, Rsp, msgSizeRecv, MSG_WAITALL);//ok ---
    //shortRetval = read(hSocket, Rsp, msgSizeRecv);//ok

    // printf("Size: %d --- Data: %s\n",shortRetval,Rsp);
    return shortRetval;
}


int clientFunction(){

	if(ctrlFormat == 0){
		msgPkgCounterLimit = 60;
	}else{
		msgPkgCounterLimit = 40;
	}

	while(1){
		uint16_t k=1;
		while(k <= msgPkgCounterLimit+1){
			hSocket = SocketCreate();//Create socket
			if(hSocket == -1)
			{
				printf("Could not create socket - client right\n");
				return 1;
			}
			//printf("Socket is created\n");
			//Connect to remote server
			int socketStatus = SocketConnect(hSocket);
			if (socketStatus < 0)
			{
				printf("connect failed - client right --- socketStatus: %d\n",socketStatus);
				sleep(1);
				//perror("connect failed.\n");
				//return 1;
			}else{
				//Send data to the server
				SendBuf[0] = 'i';
				unionMsgPkgCounter.val = k;
				SendBuf[2] = unionMsgPkgCounter.bytes.msb_0;
				SendBuf[3] = unionMsgPkgCounter.bytes.msb_1;
				SendBuf[4] = ctrlFormat;

				int sendh = SocketSend(hSocket, SendBuf, msgSizeSend);//send data
				if(sendh < 0){
					printf("send err - client right: %d \n",sendh);
				}
				read_size = SocketReceive(hSocket, rescvBuf, msgSizeRecv);//recv data
				if(read_size < 0){
					printf("read err - client right: %d \n",read_size);
				}

				//no update in the last tranmission
				if(k <= msgPkgCounterLimit){
					///////////////////////
					long int msgBlkCounter = (unionMsgPkgCounter.val - 1)*blkSize;
					//printf("k: %d msgBlkCounter: %d\n",k, msgBlkCounter);
					int i=0;
					while(i<msgSizeRecv){//msgSizeSend
						bufferImg[msgBlkCounter] = (uint8_t)rescvBuf[i];
						msgBlkCounter++;
					i++;
					}
					deserializeImageDataRightEye();

				}

				//printRecvBuffer();
			}

			close(hSocket);
			shutdown(hSocket,0);  // see listen(socket_desc, 1);//Listen inside server
		k++;
		}
	}
	return(0);
}

//main driver program
int main(int argc, char *argv[])
{
    int fbfd = 0;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    fbfd = open("/dev/fb0", O_RDWR);
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
            printf("Unable to read fixed screen information");
            return 0;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
            printf( "Unable to read variable screen information");
            return 0;
    }

    fb_width = vinfo.xres;
    fb_height = vinfo.yres;
    fb_bpp = vinfo.bits_per_pixel;
    fb_bytes = fb_bpp / 8;
    fb_data_size = fb_width * fb_height * fb_bytes;
    //cout << "fb w: " << fb_width << " h: " << fb_height << " bpp: " << fb_bpp << " size: " $
    //b w: 240 h: 240 bpp: 32 size: 230400
    fbdata = (char *)mmap(0, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    memset(fbdata ,0, fb_data_size );// set all to black


	clientFunction();


    // cleanup
    munmap(fbdata, fb_data_size);
    close(fbfd);

    return 0;
}
