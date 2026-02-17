#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Dynamixel2Arduino.h>

#define DXL_SERIAL   Serial1 //OpenCM9.04 EXP Board's DXL port Serial. (Serial1 for the DXL port on the OpenCM 9.04 board)
const int DXL_DIR_PIN = 28; //OpenCM9.04 EXP Board's DIR PIN. (28 for the DXL port on the OpenCM 9.04 board)
const float DXL_PROTOCOL_VERSION = 1.0;

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

const uint8_t DXL_ID_1 = 1; // ID of the first servo
const uint8_t DXL_ID_2 = 2; // ID of the second servo

//This namespace is required to use Control table item names
using namespace ControlTableItem;

float counter_1 = 115;
float counter_2 = 25;

struct Data_Package  {
byte throttle;      
byte pitch;
byte roll;
byte yaw;
};

Data_Package data; //Create a variable with the above structure
Data_Package prev_data; // Variable to store the previous received data

const uint64_t pipeIn = 0xE9E8F0F0E1LL;
RF24 radio(16, 17);   //CE, CSN (16,17), SCK->A1, MOSI->A7, MISO->A6 FOr OpenCM 
unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;

void ResetData()
{
// Define the inicial value of each data input. 
// The middle position for Potenciometers. (254/2=127) 
data.throttle = 127; // Motor Stop 
data.pitch = 127;  // Center 
data.roll = 127;   // Center 
data.yaw = 127;   // Center
}

void setup() {
  Serial.begin(115200);
  
  //Dynamixel
  // Set Port baudrate to 1000000bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(1000000);
  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
  // Get DYNAMIXEL information
  dxl.ping(DXL_ID_1);
  dxl.ping(DXL_ID_2);

  // Turn off torque when configuring items in EEPROM area
  dxl.torqueOff(DXL_ID_1);
  dxl.torqueOff(DXL_ID_2);
  dxl.setOperatingMode(DXL_ID_1, OP_POSITION);
  dxl.setOperatingMode(DXL_ID_2, OP_POSITION);
  dxl.torqueOn(DXL_ID_1);
  dxl.torqueOn(DXL_ID_2);

  dxl.writeControlTableItem(MOVING_SPEED, DXL_ID_1, 50);
  dxl.writeControlTableItem(MOVING_SPEED, DXL_ID_2, 50);

  dxl.setGoalPosition(DXL_ID_1, counter_1, UNIT_DEGREE);
  dxl.setGoalPosition(DXL_ID_2, counter_2, UNIT_DEGREE);

  //nrf24l01
  radio.begin();
  radio.openReadingPipe(1, pipeIn);
  //radio.setAutoAck(false);
  //radio.setDataRate(RF24_250KBPS);
  //radio.setPALevel(RF24_PA_LOW);
  radio.startListening(); //  Set the module as receiver
  ResetData();
}

void loop() {
  // Check whether there is data to be received
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package)); // Read the whole data and store it into the 'data' structure
    lastReceiveTime = millis(); // At this moment, we have received the data
  }

  // Map the increments based on the values of data.roll
    float increment_1 = map(data.roll, 0, 255, -10, 10);
    float increment_2 = map(data.roll, 0, 255, 10, -10);

  // Adjust position for servo 1
  int pos_1 = dxl.getPresentPosition(DXL_ID_1, UNIT_DEGREE);
  pos_1 = constrain(pos_1 + increment_1, 45, 115);
  dxl.setGoalPosition(DXL_ID_1, pos_1, UNIT_DEGREE);

  // Adjust position for servo 2
  int pos_2 = dxl.getPresentPosition(DXL_ID_2, UNIT_DEGREE);
  pos_2 = constrain(pos_2 + increment_2, 25, 95);
  dxl.setGoalPosition(DXL_ID_2, pos_2, UNIT_DEGREE);

  Serial1.flush();
}
