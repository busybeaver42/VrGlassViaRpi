//============================================================================
// Name        : vrGlassServerR.cpp
// Author      : Jörg Angermayer
// Version     :
// Copyright   : MIT
// Description : Server - OpenCV video streaming over TCP/IP
// Left Eye    : port = 8092
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

char str_rightEye[200];

VideoCapture capRight;
cv::Mat imgRight(240, 240, CV_8UC3, Scalar(0,0,0));

//const int memoryProImg = 172800; //240x240x3 byte
const int blkSize = 2880;
static uint8_t bufferRightEyeImg888[172800];//2880 * 60 = 172800
static uint8_t bufferRightEyeImg565[115200];//2880 * 40 = 115200
static bool imgIsInTransmitRightStatus = true;

typedef union {
   uint16_t val;
   struct {
        uint8_t msb_0;
        uint8_t msb_1;
   } bytes;
} union2Bytes_TYPE;
union2Bytes_TYPE unionMsgRightPkgCounter;
union2Bytes_TYPE union565RightConvertVal;

const int msgPkgCounterLimit = 60;
const int msgSizeRecv = 6;
const int msgPkgCounterLimit565 = 40;
const int msgSizeSend = blkSize+2;// +2 because of EOF, if you not use closed() /16

    int socket_desc, sock, clientLen, read_size;
    struct sockaddr_in server, client;
    char client_message[msgSizeRecv];//= {0};
    char server_message[msgSizeSend];//= {0};
    int port = 8092;

void init();
uint16_t rgb888torgb565(uint8_t *rgb888Pixel);

//Socket function definition
short SocketCreate(void);
void clearBuffer();
void receiveData();
void sentData();
void socketServerCommunication();
void BindCreatedSocket(int hSocket);
short acceptSocketConnectionFromClient(short socket_desc);


void init(){
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

void serializeImageData(const cv::Mat &img){
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
    printf("Create the socket\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket == -1){
    //if (socket_desc == -1){
        printf("Could not create socket");
    }else{
    	printf("Socket created\n");
    }
    return hSocket;
}


void BindCreatedSocket(int hSocket)
{
    int iRetval=-1;
    struct sockaddr_in  remote= {0};
    /* Internet address family */
    remote.sin_family = AF_INET;
    /* Any incoming interface */
    remote.sin_addr.s_addr = htonl(INADDR_ANY);
    remote.sin_port = htons(port); /* Local port */
    iRetval = bind(hSocket,(struct sockaddr *)&remote,sizeof(remote));
    if(iRetval < 0){
    	perror("bind failed.");
    }else{
    	printf("bind done\n");
    }
}

short acceptSocketConnectionFromClient(short socket_desc){
    sock = accept(socket_desc,(struct sockaddr *)&client,(socklen_t*)&clientLen);
    if(sock < 0){
        perror("accept failed");
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
        printf("recv failed");
    }


    unionMsgRightPkgCounter.bytes.msb_0 = (uint8_t)client_message[2];
    unionMsgRightPkgCounter.bytes.msb_1 = (uint8_t)client_message[3];
    uint8_t ctrlDataFormat = (uint8_t)client_message[4];

	////////////
    long int msgBlkCounter = (unionMsgRightPkgCounter.val - 1)*blkSize;

    //Update server message to send data to client
	long int i = 0;
	while(i<msgSizeSend){
		if(ctrlDataFormat == 0){
			server_message[i] = bufferRightEyeImg888[msgBlkCounter];
		}
		if(ctrlDataFormat == 1){
			server_message[i] = bufferRightEyeImg565[msgBlkCounter];
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

void sentData(){
	short status = write(sock, server_message, msgSizeSend);
    if(status < 0){
        printf("Send failed: status %d\n",status);
    }
}

void socketServerCommunication(){
	//opencv
	capRight.open("/home/joerg/Videos/3D/split/disney02Split/right240x240.mp4");

	//comm
    socket_desc = SocketCreate();//Create socket
    BindCreatedSocket(socket_desc);//bind
    listen(socket_desc, 1);//Listen

    printf("Waiting for incoming connections...\n");
    while(1)
    {
    	if(imgIsInTransmitRightStatus == false){
    		imgIsInTransmitRightStatus = true;
			capRight.read(imgRight);
			serializeImageData(imgRight);
    	}

        clientLen = sizeof(struct sockaddr_in);
        short accsock = acceptSocketConnectionFromClient(socket_desc);//accept connection from an incoming client
        clearBuffer();
        receiveData();
        sentData();
       // close(sock);//////////// accept socket from client for read/write
        close(accsock);
    }
    close(socket_desc);
    capRight.release();
}

void cv_clip_play_test(){
	init();

    VideoCapture capRight(str_rightEye);
    double fpsRight = capRight.get(cv::CAP_PROP_FPS); //get the frames per seconds of the video
    cout << "fpsRight: " << fpsRight << endl;


    //make img continuos
    if ( ! imgRight.isContinuous() ) {
    	imgRight = imgRight.clone();
    }

    while(1)
    {
        if (!capRight.read(imgRight)){ cout<<"\n Cannot read the video file. - imgLeft \n"; break;
        }else{
        	imshow("imgRight", imgRight);
        }

        if(waitKey(30) == 27) // Wait for 'esc' key press to exit
        {
            break;
        }
    }//end_while

    capRight.release();
}


int main(){
	//cv_clip_play_test();
	socketServerCommunication();
    return 0;
}
