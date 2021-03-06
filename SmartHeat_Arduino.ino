#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
#include <Timer.h>

WiFly wifly;

/* Sensor values */
boolean presence_array [30];

/* Settings */ //be aware of also changing sample bufer size (below) and watchdog timer (in sleep file)
unsigned int t_transmit_min = 2;  //transimission period in minutes
unsigned int t_transmit = t_transmit_min * 60;  //in seconds
unsigned int t_motion = 4;    //motion sample period in seconds
unsigned int motion_thresh = 10;  //threshold for motion detection

/* Serial for ZMotion detection */
SoftwareSerial zilog(2, 3);

/* For Debugging */
//SoftwareSerial debug(10, 11);

/* FSM */
enum state {INIT, READ, SEND, SLEEP, WAKE, EXIT};
state STATE = INIT;
unsigned int sample_count = 0;

void setup()
{
}

void loop()
{
  
  switch(STATE)
  {
    case INIT:
      //debug.println("State: Init");
      wifi_init();
      temp_hum_init();
      zilog_init();
      sample_count = 0;
      
      wifi_sleep();
      STATE = READ;  
      break;
      
    case READ:
      presence_array[sample_count] = zilog_detect_motion();
      sample_count++;
      STATE = SLEEP;
      break;
      
    case SLEEP:
      //debug.println("send to sleep...");
      //avr_sleep();
      delay(4000);
      STATE = WAKE;
      break;
      
    case WAKE:
      if(sample_count * t_motion >= t_transmit)
      {
        wifi_wake();
        timeout_start();
        STATE = SEND;
      }
      else  
        STATE = READ;
      break;
      
    case SEND:
      //debug.println("State: Send");
      wifly.setOUTPUT("0x30 0x70");
      if(wifi_send(temp_read(), zilog_process(&presence_array[0], motion_thresh), ldr_read(), hum_read())) //dummy humidity
      {
        wifly.setOUTPUT("0x10 0x70");
        sample_count = 0;
        timeout_stop();
        wifi_sleep();
        STATE = SLEEP;
      }
      break;
      
    case EXIT:
      wifly.factoryRestore();
      break;
  }
}

void upload()
{
  //debug.println("timerevent");
  STATE = WAKE;
}
