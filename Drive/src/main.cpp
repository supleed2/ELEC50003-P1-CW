#include <Arduino.h>
#include <ArduinoJson.h>
// #include <string>
#include <Wire.h>
#include <INA219_WE.h>
#include "SPI.h"
#include <SoftwareSerial.h>

// #define RXpin 4 // Define your RX pin here
// #define TXpin 13 // Define your TX pin here

// SoftwareSerial mySerial(RXpin, TXpin);

bool debug = false;

//TO IMPLEMENT
//DONE 2 way serial
//DONE F<>,B<>,S,L<>,R<>,p<0--1023>
//DONE Obtain current and power usage, get voltage from analog pin
//request angle facing
//DONE speed control 0-1
//speed calibration, 0 stop and max speed to match
//distance travveled and x and y at request

//-------------------------------------------------------SMPS & MOTOR CODE START------------------------------------------------------//
INA219_WE ina219; // this is the instantiation of the library for the current sensor

float open_loop, closed_loop;                                        // Duty Cycles
float vpd, vb, vref, iL, dutyref, current_mA;                        // Measurement Variables
unsigned int sensorValue0, sensorValue1, sensorValue2, sensorValue3; // ADC sample values declaration
float ev = 0, cv = 0, ei = 0, oc = 0;                                //internal signals
float Ts = 0.0008;                                                   //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.
float kpv = 0.05024, kiv = 15.78, kdv = 0;                           // voltage pid.
float u0v, u1v, delta_uv, e0v, e1v, e2v;                             // u->output; e->error; 0->this time; 1->last time; 2->last last time
float kpi = 0.02512, kii = 39.4, kdi = 0;                            // current pid.
float u0i, u1i, delta_ui, e0i, e1i, e2i;                             // Internal values for the current controller
float uv_max = 4, uv_min = 0;                                        //anti-windup limitation
float ui_max = 1, ui_min = 0;                                        //anti-windup limitation
float current_limit = 1.0;
boolean Boost_mode = 0;
boolean CL_mode = 0;

unsigned int loopTrigger;
unsigned int com_count = 0; // a variables to count the interrupts. Used for program debugging.

//************************** Motor Constants **************************//
unsigned long previousMillis = 0; //initializing time counter
const long f_i = 10000;           //time to move in forward direction, please calculate the precision and conversion factor
const long r_i = 20000;           //time to rotate clockwise
const long b_i = 30000;           //time to move backwards
const long l_i = 40000;           //time to move anticlockwise
const long s_i = 50000;
int DIRRstate = LOW; //initializing direction states
int DIRLstate = HIGH;

int DIRL = 20; //defining left direction pin
int DIRR = 21; //defining right direction pin

int pwmr = 5; //pin to control right wheel speed using pwm
int pwml = 9; //pin to control left wheel speed using pwm
//*******************************************************************//
//-------------------------------------------------------SMPS & MOTOR CODE END------------------------------------------------------//

//-------------------------------------------------------OPTICAL SENSOR CODE START------------------------------------------------------//
#define PIN_SS 10
#define PIN_MISO 12
#define PIN_MOSI 11
#define PIN_SCK 13

#define PIN_MOUSECAM_RESET 8
#define PIN_MOUSECAM_CS 7

#define ADNS3080_PIXELS_X 30
#define ADNS3080_PIXELS_Y 30

#define ADNS3080_PRODUCT_ID 0x00
#define ADNS3080_REVISION_ID 0x01
#define ADNS3080_MOTION 0x02
#define ADNS3080_DELTA_X 0x03
#define ADNS3080_DELTA_Y 0x04
#define ADNS3080_SQUAL 0x05
#define ADNS3080_PIXEL_SUM 0x06
#define ADNS3080_MAXIMUM_PIXEL 0x07
#define ADNS3080_CONFIGURATION_BITS 0x0a
#define ADNS3080_EXTENDED_CONFIG 0x0b
#define ADNS3080_DATA_OUT_LOWER 0x0c
#define ADNS3080_DATA_OUT_UPPER 0x0d
#define ADNS3080_SHUTTER_LOWER 0x0e
#define ADNS3080_SHUTTER_UPPER 0x0f
#define ADNS3080_FRAME_PERIOD_LOWER 0x10
#define ADNS3080_FRAME_PERIOD_UPPER 0x11
#define ADNS3080_MOTION_CLEAR 0x12
#define ADNS3080_FRAME_CAPTURE 0x13
#define ADNS3080_SROM_ENABLE 0x14
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_LOWER 0x19
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_UPPER 0x1a
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_LOWER 0x1b
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_UPPER 0x1c
#define ADNS3080_SHUTTER_MAX_BOUND_LOWER 0x1e
#define ADNS3080_SHUTTER_MAX_BOUND_UPPER 0x1e
#define ADNS3080_SROM_ID 0x1f
#define ADNS3080_OBSERVATION 0x3d
#define ADNS3080_INVERSE_PRODUCT_ID 0x3f
#define ADNS3080_PIXEL_BURST 0x40
#define ADNS3080_MOTION_BURST 0x50
#define ADNS3080_SROM_LOAD 0x60

#define ADNS3080_PRODUCT_ID_VAL 0x17

int total_x = 0;
int total_y = 0;

int total_x1 = 0;
int total_y1 = 0;

int x = 0;
int y = 0;

int a = 0;
int b = 0;

int distance_x = 0;
int distance_y = 0;

int dist_to_move_prev_fl = 0;
int dist_to_move_prev_fr = 0;
int dist_to_move_acc_fl = 0;
int dist_to_move_acc_fr = 0;
unsigned long time_pid_prev_fl = 0;
unsigned long time_pid_prev_fr = 0;

int angle_to_move_prev = 0;
unsigned long time_pid_prev_ang = 0;

float kpdrive = 0.059;
float kddrive = 7.900;
float kidrive = 0.0000009;

float kpheading = 0.037;
float kdheading = 3.60;

volatile byte movementflag = 0;
volatile int xydat[2];

// FUNCTION DELCARATIONS //

float pidi(float pid_input);
float pidv(float pid_input);
void pwm_modulate(float pwm_input);
float saturation(float sat_input, float uplim, float lowlim);
void sampling();
void mousecam_write_reg(int reg, int val);
int mousecam_read_reg(int reg);
void mousecam_reset();
int getCurrentHeading();
float pid_ms(int dist_to_move, int *dist_to_move_prev, int *dist_to_move_acc, unsigned long *time_pid_prev, float kps, float kds, float kis);
float pid_rotation(int angle_to_move, int *angle_to_move_prev, unsigned long *time_pid_prev, float kps, float kds);

int convTwosComp(int b)
{
  //Convert from 2's complement
  if (b & 0x80)
  {
    b = -1 * ((b ^ 0xff) + 1);
  }
  return b;
}

int tdistance = 0;

void mousecam_reset(){
  digitalWrite(PIN_MOUSECAM_RESET, HIGH);
  delay(1); // reset pulse >10us
  digitalWrite(PIN_MOUSECAM_RESET, LOW);
  delay(35); // 35ms from reset to functional
}

int mousecam_init(){
  pinMode(PIN_MOUSECAM_RESET, OUTPUT);
  pinMode(PIN_MOUSECAM_CS, OUTPUT);

  digitalWrite(PIN_MOUSECAM_CS, HIGH);

  mousecam_reset();

  int pid = mousecam_read_reg(ADNS3080_PRODUCT_ID);
  if (pid != ADNS3080_PRODUCT_ID_VAL)
    return -1;

  // turn on sensitive mode
  mousecam_write_reg(ADNS3080_CONFIGURATION_BITS, 0x19);

  return 0;
}

void mousecam_write_reg(int reg, int val){
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(50);
}

int mousecam_read_reg(int reg){
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg);
  delayMicroseconds(75);
  int ret = SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(1);
  return ret;
}

struct MD{
  byte motion;
  char dx, dy;
  byte squal;
  word shutter;
  byte max_pix;
};

void mousecam_read_motion(struct MD *p){
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(ADNS3080_MOTION_BURST);
  delayMicroseconds(75);
  p->motion = SPI.transfer(0xff);
  p->dx = SPI.transfer(0xff);
  p->dy = SPI.transfer(0xff);
  p->squal = SPI.transfer(0xff);
  p->shutter = SPI.transfer(0xff) << 8;
  p->shutter |= SPI.transfer(0xff);
  p->max_pix = SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(5);
}

// pdata must point to an array of size ADNS3080_PIXELS_X x ADNS3080_PIXELS_Y
// you must call mousecam_reset() after this if you want to go back to normal operation
int mousecam_frame_capture(byte *pdata)
{
  mousecam_write_reg(ADNS3080_FRAME_CAPTURE, 0x83);

  digitalWrite(PIN_MOUSECAM_CS, LOW);

  SPI.transfer(ADNS3080_PIXEL_BURST);
  delayMicroseconds(50);

  int pix;
  byte started = 0;
  int count;
  int timeout = 0;
  int ret = 0;
  for (count = 0; count < ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y;)
  {
    pix = SPI.transfer(0xff);
    delayMicroseconds(10);
    if (started == 0)
    {
      if (pix & 0x40)
        started = 1;
      else
      {
        timeout++;
        if (timeout == 100)
        {
          ret = -1;
          break;
        }
      }
    }
    if (started == 1)
    {
      pdata[count++] = (pix & 0x3f) << 2; // scale to normal grayscale byte range
    }
  }

  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(14);

  return ret;
}

//-------------------------------------------------------OPTICAL SENSOR CODE END------------------------------------------------------//

//Tracker Variables
int current_x = 0;
int current_y = 0;
int goal_x = 0;
int goal_y = 0;
int distanceGoal;
bool commandComplete = 1;
float powerUsage_mWh = 0;
int distTravelled_mm = 0;
bool initialAngleSet = false;

//calibration varibles
int leftStart = 80;   //pwm min for left motor
int leftStop = 255;   //pwm max for left motor
int rightStart = 80;  //pwm min for right motor
int rightStop = 255;  //pwm max for right motor

//Energy Usage Variables
unsigned long previousMillis_Energy = 0; // will store last time energy use was updated
const long interval_Energy = 1000;       //energy usaged update frequency
float totalEnergyUsed = 0;
float powerUsed = 0;
int loopCount = 0;
float motorVoltage = 0;

int getPWMfromSpeed(float speedr, bool left)
{
  if (speedr >= 1)
  {
    return 512;
  }
  else if (speedr < 0)
  {
    return 0;
  }
  else
  {
    int speedpercentage = (speedr * 100);
    if (left)
    {
      return map(speedpercentage, 0, 100, leftStart, leftStop);
    }
    else
    {
      return map(speedpercentage, 0, 100, rightStart, rightStop);
    }
  }
}

void setup()
{
  //-------------------------------------------------------SMPS & MOTOR CODE START------------------------------------------------------//
  //************************** Motor Pins Defining **************************//
  pinMode(DIRR, OUTPUT);
  pinMode(DIRL, OUTPUT);
  pinMode(pwmr, OUTPUT);
  pinMode(pwml, OUTPUT);
  digitalWrite(pwmr, HIGH); //setting right motor speed at maximum
  digitalWrite(pwml, HIGH); //setting left motor speed at maximum
  //*******************************************************************//

  //Basic pin setups

  noInterrupts();            //disable all interrupts
  pinMode(13, OUTPUT);       //Pin13 is used to time the loops of the controller
  pinMode(3, INPUT_PULLUP);  //Pin3 is the input from the Buck/Boost switch
  pinMode(2, INPUT_PULLUP);  // Pin 2 is the input from the CL/OL switch
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  // TimerA0 initialization for control-loop interrupt.

  TCA0.SINGLE.PER = 999;                                                 //
  TCA0.SINGLE.CMP1 = 999;                                                //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output

  pinMode(6, OUTPUT);
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz
  analogWrite(6, 120);

  interrupts();          //enable interrupts.
  Wire.begin();          // We need this for the i2c comms for the current sensor
  ina219.init();         // this initiates the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  //-------------------------------------------------------SMPS & MOTOR CODE END------------------------------------------------------//
  Serial.begin(115200); // Set up hardware UART0 (Connected to USB port)
  Serial1.begin(9600);  // Set up hardware UART

  //Serial.println(getPWMfromSpeed(-1));
  //Serial.println(getPWMfromSpeed(256));
  //Serial.println(getPWMfromSpeed(0.5));
  // Other Drive setup stuff
  /////////currentHeading = REQUEST HEADING HERE;

  analogWrite(pwmr, 0);
  analogWrite(pwml, 0);
  //digitalWrite(DIRR, LOW);
  //digitalWrite(DIRL, HIGH);

  pinMode(PIN_SS, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);

  if (mousecam_init() == -1)
  {
    Serial.println("Mouse cam failed to init");
    while (1)
      ;
  }
}

int commandCompletionStatus = 0; //0-No Command, 1-New Command, 2-Command being run, 3-Command Complete
int requiredHeading = 0;
int distance = 0;
float spd = 0;
int currentHeading = 0;
//reset variables for update on completion

unsigned long previousMillis_Command = 0;
const long interval_Command = 1000;
DeserializationError error;

char asciiart(int k)
{
  static char foo[] = "WX86*3I>!;~:,`. ";
  return foo[k >> 4];
}

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y];
DynamicJsonDocument rdoc(1024);

void loop()
{
  if (Serial1.available() && ((commandCompletionStatus == 0)||(commandCompletionStatus == 2))){
    // receive doc, not sure how big this needs to be
    error = deserializeJson(rdoc, Serial1);

    //Serial.println("Got serial");

    // Test if parsing succeeds.
    if (error)
    {
      //Serial.print(F("deserializeJson() failed: "));
      //Serial.println(error.f_str());
      return;
    }
    else
    {
      if(rdoc.containsKey("sp") && rdoc["rH"] != -1){
        //parsing success, prepare command and pull request information
        commandCompletionStatus = 1;
        requiredHeading = rdoc["rH"];
        distance = rdoc["dist"];
        spd = rdoc["sp"];
        currentHeading = int(rdoc["cH"]) + 180;

        Serial.println("rH = " + String(requiredHeading) + " dist = " + String(distance) + " speed = " + String(spd));

        //reset variables for update on completion
        commandComplete = 0;
        powerUsage_mWh = 0.0;
        dist_to_move_acc_fl = 0;
        dist_to_move_acc_fr = 0;
      }else if (rdoc.containsKey("cH")){
        currentHeading = 180 + int(rdoc["cH"]);
        // Serial.println(currentHeading);
      }else if (rdoc.containsKey("stp") && rdoc["stp"] == 1){
        digitalWrite(pwmr, LOW);
        digitalWrite(pwml, LOW);
        commandCompletionStatus = 3;
      }else if (rdoc.containsKey("rstD") && rdoc["rstD"] == 1){
        goal_x = 0;
        goal_y = 0;
        distTravelled_mm = 0;
      }
    }
  }

  //if(current_x!=goal_x)

  // Do Drive stuff, set the 5 values above

  if (commandCompletionStatus == 0)
  { //noCommand
    //Do Nothing just wait
    //Serial.println("status0");
  }
  if (commandCompletionStatus == 1)
  { //Serial.println("status1");
    //newCommand
    //set goals
    goal_x += distance * sin(requiredHeading);
    goal_y += distance * cos(requiredHeading);
    total_y = 0;
    total_x = 0;
    commandCompletionStatus = 2;

    initialAngleSet = false;
  }
  if (commandCompletionStatus == 2)
  { //Serial.println("status2");
    //ongoingCommand
    //start moving towards goal

    //set angle first
    if (!initialAngleSet)
    {
      //turn to angle
      if (currentHeading < requiredHeading)
      { //turn right
        //Serial.println("turning right");
        //Serial.println(currentHeading);
        float spd_pid = pid_rotation(abs(currentHeading - requiredHeading), &angle_to_move_prev, &time_pid_prev_ang, kpheading, kdheading);
        analogWrite(pwmr, getPWMfromSpeed(spd_pid, false));
        analogWrite(pwml, getPWMfromSpeed(spd_pid, true));
        digitalWrite(DIRR, LOW);
        digitalWrite(DIRL, LOW);
      }
      else if (currentHeading > requiredHeading)
      { //turn left
        //Serial.println("turning left");
        //Serial.println(currentHeading);
        float spd_pid = pid_rotation(abs(currentHeading - requiredHeading), &angle_to_move_prev, &time_pid_prev_ang, kpheading, kdheading);
        analogWrite(pwmr, getPWMfromSpeed(spd_pid, false));
        analogWrite(pwml, getPWMfromSpeed(spd_pid, true));
        digitalWrite(DIRR, HIGH);
        digitalWrite(DIRL, HIGH);
      }
      else
      {
        //heading correct therefore move to next step...
        //STOP!!!!
        digitalWrite(pwmr, LOW);
        digitalWrite(pwml, LOW);

        initialAngleSet = true;
      }
    }
    else
    { //then move forwards but check angle for drift using optical flow
      if (total_y - distance < 0)
      { //go forwards
        //Serial.println("going forwards");
        float speed_r = pid_ms(abs(total_y - distance), &dist_to_move_prev_fr, &dist_to_move_acc_fr, &time_pid_prev_fr, kpdrive, kddrive, kidrive);
        float speed_l = pid_ms(abs(total_y - distance), &dist_to_move_prev_fl, &dist_to_move_acc_fl, &time_pid_prev_fl, kpdrive, kddrive, kidrive);
        analogWrite(pwmr, getPWMfromSpeed(speed_r, false));
        analogWrite(pwml, getPWMfromSpeed(speed_l, true));
        digitalWrite(DIRR, LOW);
        digitalWrite(DIRL, HIGH);
      }
      else if (total_y - distance > 0)
      { //go backwards
        //Serial.println("going backwards");
        float speed_r = pid_ms(abs(total_y - distance), &dist_to_move_prev_fr, &dist_to_move_acc_fr, &time_pid_prev_fr, kpdrive, kddrive, kidrive);
        float speed_l = pid_ms(abs(total_y - distance), &dist_to_move_prev_fl, &dist_to_move_acc_fl, &time_pid_prev_fl, kpdrive, kddrive, kidrive);
        analogWrite(pwmr, getPWMfromSpeed(speed_r, false));
        analogWrite(pwml, getPWMfromSpeed(speed_l, true));
        digitalWrite(DIRR, HIGH);
        digitalWrite(DIRL, LOW);
      }
      else if ((total_y == distance))
      { //distance met
        //STOP!!!!!
        digitalWrite(pwmr, LOW);
        digitalWrite(pwml, LOW);
        commandCompletionStatus = 3;
        initialAngleSet = true;
      }
    }
  }
  if (commandCompletionStatus == 3)
  { // Serial.println("status3");
    //currentPosMatchesOrExceedsRequest
    ///finish moving

    //send update via UART

    //prepare feedback variables
    commandComplete = true;
    current_x = goal_x;
    current_y = goal_y;
    distTravelled_mm += abs(distance);

    //compile energy use
    unsigned long currentMillis_Energy = millis();

    totalEnergyUsed += (currentMillis_Energy - previousMillis_Energy) * (powerUsed / loopCount) / 1000 / (60 * 60);
    previousMillis_Energy = currentMillis_Energy;

    if (debug){
      Serial.print(motorVoltage);
      Serial.print("Energy Used: ");
      Serial.print(totalEnergyUsed);
      Serial.println("mWh");
    }

    loopCount = 0; //reset counter to zero
    powerUsed = 0; //reset power usage
    powerUsage_mWh = totalEnergyUsed;
    totalEnergyUsed = 0;
    total_x1 = 0;
    total_y1 = 0;

    DynamicJsonDocument tdoc(1024); // transmit doc, not sure how big this needs to be
    tdoc["comp"] = commandComplete;
    tdoc["mWh"] = powerUsage_mWh;
    tdoc["mm"] = distTravelled_mm;
    tdoc["pos"][0] = current_x;
    tdoc["pos"][1] = current_y;
    tdoc["bV"] = vb;
    serializeJson(tdoc, Serial1); // Build JSON and send on UART1
    serializeJson(tdoc, Serial); // Build JSON and send on UART1
    commandCompletionStatus = 0;
  }

  //Handle power usage
  //find motor voltage
  //int motorVSensor = analogRead(A5);
  //float motorVoltage = motorVSensor * (5.0 / 1023.0);
  float motorVoltage = vb;

  //find average power

  if (current_mA >= 0){
    powerUsed += current_mA * motorVoltage;
  }
  if (debug){
    Serial.println(powerUsed);
  }

  //calculate averages for energy use calculations
  loopCount += 1; //handle loop quantity for averaging

  //find average current
  //find average voltage

  //update command/control

  if (movementflag){

    tdistance = tdistance + convTwosComp(xydat[0]);
    // Serial.println("Distance = " + String(tdistance));
    movementflag = 0;
    delay(3);
  }
  int val = mousecam_read_reg(ADNS3080_PIXEL_SUM);
  MD md;
  mousecam_read_motion(&md);
  /* for (int i = 0; i < md.squal / 4; i++)
    Serial.print('*');
  Serial.print(' ');
  Serial.print((val * 100) / 351);
  Serial.print(' ');
  Serial.print(md.shutter);
  Serial.print(" (");
  Serial.print((int)md.dx);
  Serial.print(',');
  Serial.print((int)md.dy);
  Serial.println(')'); */

  // Serial.println(md.max_pix);
  delay(50);

  distance_x = md.dx; //convTwosComp(md.dx);
  distance_y = md.dy; //convTwosComp(md.dy);

  total_x1 = (total_x1 + distance_x);
  total_y1 = (total_y1 + distance_y);

  total_x = 10 * total_x1 / 157; //Conversion from counts per inch to mm (400 counts per inch)
  total_y = 10 * total_y1 / 157; //Conversion from counts per inch to mm (400 counts per inch)

/*   Serial.print('\n');

  Serial.println("Distance_x = " + String(total_x));

  Serial.println("Distance_y = " + String(total_y));
  Serial.print('\n'); */
  //-------------------------------------------------------SMPS & MOTOR CODE START------------------------------------------------------//
  unsigned long currentMillis = millis();
  if (loopTrigger)
  { // This loop is triggered, it wont run unless there is an interrupt

    digitalWrite(13, HIGH); // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.

    // Sample all of the measurements and check which control mode we are in
    sampling();
    CL_mode = digitalRead(3);    // input from the OL_CL switch
    Boost_mode = digitalRead(2); // input from the Buck_Boost switch

    if (Boost_mode)
    {
      if (CL_mode)
      {                  //Closed Loop Boost
        pwm_modulate(1); // This disables the Boost as we are not using this mode
      }
      else
      {                  // Open Loop Boost
        pwm_modulate(1); // This disables the Boost as we are not using this mode
      }
    }
    else
    {
      if (CL_mode)
      { // Closed Loop Buck
        // The closed loop path has a voltage controller cascaded with a current controller. The voltage controller
        // creates a current demand based upon the voltage error. This demand is saturated to give current limiting.
        // The current loop then gives a duty cycle demand based upon the error between demanded current and measured
        // current
        current_limit = 3;                                 // Buck has a higher current limit
        ev = vref - vb;                                    //voltage error at this time
        cv = pidv(ev);                                     //voltage pid
        cv = saturation(cv, current_limit, 0);             //current demand saturation
        ei = cv - iL;                                      //current error
        closed_loop = pidi(ei);                            //current pid
        closed_loop = saturation(closed_loop, 0.99, 0.01); //duty_cycle saturation
        pwm_modulate(closed_loop);                         //pwm modulation
      }
      else
      {                          // Open Loop Buck
        current_limit = 3;       // Buck has a higher current limit
        oc = iL - current_limit; // Calculate the difference between current measurement and current limit
        if (oc > 0)
        {
          open_loop = open_loop - 0.001; // We are above the current limit so less duty cycle
        }
        else
        {
          open_loop = open_loop + 0.001; // We are below the current limit so more duty cycle
        }
        open_loop = saturation(open_loop, dutyref, 0.02); // saturate the duty cycle at the reference or a min of 0.01
        pwm_modulate(open_loop);                          // and send it out
      }
    }
    // closed loop control path

    digitalWrite(13, LOW); // reset pin13.
    loopTrigger = 0;
  }
}

// Timer A CMP1 interrupt. Every 800us the program enters this interrupt.
// This, clears the incoming interrupt flag and triggers the main loop.

ISR(TCA0_CMP1_vect){
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loopTrigger = 1;
}

// This subroutine processes all of the analogue samples, creating the required values for the main loop

void sampling(){

  // Make the initial sampling operations for the circuit measurements

  sensorValue0 = analogRead(A0);       //sample Vb
  sensorValue2 = analogRead(A2);       //sample Vref
  sensorValue3 = analogRead(A3);       //sample Vpd
  current_mA = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)

  // Process the values so they are a bit more usable/readable
  // The analogRead process gives a value between 0 and 1023
  // representing a voltage between 0 and the analogue reference which is 4.096V

  vb = sensorValue0 * (4.94 / 1023.0);   // Convert the Vb sensor reading to volts
  vref = sensorValue2 * (4.096 / 1023.0); // Convert the Vref sensor reading to volts
  vpd = sensorValue3 * (4.096 / 1023.0);  // Convert the Vpd sensor reading to volts


  // The inductor current is in mA from the sensor so we need to convert to amps.
  // We want to treat it as an input current in the Boost, so its also inverted
  // For open loop control the duty cycle reference is calculated from the sensor
  // differently from the Vref, this time scaled between zero and 1.
  // The boost duty cycle needs to be saturated with a 0.33 minimum to prevent high output voltages

  if (Boost_mode == 1){
    iL = -current_mA / 1000.0;
    dutyref = saturation(sensorValue2 * (1.0 / 1023.0), 0.99, 0.33);
  }else{
    iL = current_mA / 1000.0;
    dutyref = sensorValue2 * (1.0 / 1023.0);
  }
}

float saturation(float sat_input, float uplim, float lowlim){ // Saturation function
  if (sat_input > uplim)
    sat_input = uplim;
  else if (sat_input < lowlim)
    sat_input = lowlim;
  else
    ;
  return sat_input;
}

void pwm_modulate(float pwm_input){ // PWM function
  analogWrite(6, (int)(255 - pwm_input * 255));
}

// This is a PID controller for the voltage

float pidv(float pid_input){
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;

  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if (u1v >= uv_max){
    e_integration = 0;
  }else if (u1v <= uv_min){
    e_integration = 0;
  }

  delta_uv = kpv * (e0v - e1v) + kiv * Ts * e_integration + kdv / Ts * (e0v - 2 * e1v + e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;                                                                       //this time's control output

  //output limitation
  saturation(u0v, uv_max, uv_min);

  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

// This is a PID controller for the current

float pidi(float pid_input){
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max){
    e_integration = 0;
  }
  else if (u1i <= ui_min){
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;                                                                       //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}

// This is a P!ID contrller for motor speed

float pid_ms(int dist_to_move, int *dist_to_move_prev, int *dist_to_move_acc, unsigned long *time_pid_prev, float kps, float kds, float kis){
  
  int T_diff = millis() - *time_pid_prev;
  *dist_to_move_acc = *dist_to_move_acc + dist_to_move;
  float speed = (kps * dist_to_move) + ((kds/T_diff) * (dist_to_move - *dist_to_move_prev)) + ((kis*T_diff) * (*dist_to_move_acc));
  *time_pid_prev = millis();

  Serial.println(speed);

  if (speed >= 1) speed = 1;
  else if (speed <= 0.55) speed = 0.55;

  *dist_to_move_prev = dist_to_move;
  return speed;
}

float pid_rotation(int angle_to_move, int *angle_to_move_prev, unsigned long *time_pid_prev, float kps, float kds){
  int T_diff = millis() - *time_pid_prev;
  float speed = (kps * angle_to_move) + ((kds/T_diff) * (angle_to_move - *angle_to_move_prev));
  *time_pid_prev = millis();

  Serial.println(speed);

  if (speed >= 1) speed = 1;
  else if (speed <= 0.85) speed = 0.85;

  *angle_to_move_prev = angle_to_move;
  return speed;
}