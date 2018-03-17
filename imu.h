#include <Wire.h>
#include <BMA250.h>

// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M       0x1C
#define LSM9DS1_AG      0x6A

BMA250 imu;

int SetupIMU()
{
  imu.begin(BMA250_range_8g, BMA250_update_time_8ms);//This sets up the BMA250 accelerometer
  return 1;
}

