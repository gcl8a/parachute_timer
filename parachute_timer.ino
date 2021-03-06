#include "imu.h"
#include <TinyScreen.h>

//define the display
TinyScreen display = TinyScreen(TinyScreenPlus);

#define GRAVITY 1366 //with accelerometer set to  +-16g full scale => 0.732 mg/LSB => 1g = 1366

//define the states of the system -- see state diagram
enum ACCEL_STATE {RESETTING, STANDBY, READY, FREEFALL, CHUTE, LANDED};

//threshold for detecting release
#define FALL_THRESHOLD (GRAVITY / 3)

//threshold for detecting landing
#define LAND_THRESHOLD (5 * GRAVITY)  

//threshold for detecting if the system is still before dropping
#define WAIT_THRESHOLD ((GRAVITY * 7) / 8)  

//length of time system must be "still" before dropping
#define WAIT_TIME 3000

//minimum flight time
#define MIN_TIME 500 //minimum time for impact based on free fall -- don't want false positive when chute opens

//start at resetting
ACCEL_STATE state = RESETTING;

uint32_t startTime = 0;
uint32_t landTime = 0;

#define  SERIAL_PORT_SPEED  115200

#ifdef SERIAL_PORT_MONITOR
  #define SerialMonitor SERIAL_PORT_MONITOR
#else
  #define SerialMonitor SerialUSB
#endif

void setup()
{
  SerialMonitor.begin(115200);
  //while (!SerialMonitor) {} // Uncomment this if you want to wait for the Serial monitor to open -- but then you must use it

  SerialMonitor.println("Initializing..."); //let us know we're talking

  //initialize the IMU
  if (!SetupIMU()) //see imu.h for details of what this does
  {
    SerialMonitor.println("Failed to communicate with LSM9DS1.");
    while (1) {}  //die gracefully
  }

  display.begin();
  
  //sets main current level, valid levels are 0-15
  display.setBrightness(10);

  startTime = millis();
  
  SerialMonitor.println("Done initializing.");
}

void loop(void)
{
  bool input = false;
  if(SerialMonitor.available())
  {
    // Received a message
    SerialMonitor.read();
    input = true;
  }

  //button state machine
  static int prevState = 1;
  int currState = display.getButtons();
  
  if(prevState && !currState)
  {
    input = true;
  }

  prevState = currState;

  if(input) //button has been pressed
  {
    if(state == RESETTING || state == LANDED)
    {
      state = STANDBY;
      startTime = millis();
      String message("\nPlease wait ");
      message += String(WAIT_TIME / 1000);
      message += (" seconds before dropping.\n");
      PrintScreen(message);
    }

    else 
    {
      state = RESETTING;
      display.clearScreen();
      String message("Please place the parachute.\n");
      PrintScreen(message);
    }
  }
  
  //if there is new accelerometer data available
  if(imu.accelAvailable())
  {
    imu.readAccel();

    float ax = imu.ax;
    float ay = imu.ay;
    float az = imu.az;

    //magnitude
    float accel = sqrt(ax * ax + ay * ay + az * az);

    if (state == STANDBY)
    {
      if(fabs(accel - GRAVITY) > WAIT_THRESHOLD) 
      {
        if(millis() - startTime > 500)
        {
          String message("Hold still or press <return> to reset.");
          PrintScreen(message);
  
          startTime = millis();
        }
      }
      
      if(millis() - startTime > WAIT_TIME)
      {
        state = READY;
        display.drawRect(0,0,96,64,TSRectangleFilled,TS_8b_Green);

        String message("\nDrop when ready!\n");
        PrintScreen(message);
      }
    }
    
    else if(state == READY)
    {
      if(accel < FALL_THRESHOLD)
      {
        startTime = millis();
        state = FREEFALL;

        String message("\nFalling!\n");
        PrintScreen(message);
      }
    }

    else if(state == FREEFALL)
    {
      if(millis() - startTime > MIN_TIME)
        state = CHUTE;
    }

    else if(state == CHUTE)
    {
      if(accel > LAND_THRESHOLD)
      {
        landTime = millis();
        state = LANDED;

        String message("\n\nLanded in ");
        message += String((landTime - startTime) / 1000.);
        message += " seconds!\n\n";
        
        PrintScreen(message);
      }
    }
  }
}

int PrintScreen(String inputString)
{
  SerialMonitor.println(inputString);

  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.setCursor(0,10);
  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.print(inputString);

  return 0;
}

