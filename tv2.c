#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdbool.h>
#define INTERVAL_SEC 0 /* second */
#define INTERVAL_MS 100 /* microseconds */

#define MOST_CATALOG_VERSION	 	0x01
#define FBLOCK_AVAILABILITY 		0x02
#define BAP_CONFIG 			0x03
#define DEVICE_SERVICE_SUPPORT	 	0x04
#define FSG_CONTROL 			0x0D
#define FSG_SETUP 			0x0E
#define FSG_OPERATION_STATE 		0x0F
#define TUNED_PROGRAM_HYBRID_TV 	0x10
#define TUNE_TO_HYBRID	 		0x11
#define TUNE_UP		 		0x12
#define TUNE_DOWN		 	0x13
#define BROWSER_LIST 			0x14
#define SOURCE 				0x15
#define SWITCH_SOURCE			0x16
#define TVNORM_AREA			0x17
#define AVNORM				0x18
#define TVNORM_AREA_SUB_LIST		0x19
#define AUDIO_FORMAT_TV			0x1A
#define TVADDIVERSITY			0x1B
#define TVTERMINAL_MODE			0x1C
#define INFO_BAR_CONTROL		0x1D
#define INFO_BAR 			0x1E
#define TUNER_STATUS			0x1F
#define EPGCONTROL			0x20
#define TTCONTROL			0x21
#define TTPAGE_TO			0x22
#define TTMODE				0x23
#define COLOR				0x24
#define CONTRAST			0x25
#define BRIGHTNESS			0x26
#define TINT				0x27
#define TVDISPLAY_NOTIFICATION		0x28


 struct tuned_program {
   unsigned char preset_number;
   unsigned char* preset_name;
 };

typedef struct tuned_program Tuned_program;
int s,read_can_port;
unsigned char heartbeat_counter, second_counter; 
volatile unsigned char timeout_counter;
timer_t update_timer;
unsigned char *program_list[] = {"Program1","Program 2","Program  3"};

//CAN Variables

unsigned char most_catalog_version_data[] = {0x03,0x00,0x2C,0x00,0x03,0x00,0x38,0x07,0xFF,0xFF,0xFF,
0x80,0x00,0x00,0x0A,0x00,0x00,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x11,0x43,0x61,0x72,0x61,0x75,0x64,0x69,
0x6F,0x2D,0x53,0x79,0x73,0x74,0x65,0x6D,0x73,0xAE,0x05,0x3D,0x43,0x61,0x53,0xAE,0x05,0x01,0x00,0x19,0x00,
0x01,0x00,0x01,0x00,0x00,0xFF,0x00,00,00,00,0x09,00,00,00,00,00,00,00,00,00,00,0x06,00,0x52,0x47,0x33,0x33,00};
unsigned char fblock_availability_data[] = {0x03,0x00,0x2C,0x00,0x03,0x00};
unsigned char BAP_config_data[] = {0x38,0x07,0xFF,0xFF,0xFF,0x80};
unsigned char device_service_support_data = 0x0A;
unsigned char fsg_control_data = 0x10;
unsigned char fsg_setup_data = 0x00;
unsigned char fsg_operation_state_data = 0x00;
unsigned char tuned_program_hybrid_tv_data[40];
unsigned char info_bar_data[40];
unsigned char browser_list_data[40] = {00,00,0x27};
unsigned char source = 0;
unsigned char switch_source = 0;
unsigned char tvnorm_area = 1;
unsigned char avnorm = 0;
unsigned char audio_format_tv = 1;
unsigned char tvaddiversity = 0;
unsigned char tvnorm_area_sub_list[] = {1,0};
unsigned char tvterminal_mode[] = {0,0};
unsigned char info_bar_control = 0;
unsigned char tuner_status = 0x80;
unsigned char ttmode = 0;
unsigned char ttpage[] =  {0,0};
unsigned char color = 32;
unsigned char contrast = 32;
unsigned char brightness = 32;
unsigned char tint = 32;
unsigned char tvdisplay_notification = 1;
unsigned char epgcontrol = 0;
unsigned char ttcontrol = 0;
bool tuner_inited = 0;
//unsigned char tuned_program_hybrid_tv_data = 0x00;
//unsigned const char tuned_program_hybrid_tv_id = 0x0F;





///////////////



void send_can_frame(unsigned int id, unsigned char len, unsigned char *data) {
    struct can_frame frame;
        
    frame.can_id  = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, 8);
    
write(s, &frame, sizeof(struct can_frame));


}

void send_bap_single_byte(unsigned char data, unsigned char importance, unsigned char fct_ind) {

    unsigned char temp[3];
    
    temp[0]=importance;
    temp[1]=fct_ind;
    temp[2]=data;
    send_can_frame(0x6D3,3,temp);

}



void send_bap_frame(unsigned char *data,unsigned char data_len,  unsigned char importance, unsigned char fct_ind, unsigned char fct_type, bool encap) {

    unsigned char temp[255], temp2[8];
    unsigned char *Buf;
    unsigned char header, continuer;
    int i=0, block_counter=0, block_size_counter=0;

  //printf("size of buf: %d\n", data_len);

    Buf = temp;
    if(encap) {
	switch (importance) {
	    case 0: //0x3b
	    header=0x90;
	    continuer=0xD0;    
	    break;
	    case 1: //0x4b
	    header=0xA0;
	    continuer=0xE0;
	    break;
	    case 2: //0x4b inne
	    header=0x80;
	    continuer=0xC0;
	    break;
	}
    
	
	*Buf++ = header;
	*Buf++ = data_len;
	*Buf++ = fct_ind;
	*Buf++ = fct_type;
	block_size_counter = (short)(Buf-temp) -1;
	for (i=0; i<data_len; i++) {
	
	    if (block_size_counter == 7) {
		if (block_counter==0x10) block_counter=0;
		*Buf++ = continuer+block_counter;
		block_counter++;
		block_size_counter=0;
		
	    } else {
		
		//block_size_counter++;
	    }
	*Buf++ = data[i];
	block_size_counter++;
	}
        
	block_size_counter=0;
	for (i=0; i<(short)(Buf-temp); i++) {
	
	    //printf("%02X",temp[i] );
	    temp2[block_size_counter]=temp[i];
	    block_size_counter++;
	    if (block_size_counter==8) {
	    //    puts("");
		send_can_frame(0x6D3,8,temp2);
		block_size_counter=0;
	    }
	
	}     
	    //if (((Buf-temp)-i)<8) puts("last frame");
	    //printf("lll: %d\n",block_size_counter);
	    if (block_size_counter) send_can_frame(0x6D3,block_size_counter,temp2);
    } else { //encap

	if (data_len>6) return;
	*Buf++ = fct_ind;
	*Buf++ = fct_type;
	for (i=0; i<data_len; i++) {
	    *Buf++ = data[i];
	}
	memcpy(temp2,temp,8);
	send_can_frame(0x6D3,data_len+2,temp2);

    }

}



void send_tv_heartbeat_frame() {
    struct can_frame frame;
    //unsigned char temp[] = "1234567890qwertyuiopasdfghjklzxcvbnm";    
    unsigned char temp[] = "aq";    


frame.can_id  = 0x602;
    frame.can_dlc = 8;
    frame.data[0] = 0x00;
    frame.data[1] = 0x00;
    frame.data[2] = 0x00;
    frame.data[3] = 0x00;
    frame.data[4] = 0x00;
    frame.data[5] = 0x00;
    frame.data[6] = 0x00;
    frame.data[7] = 0x00;
write(s, &frame, sizeof(struct can_frame));

//send_bap_frame(temp,sizeof(temp),0,0x3b,0x10,0);

}


void send_tuned_program (Tuned_program *t) {

    unsigned char temp[100], temp2[]={0x43,0x61,0x53}, temp3[]={0x08,0x54,0x56,0x2D,0x43,0x61,0x53,0xAE,0x20,0x02,0x01,0x02}; 
    
    //unsigned char temp[100], temp2[]={0x43,0x61,0x53}, temp3[]={0x08,0x54,0x56,0x2D,0x43,0x61,0x53,0xAE,0x20,0x02,0x01,0x00}; 
    unsigned char *Buf;
    int i;
    Buf = temp;
    
    for (i=0; i<sizeof(temp2); i++) {
	*Buf++ = temp2[i];
    }
    *Buf++ = t->preset_number;
    
    i=0;
    while(t->preset_name[i]) i++;   
    *Buf++ = i;
    i=0;
    while(t->preset_name[i]) {
	*Buf++ = t->preset_name[i];
	i++;
    }
    for (i=0; i<sizeof(temp3); i++) {
	*Buf++ = temp3[i];
    }

    send_bap_frame(temp,Buf-temp,0,0x3B,TUNED_PROGRAM_HYBRID_TV,1);

}


void send_info_bar (Tuned_program *t) {

    //unsigned char temp[100], temp2[]={0x43,0x61,0x53}, temp3[]={0,0,0,0}; 
    unsigned char temp[100], temp2[]={0x43,0x61,0x53}, temp3[]={0,0,0x59,0x23}; 
    unsigned char *Buf;
    int i;
    Buf = temp;
    
    for (i=0; i<sizeof(temp2); i++) {
	*Buf++ = temp2[i];
    }
    *Buf++ = t->preset_number;
    
    i=0;
    while(t->preset_name[i]) i++;   
    *Buf++ = i;
    i=0;
    while(t->preset_name[i]) {
	*Buf++ = t->preset_name[i];
	i++;
    }
    for (i=0; i<sizeof(temp3); i++) {
	*Buf++ = temp3[i];
    }

    send_bap_frame(temp,Buf-temp,1,0x4B,INFO_BAR,1);

}


void send_program_list (unsigned char* list[], unsigned char size, unsigned char start_num) {

    unsigned char temp[100], temp2[]={0x43,0x61,0x53}; 
    unsigned char *Buf;
    //unsigned char buf2[40];
    int i,ii;
    Buf = temp;
    
    //strcpy(buf2,list[0]);
    //printf("%c\n",list[0][1]);
    if (start_num > (size/4)) {
	*Buf++ = 0;
	*Buf++ = start_num;
	*Buf++ = 0; //no channels
	send_bap_frame(temp,Buf-temp,2,0x4B,BROWSER_LIST,1);
	return;
    }
    *Buf++ = 0;
    *Buf++ = start_num;    
    *Buf++ = size/4;
    for (ii=0; ii<(size/4); ii++) {
	for (i=0; i<sizeof(temp2); i++) {
	    *Buf++ = temp2[i];
	}
    
	i=0;
	*Buf++ = ii+1;
	while(list[ii+start_num][i]) i++;   
	//printf("ii=%d start_num=%d\n",ii,start_num);
	//printf("%c\n",list[ii+start_num][i]);
	//printf("i=%d\n",i);
	*Buf++ = i;
	i=0;
	while(list[ii+start_num][i]) {
	    *Buf++ = (list[ii+start_num][i]);
	    i++;
	} 
	*Buf++ = 2;  //unknown
	*Buf++ = 1;  //unknown
	*Buf++ = 2;  //unknown
    }
    
    send_bap_frame(temp,Buf-temp,2,0x4B,BROWSER_LIST,1);


}


void send_heartbeat(void) {
    
    Tuned_program t;
    
    if (!tuner_inited) return;
    heartbeat_counter++;
    t.preset_number=1;
    t.preset_name="Mirror Link";
    switch (heartbeat_counter) {
	case 1: 
	    send_bap_frame(fblock_availability_data,sizeof(fblock_availability_data),0,0x3B,FBLOCK_AVAILABILITY,0);
	break;
	case 2: 
	    send_bap_frame(BAP_config_data,sizeof(BAP_config_data),0,0x3B,BAP_CONFIG,0);
	break;
	case 3: 
	    send_bap_single_byte(device_service_support_data,0x3B,DEVICE_SERVICE_SUPPORT);
	break;
	case 4: 
	    send_bap_single_byte(fsg_control_data,0x3B,FSG_CONTROL);
	break;
	case 5: 
	    send_bap_single_byte(fsg_setup_data,0x3B,FSG_SETUP);
	break;
	case 6: 
	    send_bap_single_byte(fsg_operation_state_data,0x3B,FSG_OPERATION_STATE);
	break;
	case 7: 
	    send_tuned_program(&t);
	break;
	case 8: 
	    send_bap_single_byte(source,0x3B,SOURCE);
	break;
	case 9: 
	    send_bap_single_byte(tvnorm_area,0x3B,TVNORM_AREA);
	break;
	case 10: 
	    send_bap_single_byte(avnorm,0x3B,AVNORM);
	break;
	case 11: 
	    send_bap_frame(tvnorm_area_sub_list,1,2,0x3B,TVNORM_AREA_SUB_LIST,1);
	break;
	case 12: 
	    send_bap_single_byte(audio_format_tv,0x3B,AUDIO_FORMAT_TV);
	break;
	case 13: 
	    send_bap_single_byte(tvaddiversity,0x3B,TVADDIVERSITY);
	break;
	case 14: 
	    send_bap_frame(tvterminal_mode,sizeof(tvterminal_mode),2,0x3B,TVTERMINAL_MODE,0);
	break;
	case 15: 
	    send_bap_single_byte(info_bar_control,0x3B,INFO_BAR_CONTROL);
	break;
	case 16: 
	    send_info_bar(&t);
	break;
	case 17: 
	    send_bap_single_byte(tuner_status,0x3B,TUNER_STATUS);
	break;
	case 18: 
	    send_bap_single_byte(ttmode,0x3B,TTMODE);
	break;
	case 19: 
	    send_bap_single_byte(color,0x3B,COLOR);
	break;
	case 20: 
	    send_bap_single_byte(contrast,0x3B,CONTRAST);
	break;
	case 21: 
	    send_bap_single_byte(brightness,0x3B,BRIGHTNESS);
	break;
	case 22: 
	    send_bap_single_byte(tint,0x3B,TINT);
	break;
	case 23: 
	    send_bap_single_byte(tvdisplay_notification,0x3B,TVDISPLAY_NOTIFICATION);
	    heartbeat_counter=0;
	break;
	default:
	    heartbeat_counter=0;
	break;
    }
}



static void timer_handler(int sig, siginfo_t *si, void *uc)
{   //Every 200msec
    
    if(++timeout_counter>25) {// ponad 5s
	--timeout_counter;
//	puts("Timeout!");
	
    } else {
	send_tv_heartbeat_frame();
	if (++second_counter==5) {
	    //puts("1 second Timer!");
	    send_heartbeat();
	    second_counter=0;
	}
    }
}




void timer_enable()
{
    struct sigevent te;
    struct itimerspec its;
    struct sigaction sa;
    int signal_num = SIGALRM;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(signal_num, &sa, NULL);

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = signal_num;
    timer_create(CLOCK_REALTIME, &te, &update_timer);

    its.it_interval.tv_sec = INTERVAL_SEC;
    its.it_interval.tv_nsec = INTERVAL_MS * 2000000;
    its.it_value.tv_sec = INTERVAL_SEC;
    its.it_value.tv_nsec = INTERVAL_MS * 2000000;
    timer_settime(update_timer, 0, &its, NULL);

}

void init_tuner(void) {

    send_bap_frame(fblock_availability_data,sizeof(fblock_availability_data),0,0x0B,FBLOCK_AVAILABILITY,0);
    send_bap_frame(most_catalog_version_data,sizeof(most_catalog_version_data),0,0x4b,MOST_CATALOG_VERSION,1);
    send_bap_frame((unsigned char*)("\0\0\0\0\0\0\0\0\0"),9,2,0x4b,INFO_BAR,1);
    send_bap_single_byte(0xFF,0x4b,0x1A);
    send_bap_single_byte(0x06,0x4b,0x1F);
    send_bap_frame((unsigned char*)(browser_list_data),3,2,0x4b,0x14,1);
    tuner_inited = 1;
}

void read_port()
{
    struct can_frame frame_rd;
    int recvbytes = 0,i;

    read_can_port = 1;
    while(read_can_port)
    {
        struct timeval timeout = {1, 0};
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);

        if (select((s + 1), &readSet, NULL, NULL, &timeout) >= 0)
        {
            if (!read_can_port)
            {
                break;
            }
            if (FD_ISSET(s, &readSet))
            {
                recvbytes = read(s, &frame_rd, sizeof(struct can_frame));
                if(recvbytes)
                {
                    switch(frame_rd.can_id) {
		    case 0x6c7:
		    //if (frame_rd.can_id == 0x6c7) {
			if((frame_rd.data[0] == 0x1B) && (frame_rd.data[1] == 0x02)) 
			    send_bap_frame(fblock_availability_data,sizeof(fblock_availability_data),0,0x0B,FBLOCK_AVAILABILITY,0);
			else if((frame_rd.data[0] == 0x1B) && (frame_rd.data[1] == MOST_CATALOG_VERSION))
			    init_tuner();
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == COLOR)) {
			    color = frame_rd.data[2];
			    send_bap_single_byte(color,0x4B,COLOR);} 
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == CONTRAST)) {
			    contrast = frame_rd.data[2];
			    send_bap_single_byte(contrast,0x4B,CONTRAST); }
			 else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == BRIGHTNESS)) {
			    brightness = frame_rd.data[2];
			    send_bap_single_byte(brightness,0x4B,BRIGHTNESS); }
			 else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TINT)) {
			    tint = frame_rd.data[2];
			    send_bap_single_byte(tint,0x4B,TINT); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TVADDIVERSITY)) {
			    tvaddiversity = frame_rd.data[2];
			    send_bap_single_byte(tvaddiversity,0x4B,TVADDIVERSITY); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == SWITCH_SOURCE)) {
			    switch_source = frame_rd.data[2];
			    send_bap_single_byte(switch_source,0x4B,SWITCH_SOURCE); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == FSG_CONTROL)) {
			    fsg_control_data = frame_rd.data[2];
			    send_bap_single_byte(fsg_control_data,0x4B,FSG_CONTROL); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == EPGCONTROL)) {
			    epgcontrol = frame_rd.data[2];
			    send_bap_single_byte(epgcontrol,0x4B,EPGCONTROL); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TTCONTROL)) {
			    ttcontrol = frame_rd.data[2];
			    send_bap_single_byte(ttcontrol,0x4B,TTCONTROL); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TTPAGE_TO)) {
			    
			    ttpage[0] = frame_rd.data[2];
			    ttpage[1] = frame_rd.data[3];
			    send_bap_frame(ttpage,sizeof(ttpage),2,0x4B,TTPAGE_TO,0); }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TVNORM_AREA)) {
			    tvnorm_area = frame_rd.data[2];
			    send_bap_single_byte(tvnorm_area,0x4B,TVNORM_AREA); }

			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TUNE_UP)) {
			    send_bap_single_byte(00,0x4B,TUNE_UP); }

			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TUNE_DOWN)) {
			    send_bap_single_byte(00,0x4B,TUNE_DOWN); }

			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TUNE_TO_HYBRID)) {
			    //////////////////switch to channel
			    send_bap_single_byte(audio_format_tv,0x4B,AUDIO_FORMAT_TV);
			    Tuned_program t;
			    t.preset_number=frame_rd.data[5];
			    t.preset_name="Mirror Link";
			    send_tuned_program(&t);
			    send_bap_single_byte(1,0x3B,TUNE_TO_HYBRID);
			    send_info_bar(&t);
			    //usleep(10000);
			    send_bap_single_byte(1,0x4B,TUNE_TO_HYBRID);
			    send_bap_single_byte(0x22,0x4B,0x1f);
			    send_bap_single_byte(0x32,0x4B,0x1f);
			 }
			else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TVDISPLAY_NOTIFICATION)) {
			    if (frame_rd.data[2]==0x10) tvdisplay_notification = 0;
			    if (frame_rd.data[2]==0x00) tvdisplay_notification = 1;
			    send_bap_single_byte(tvdisplay_notification,0x4B,TVDISPLAY_NOTIFICATION); }
			 else if((frame_rd.data[0] == 0x2B) && (frame_rd.data[1] == TVTERMINAL_MODE)) {
			    tvterminal_mode[0] = frame_rd.data[2];
			    tvterminal_mode[1] = frame_rd.data[3];
			    send_bap_frame(tvterminal_mode,sizeof(tvterminal_mode),2,0x4B,TVTERMINAL_MODE,0); }
			 else if(frame_rd.data[0] == 0x80) {
			    switch(frame_rd.data[3]) {
				case TVNORM_AREA_SUB_LIST:
				send_bap_frame(tvnorm_area_sub_list,1,2,0x4B,TVNORM_AREA_SUB_LIST,1);
				break;
				case BROWSER_LIST: //channel list request
				send_program_list(program_list, sizeof(program_list),frame_rd.data[5]);
				break;
			    
			    }//end switch
			}
		    //send_tst_frame();
		    fprintf(stdout,"%lX %d ",frame_rd.can_id, frame_rd.can_dlc);
                    for (i=0; i<frame_rd.can_dlc; i++) printf("%02X", frame_rd.data[i]);
                    fprintf(stdout,"\n");
		    fflush(stdout);
		    break;
		    case 0x575: //ignition
			//printf("ignition: %02X\n",frame_rd.data[0]);
			if(frame_rd.data[0]&0x01) timeout_counter = 0; 
		    break;
		    case 0x661: //radio on/off
			if(frame_rd.data[0]&0x01) timeout_counter = 0; 
		    break;
		    }//end switch can ID
                    
                }
            }
        }

    }

}


int main(void) {

    int nbytes;
    struct sockaddr_can addr;
    struct can_frame frame;
    struct ifreq ifr;
    char *ifname = "can0";

    if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
	perror("Error while opening socket");
	return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    printf("%s at index %d\n", ifname, ifr.ifr_ifindex);
    if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("Error in socket bind");
	return -2;
    }


    timer_enable();
    init_tuner();
    while(1) read_port();
}

