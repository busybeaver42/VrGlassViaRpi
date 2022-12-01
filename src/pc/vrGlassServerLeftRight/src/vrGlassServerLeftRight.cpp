//============================================================================
// Name        : vrGlassServerLeftRight.cpp
// Author      : Jörg Angermayer
// Version     :
// Copyright   : MIT
// Description : Server - OpenCV video streaming over TCP/IP for Left and right images
//  Left Eye   : port = 8090
// Right Eye   : port = 8092
//============================================================================
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>//malloc

using namespace cv;
using namespace std;

char str_leftEye[200];
char str_rightEye[200];

static bool syncVrFlagLeft = false;
static bool syncVrFlagRight = false;
static long long int syncFrameLeft = 0;
static long long int syncFrameRight = 0;

VideoCapture capLeft;
cv::Mat imgLeft(240, 240, CV_8UC3, Scalar(0,0,0));
VideoCapture capRight;
cv::Mat imgRight(240, 240, CV_8UC3, Scalar(0,0,0));

static pthread_t threadsLeftEye;
static pthread_t threadsRightEye;


const int memoryProImg = 172800; //240x240x3 byte
const int blkSize = 2880;
//2880 * 60 = 172800
//2880 * 40 = 115200
static uint8_t bufferLeftEyeImg888[172800];
static uint8_t bufferLeftEyeImg565[115200];
static bool imgIsInTransmitLeftStatus = true;

static uint8_t bufferRightEyeImg888[172800];
static uint8_t bufferRightEyeImg565[115200];
static bool imgIsInTransmitRightStatus = true;

/*
  int up_width = 600;
  int up_height = 400;
  Mat resized_up;
  //resize up
  resize(image, resized_up, Size(up_width, up_height), INTER_LINEAR);
 */


typedef union {
   uint16_t val;
   struct {
        uint8_t msb_0;
        uint8_t msb_1;
   } bytes;
} union2Bytes_TYPE;
union2Bytes_TYPE unionMsgLeftPkgCounter;
union2Bytes_TYPE union565LeftConvertVal;


union2Bytes_TYPE unionMsgRightPkgCounter;
union2Bytes_TYPE union565RightConvertVal;

const int msgPkgCounterLimit = 60;
const int msgPkgCounterLimit565 = 40;
const int msgSizeRecv = 6;
const int msgSizeSend = blkSize+2;// +2 because of EOF, if you not use closed() /16

//Left
    int socket_desc, sock, clientLen;
   // struct sockaddr_in server, client;
    struct sockaddr_in client;
    char client_message[msgSizeRecv];//= {0};
    char server_message[msgSizeSend];//= {0};
    int portLeftEye = 8090;
//Right
    int socket_descR, sockR, clientLenR;
  //  struct sockaddr_in server, client;
    struct sockaddr_in clientR;
    char client_messageR[msgSizeRecv];//= {0};
    char server_messageR[msgSizeSend];//= {0};
    int portRightEye = 8092;

void init();
uint16_t rgb888torgb565(uint8_t *rgb888Pixel);

//Socket function definition
//Left
short SocketCreate(void);
void clearBuffer();
short acceptSocketConnectionFromClient(short socket_desc);
void receiveData();
void sentData();
void leftEyeSocketServerCommunication(std::string srcPath);
//void BindCreatedSocket(int hSocket);
void leftEyeBindCreatedSocket(int hSocket);

//Right
short SocketCreateR(void);
void clearBufferR();
short acceptSocketConnectionFromClientR(short socket_desc);
void receiveDataR();
void sentDataR();
void rightEyeSocketServerCommunication(std::string srcPath);
//void BindCreatedSocketR(int hSocket);
void rightEyeBindCreatedSocket(int hSocket);


void init(){
	sprintf(str_leftEye,"/home/joerg/Videos/3D/split/disney02Split/left240x240.mp4");
    if ( ! imgLeft.isContinuous() ) {
    	imgLeft = imgLeft.clone();
    }
	sprintf(str_rightEye,"/home/joerg/Videos/3D/split/disney02Split/right240x240.mp4");
    if ( ! imgRight.isContinuous() ) {
    	imgRight = imgRight.clone();
    }
}

uint16_t rgb888torgb565(uint8_t *rgb888Pixel)
{
    uint8_t red   = rgb888Pixel[0];
    uint8_t green = rgb888Pixel[1];
    uint8_t blue  = rgb888Pixel[2];

    uint16_t b = (blue >> 3) & 0x1f;
    uint16_t g = ((green >> 2) & 0x3f) << 5;
    uint16_t r = ((red >> 3) & 0x1f) << 11;

    return (uint16_t) (r | g | b);
}

void serializeLeftImageData(const cv::Mat &img){
	unsigned long int bufCounter;
	bufCounter = 0;

	unsigned long int bufCounter565;
	bufCounter565 = 0;

	///image
    int y=0;
    while(y<240){
    	int x=0;
		while(x<240){
			bufferLeftEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[2];//blue
			uint8_t blue = img.at<Vec3b>(y,x)[2];//blue
			bufCounter++;
			bufferLeftEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[1];//green
			uint8_t green = img.at<Vec3b>(y,x)[1];//green
			bufCounter++;
			bufferLeftEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[0];//red
			uint8_t red   = img.at<Vec3b>(y,x)[0];//red
			bufCounter++;

			union565LeftConvertVal.bytes.msb_0 = (uint8_t)((blue >> 3) & 0x1F);
			union565LeftConvertVal.bytes.msb_0 = union565LeftConvertVal.bytes.msb_0 | (((green >> 2) & 0x3F) << 5);
			union565LeftConvertVal.bytes.msb_1 = ((green >> 5) & 0x07);
			union565LeftConvertVal.bytes.msb_1 = union565LeftConvertVal.bytes.msb_1 | (((red >> 3) & 0x1F) << 3);

			bufferLeftEyeImg565[bufCounter565] = union565LeftConvertVal.bytes.msb_0;
			bufCounter565++;
			bufferLeftEyeImg565[bufCounter565] = union565LeftConvertVal.bytes.msb_1;
			bufCounter565++;

			x++;
		}
    y++;
    }
}

void serializeRightImageData(const cv::Mat &img){
	unsigned long int bufCounter;
	bufCounter = 0;

	unsigned long int bufCounter565;
	bufCounter565 = 0;

	///image
    int y=0;
    while(y<240){
    	int x=0;
		while(x<240){
			bufferRightEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[2];//blue
			uint8_t blue = img.at<Vec3b>(y,x)[2];//blue
			bufCounter++;
			bufferRightEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[1];//green
			uint8_t green = img.at<Vec3b>(y,x)[1];//green
			bufCounter++;
			bufferRightEyeImg888[bufCounter] = img.at<Vec3b>(y,x)[0];//red
			uint8_t red   = img.at<Vec3b>(y,x)[0];//red
			bufCounter++;

			union565RightConvertVal.bytes.msb_0 = (uint8_t)((blue >> 3) & 0x1F);
			union565RightConvertVal.bytes.msb_0 = union565RightConvertVal.bytes.msb_0 | (((green >> 2) & 0x3F) << 5);
			union565RightConvertVal.bytes.msb_1 = ((green >> 5) & 0x07);
			union565RightConvertVal.bytes.msb_1 = union565RightConvertVal.bytes.msb_1 | (((red >> 3) & 0x1F) << 3);

			bufferRightEyeImg565[bufCounter565] = union565RightConvertVal.bytes.msb_0;
			bufCounter565++;
			bufferRightEyeImg565[bufCounter565] = union565RightConvertVal.bytes.msb_1;
			bufCounter565++;

			x++;
		}
    y++;
    }

}

short SocketCreate(void)
{
    short hSocket;
    printf("Create the socket - left\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket == -1){
    //if (socket_desc == -1){
        printf("Could not create socket - left");
    }else{
    	printf("Socket created - left\n");
    }
    return hSocket;
}


void leftEyeBindCreatedSocket(int hSocket)
{
    int iRetval=-1;
    struct sockaddr_in  remote={0};
    /* Internet address family */
    remote.sin_family = AF_INET;
    /* Any incoming interface */
    remote.sin_addr.s_addr = htonl(INADDR_ANY);
    remote.sin_port = htons(portLeftEye); /* Local port */
    iRetval = bind(hSocket,(struct sockaddr *)&remote,sizeof(remote));
    if(iRetval < 0){
    	perror("bind failed - left.");
    }else{
    	printf("bind done - left\n");
    }
}


short acceptSocketConnectionFromClient(short socket_desc){
    sock = accept(socket_desc,(struct sockaddr *)&client,(socklen_t*)&clientLen);
    if(sock < 0){
        perror("accept failed - left");
    }else{
    	//printf("Connection accepted\n");
    }
    return sock;
}

void clearBuffer(){
    memset(client_message, 0, sizeof(client_message));
    memset(server_message, 0, sizeof(server_message));
}


void receiveData(){
	//short status = recv(sock, client_message, msgSizeRecv, 0);//works too
	short status = read(sock, client_message, msgSizeRecv);
    if( status < 0){
        printf("recv failed - left");
    }
    unionMsgLeftPkgCounter.bytes.msb_0 = (uint8_t)client_message[2];
    unionMsgLeftPkgCounter.bytes.msb_1 = (uint8_t)client_message[3];
    uint8_t ctrlDataFormat = (uint8_t)client_messageR[4];

	////////////
    long int msgBlkCounter = (unionMsgLeftPkgCounter.val - 1)*blkSize;

    //Update server message to send data to client
	long int i = 0;
	while(i<msgSizeSend){
		if(ctrlDataFormat == 0){
			server_message[i] = bufferLeftEyeImg888[msgBlkCounter];
		}
		if(ctrlDataFormat == 1){
			server_message[i] = bufferLeftEyeImg565[msgBlkCounter];
		}
		msgBlkCounter++;

	i++;
	}

	if(ctrlDataFormat == 0){
		if(unionMsgLeftPkgCounter.val > msgPkgCounterLimit){
			imgIsInTransmitLeftStatus = false;
		}
	}
	if(ctrlDataFormat == 1){
		if(unionMsgLeftPkgCounter.val > msgPkgCounterLimit565){
			imgIsInTransmitLeftStatus = false;
		}
	}
}

void sentData(){
	short statusleft = write(sock, server_message, msgSizeSend);
    if(statusleft < 0){
        printf("SendData failed - left: status %d\n", statusleft);
    }
}

void *leftEyeSocketServerCommunication(void *ptr){
	//capLeft.open(srcPath);
	capLeft.open("/home/joerg/Videos/3D/split/disney02Split/left240x240.mp4");

	//comm
    socket_desc = SocketCreate();//Create socket
    leftEyeBindCreatedSocket(socket_desc);//bind
    listen(socket_desc, 1);//Listen

    printf("left eye - Waiting for incoming connections...\n");
    while(1)
    {
    	if(syncFrameRight >= syncFrameLeft){
    		syncVrFlagLeft = true;
    	}else{
    		syncVrFlagLeft = false;
    	}

		if(imgIsInTransmitLeftStatus == false){
			imgIsInTransmitLeftStatus = true;
			if(syncVrFlagLeft == true){
				capLeft.read(imgLeft);
				serializeLeftImageData(imgLeft);
				syncFrameLeft++;
			}
		}

		clientLen = sizeof(struct sockaddr_in);
		short accsock = acceptSocketConnectionFromClient(socket_desc);//accept connection from an incoming client
		clearBuffer();
		receiveData();
		sentData();
		close(accsock);

    }
    close(socket_desc);
    capLeft.release();
}

////////////////////////////////Right
short SocketCreateR(void)
{
    short hSocket;
    printf("Create the socket - right\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket == -1){
    //if (socket_desc == -1){
        printf("Could not create socket - right");
    }else{
    	printf("Socket created - right\n");
    }
    return hSocket;
}

void rightEyeBindCreatedSocket(int hSocket)
{
    int iRetval=-1;
    struct sockaddr_in  remote= {0};
    /* Internet address family */
    remote.sin_family = AF_INET;
    /* Any incoming interface */
    remote.sin_addr.s_addr = htonl(INADDR_ANY);
    remote.sin_port = htons(portRightEye); /* Local port */
    iRetval = bind(hSocket,(struct sockaddr *)&remote,sizeof(remote));
    if(iRetval < 0){
    	perror("bind failed - right.");
    }else{
    	printf("bind done - right\n");
    }
}

short acceptSocketConnectionFromClientR(short socket_desc){
    sockR = accept(socket_desc,(struct sockaddr *)&clientR,(socklen_t*)&clientLenR);
    if(sockR < 0){
        perror("accept failed - right");
    }else{
    	//printf("Connection accepted\n");
    }
    return sockR;
}

void clearBufferR(){
    memset(client_messageR, 0, sizeof(client_messageR));
    memset(server_messageR, 0, sizeof(server_messageR));
}

void receiveDataR(){
	//short status = recv(sock, client_message, msgSizeRecv, 0);//works too
	short status = read(sockR, client_messageR, msgSizeRecv);
    if( status < 0){
        printf("recv failed - right");
    }

    unionMsgRightPkgCounter.bytes.msb_0 = (uint8_t)client_messageR[2];
    unionMsgRightPkgCounter.bytes.msb_1 = (uint8_t)client_messageR[3];
    uint8_t ctrlDataFormat = (uint8_t)client_messageR[4];

	////////////
    long int msgBlkCounter = (unionMsgRightPkgCounter.val - 1)*blkSize;

    //Update server message to send data to client
	long int i = 0;
	while(i<msgSizeSend){
		if(ctrlDataFormat == 0){
			server_messageR[i] = bufferRightEyeImg888[msgBlkCounter];
		}
		if(ctrlDataFormat == 1){
			server_messageR[i] = bufferRightEyeImg565[msgBlkCounter];
		}
		msgBlkCounter++;
	i++;
	}

	if(ctrlDataFormat == 0){
		if(unionMsgRightPkgCounter.val > msgPkgCounterLimit){
			imgIsInTransmitRightStatus = false;
		}
	}

	if(ctrlDataFormat == 1){
		if(unionMsgRightPkgCounter.val > msgPkgCounterLimit565){
			imgIsInTransmitRightStatus = false;
		}
	}

}

void sentDataR(){
	short status = write(sockR, server_messageR, msgSizeSend);
    if(status < 0){
        printf("SendDataR failed - right: status %d\n",status);
    }
}

void *rightEyeSocketServerCommunication(void *ptr){
	//capRight.open(srcPath);
	capRight.open("/home/joerg/Videos/3D/split/disney02Split/right240x240.mp4");

	//comm
	socket_descR = SocketCreateR();//Create socket
    rightEyeBindCreatedSocket(socket_descR);//bind
    listen(socket_descR, 1);//Listen

    printf("rigth eye - Waiting for incoming connections...\n");
    while(1)
    {
    	if(syncFrameLeft >= syncFrameRight){
    		syncVrFlagRight = true;
    	}else{
    		syncVrFlagRight = false;
    	}

		if(imgIsInTransmitRightStatus == false){
			imgIsInTransmitRightStatus = true;
			if(syncVrFlagRight == true){
				capRight.read(imgRight);
				serializeRightImageData(imgRight);
				syncFrameRight++;
			}
		}

		clientLenR = sizeof(struct sockaddr_in);
		short accsock = acceptSocketConnectionFromClientR(socket_descR);//accept connection from an incoming client
		clearBufferR();
		receiveDataR();
		sentDataR();
		close(accsock);

    }
    close(socket_descR);
    capRight.release();
}


int main(){
	std::string strPathLeftEye("/home/joerg/Videos/3D/split/disney02Split/left240x240.mp4");
	pthread_create(&threadsLeftEye, NULL, leftEyeSocketServerCommunication, (void *)&strPathLeftEye);

	std::string strPathRightEye("/home/joerg/Videos/3D/split/disney02Split/rightt240x240.mp4");
	pthread_create(&threadsRightEye, NULL, rightEyeSocketServerCommunication, (void *)&strPathRightEye);

	while(1){
		//sleep(30);
	}
    return 0;
}
