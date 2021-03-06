/**
 * Remember to install Adafruit ADXL345, HMC5883, Wire and SoftwareServo libraries through Arduino IDE before compiling.
 * The other libraries can be installed by Library Manager directly from the Internet, but SoftwareServo library you have to install from a ZIP file available at: http://playground.arduino.cc/ComponentLib/Servo
 * In the SoftwareServo.h file you might have to change WProgram.h include to Arduino.h header.
 * Remember to select correct board type, such as Arduino Nano.
 */
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_HMC5883_U.h>
#include <SoftwareServo.h>

#define HEADING_PWM 12       // PWM output to servo
#define TILT_PWM 9
#define SENSOR_INTERVAL 50
#define SERVO_TURN_DURATION 15
#define SERVO_TILT_DURATION 50
#define SERVO_CENTER 92
#define SERVO_LEFT 102
#define SERVO_RIGHT 82
#define SERVO_UP 82
#define SERVO_DOWN 102 // +50
#undef USE_SERVO
#define MAG_TEST 1

/*
  Arduino       GY-85
  A5            SCL
  A4            SDA
  VCC           VCC
  GND           GND
*/

Adafruit_ADXL345_Unified acc = Adafruit_ADXL345_Unified(12345);
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12346);
//unsigned long lastSensorTime = 0;
float tiltDegrees = 20, headingDegrees = 0;
SoftwareServo tiltServo, headingServo;

void setup()
{
  #ifdef USE_SERVO
  tiltServo.attach(TILT_PWM);
  tiltServo.write(SERVO_CENTER);
  //pinMode(TILT_PWM, INPUT);
  headingServo.attach(HEADING_PWM);
  headingServo.write(SERVO_CENTER);
  //pinMode(HEADING_PWM, INPUT);
  #endif
  
  Serial.begin(9600);
  
  while (!acc.begin())
  {
    //Serial.println("ADXL345 not detected, retrying...");
    delay(100);
  }
  
  while (!mag.begin())
  {
    //Serial.println("HMC5883 not detected, retrying...");
    delay(100);
  }
  //mag.setMagGain(HMC5883_MAGGAIN_1_3);
}

void loop() 
{
  // Read serial port and when two lines received, process them as keps data
  if (Serial.available())
  {
    char chr = Serial.read();
    if (chr=='u') turnUp();
    else if (chr=='d') turnDown();
    else if (chr=='l') turnLeft();
    else if (chr=='r') turnRight();
    else if (chr=='?') reportPosition();
  }

#ifdef MAG_TEST
  reportPosition();
  delay(100);
#endif

  //if ((millis() - lastSensorTime) >= SENSOR_INTERVAL)
#ifdef USE_SERVO
  SoftwareServo::refresh();
#endif
}

float getCompassCourse(float ax, float ay, float az, float cx, float cz, float cy)
{
  float xh,yh,ayf,axf;
  ayf=ay/57.0;//Convert to rad
  axf=ax/57.0;//Convert to rad
  xh=cx*cos(ayf)+cy*sin(ayf)*sin(axf)-cz*cos(axf)*sin(ayf);
  yh=cy*cos(axf)+cz*sin(axf);
 
  float var_compass=atan2((double)yh,(double)xh) * (180 / PI) -90; // angle in degrees
  if (var_compass>0){var_compass=var_compass-360;}
  var_compass=360+var_compass;
 
  return (var_compass);
}

void reportPosition()
{
  //lastSensorTime = millis();
    
  sensors_event_t accelEvent;
  sensors_event_t magEvent;
  acc.getEvent(&accelEvent);
  mag.getEvent(&magEvent);
  
  // Calculate tilt - 0 degrees = towards horizon, 90 degrees = towards sky
  tiltDegrees = accelEvent.acceleration.x * 9.0f;
  // Calculate cosine and sine of tilt for later purposes
  float cos_pitch = cos(-tiltDegrees * M_PI / 180);
  float sin_pitch = sin(-tiltDegrees * M_PI / 180);
  // Calculate heading towards north taking tilt into account
  float mag_x = magEvent.magnetic.x * cos_pitch + magEvent.magnetic.z * sin_pitch;
  //float mag_x = magEvent.magnetic.x;
  float mag_y = magEvent.magnetic.y;
  float heading = atan2(-mag_y, mag_x); // + DECLANATION;
  // Calculate degrees of tilt (0-360)
  //if (heading < 0) heading += 2 * PI;
 // if (heading > 2 * PI) heading -= 2 * PI;
  headingDegrees = heading * 180 / M_PI;
  // xyz, xzy, yxz, zxy, zyx
  headingDegrees = getCompassCourse(accelEvent.acceleration.x, accelEvent.acceleration.y, 0,
    magEvent.magnetic.x, magEvent.magnetic.y, magEvent.magnetic.z);
  /*float tiltRad = tiltDegrees * M_PI / 180;
  float xh = magEvent.magnetic.x * cos(tiltRad) + magEvent.magnetic.y * sin(tiltRad) - magEvent.magnetic.z*cos(tiltRad) * sin(tiltRad);
  float yh = magEvent.magnetic.y * cos(tiltRad) - magEvent.magnetic.z * sin(tiltRad);
  headingDegrees = atan(yh/xh) * 180 / M_PI;
if    (xh<0)          headingDegrees=180-headingDegrees;
else if(xh>0  && yh<0) headingDegrees=-headingDegrees;
else if(xh>0  && yh>0) headingDegrees=360-headingDegrees;
else if(xh==0 && yh<0) headingDegrees=90;
else if(xh==0 && yh>0) headingDegrees=270;
  while (headingDegrees < 0) headingDegrees += 360;
  while (headingDegrees >= 360) headingDegrees -= 360;*/

  Serial.print(headingDegrees);
  Serial.print(" ");
  Serial.println(tiltDegrees);
}

void turnUp()
{
  // Can't be too close to 90 or the antenna might flip over
  if (tiltDegrees >= 80) return;
  //Serial.println("Turning up");
  //pinMode(TILT_PWM, OUTPUT);
  tiltServo.write(SERVO_UP);
  servoDelay(SERVO_TILT_DURATION);
  tiltServo.write(SERVO_CENTER);
  //pinMode(TILT_PWM, INPUT);
}

void turnDown()
{
  // No point in turning below the horizon level
  if (tiltDegrees <= 0) return;
  //Serial.println("Turning down");
  //pinMode(TILT_PWM, OUTPUT);
  tiltServo.write(SERVO_DOWN);
  servoDelay(SERVO_TILT_DURATION);
  tiltServo.write(SERVO_CENTER);
  //pinMode(TILT_PWM, INPUT);
}

void turnLeft()
{
  //Serial.println("Turning left");
  //pinMode(HEADING_PWM, OUTPUT);
  headingServo.write(SERVO_LEFT);
  servoDelay(SERVO_TURN_DURATION);
  headingServo.write(SERVO_CENTER);
  //pinMode(HEADING_PWM, INPUT);
}

void turnRight()
{
  //Serial.println("Turning right");
  //pinMode(HEADING_PWM, OUTPUT);
  headingServo.write(SERVO_RIGHT);
  servoDelay(SERVO_TURN_DURATION);
  headingServo.write(SERVO_CENTER);
  //pinMode(HEADING_PWM, INPUT);
}

void servoDelay(int amount)
{
  unsigned long start = millis();
  while (millis() - start < amount) SoftwareServo::refresh();
}

