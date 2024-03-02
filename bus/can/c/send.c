#include "unistd.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "linux/can/raw.h"
#include "linux/can.h"
#include "sys/socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define data_size 8
#define BUS "can0"
#define command "sudo ip link set can0 type can bitrate 1000000"
#define up "sudo ifconfig can0 up"
#define down "sudo ifconfig can0 down"

int can_init()
{
    system(down);
    system(command);
    system(up);
    return 0;
}

void copych(__u8* data, __u8* datach)
{
    int num = CAN_MAX_DLEN;
    for(int i=0; i<num; i++){
        data[i] = datach[i];
    }

}

int can_send_init(int s)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, BUS);
    ioctl(s, SIOCGIFINDEX, &ifr); 
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr*)&addr, sizeof(addr));
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
    return s;
}

void can_send_data_init(can_frame* frame, __u8 id, __u8 dlc, __u8 data[])
{
    frame->can_id = id;
    frame->can_dlc = dlc;
    copych(frame->data, data);
}

int can_receive_init(int s)
{
    struct sockaddr_can addr;
    struct ifreq ifr;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, BUS);
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    return s;
}

void can_receive_data_init(can_filter* rfilter, canid_t id, canid_t can_mask)
{
    rfilter->can_id = id;
    rfilter->can_mask = can_mask;
}

int main()
{
    can_init();
    int s, nbytes;
    struct can_frame frame;
    struct can_frame reframe;
    struct can_filter rfilter;
    __u8 clc[] = "00000000";
    __u8 data[] = "01234567";


    s = can_send_init(s);

    can_send_data_init(&frame, 0x11, data_size, data);

    nbytes = write(s, &frame, sizeof(frame));
    if(nbytes != sizeof(frame))
    {
        printf("send_error!!!!");
        return 1;
    }
    close(s);
    s = can_receive_init(s);

    can_receive_data_init(&rfilter, 0x22, CAN_SFF_MASK);
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
    
    nbytes = read(s, &reframe, sizeof(reframe));
    // printf("frame_data=%s,frame[8]=%c,sizeof=%d\n",frame.data,frame.data[9],sizeof(frame.data)/sizeof(__u8));
    // printf("reframe_data=%s",reframe.data);

    for(int i=0; i<data_size; i++)
    {
        if(frame.data[i] != reframe.data[i]){
            // printf("i=%d,frame.data[i]=%c,reframe.data[i]%c",i ,frame.data[i],reframe.data[i]);
            // printf("error");
            return 1;
        }

    }
    close(s);

    return 0;
}
