#include <OneWire.h>
#include <DallasTemperature.h>

const word PWM_FREQ_HZ = 25000;
const word TCNT1_TOP = 16000000 / (2 * PWM_FREQ_HZ);

#define ULTRALOW_TEMP 21
#define LOW_TEMP 24
#define RAMP_TEMP 26
#define HIGH_TEMP 28
#define OFF_FRONT_TEMP 15
#define OFF_BACK_TEMP 10
#define BACK_ON_OFFSET 2

#define EMERGENCY_SPEED 0
#define LOW_SPEED 50
#define ULTRALOW_SPEED 30
#define RAMP_SPEED 70

#define ONE_WIRE_BUS 7

#define EMERGENCY_PIN 2
#define EMERGENCY_LED 4
#define FULLSPEED_PIN 3
#define FULLSPEED_LED 5

boolean emergency_mode = false;
boolean full_speed_mode = false;
boolean fan_front_off = false;
boolean fan_back_off = false;
boolean ramped = true;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {

  //FAN SETUP
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;

  setFanspeed(LOW_SPEED);

  // Setup our buttons and leds
  digitalWrite(EMERGENCY_LED, LOW);
  pinMode(EMERGENCY_PIN, INPUT_PULLUP);
  digitalWrite(FULLSPEED_LED, LOW);
  pinMode(FULLSPEED_PIN, INPUT_PULLUP);

  sensors.begin();
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), switchEmergency, RISING);
  attachInterrupt(digitalPinToInterrupt(FULLSPEED_PIN), switchFullSpeed, RISING);
}

void loop() {
  int front_fanspeed, back_fanspeed = 0;
  //attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), emergencyMode,RISING);
  //attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), emergencyModeOff,LOW);
  

  float temperature = measureTemp();
  Serial.print("Temperature: ");
  Serial.println(temperature);


  if (emergency_mode) {
    setFanspeed(EMERGENCY_SPEED,EMERGENCY_SPEED);
    delay(250);
    digitalWrite(EMERGENCY_LED, HIGH);
    delay(250);
    digitalWrite(EMERGENCY_LED, LOW);

    Serial.println("EMERGENCY STOP");
    Serial.println();
  }
  else if (full_speed_mode) {
    setFanspeed(100);
    delay(250);
    digitalWrite(FULLSPEED_LED, HIGH);
    delay(250);
    digitalWrite(FULLSPEED_LED, LOW);
    Serial.println("FULL SPEED");
    Serial.println();
  }
  else  {
    front_fanspeed = getFanSpeed(temperature, "front");
    back_fanspeed = getFanSpeed(temperature, "back");

    if(ramped) digitalWrite(FULLSPEED_LED, HIGH);
    else digitalWrite(FULLSPEED_LED, LOW);

    Serial.print("Speed front: ");
    Serial.print(front_fanspeed);
    Serial.print(" Speed back: ");
    Serial.println(back_fanspeed);

    Serial.print("Ramped: ");
    Serial.println(ramped);

    setFanspeed(front_fanspeed, back_fanspeed);
    Serial.println();
    delay((ramped ? 10000 : 20000));
  }
  
}

void setFanspeedFront(byte duty) {
  OCR1A = (word) (duty * TCNT1_TOP) / 100;
}

void setFanspeedBack(byte duty) {
  OCR1B = (word) (duty * TCNT1_TOP) / 100;
}

void setFanspeed(int fan_speed) {
  setFanspeed(fan_speed, fan_speed);
}

void setFanspeed(int speed_front, int speed_back) {
  setFanspeedFront(speed_front);
  setFanspeedBack(speed_back);
}

int getFanSpeed(float temperature, String position) {
  int fanspeed = (temperature < ULTRALOW_TEMP ? ULTRALOW_SPEED : LOW_SPEED);
  
  int OFF_TEMP = (position == "front" ? OFF_FRONT_TEMP : OFF_BACK_TEMP);  
  
  if(ramped && temperature < LOW_TEMP) ramped = false;

  if (temperature > HIGH_TEMP) {
    return 100;
  }
  else if (temperature >= RAMP_TEMP || ramped) {
    fanspeed = RAMP_SPEED;
    fanspeed += ceil(((100 - RAMP_SPEED) / (HIGH_TEMP - RAMP_TEMP)) * (ceil(temperature) - RAMP_TEMP));
    ramped = true;
    if(fanspeed < RAMP_SPEED) fanspeed = RAMP_SPEED;
  }

  if (temperature < OFF_TEMP) {
    fanspeed = 0;
    if (position == "front") fan_front_off = true;
    else fan_back_off = true;
  }

  if (position == "front")  {
    if (fan_front_off && temperature < (OFF_TEMP + BACK_ON_OFFSET)) {
      fanspeed = 0;
    }
    else fan_front_off = false;
  }
  else if (position == "back")  {    
    if (fan_back_off && temperature < (OFF_TEMP + BACK_ON_OFFSET)) {
      fanspeed = 0;
    }
    else{
      //fanspeed += 20;
      fan_back_off = false;
    }
  }

  if (fanspeed > 100) fanspeed = 100;
  else if (fanspeed < 0) fanspeed = 0;

  return fanspeed;
}

float measureTemp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void emergencyMode(){
  setFanspeed(0);
  emergency_mode= true;
}
void emergencyModeOff(){
  emergency_mode= false;
}

void switchEmergency() {
  emergency_mode = !emergency_mode;
  full_speed_mode = false;
  digitalWrite(FULLSPEED_LED, LOW);

  digitalWrite(EMERGENCY_LED, (emergency_mode ? HIGH : LOW));
  setFanspeed((emergency_mode ? EMERGENCY_SPEED : LOW_SPEED));
}

void switchFullSpeed() {
  full_speed_mode = !full_speed_mode;
  emergency_mode = false;
  digitalWrite(EMERGENCY_LED, LOW);

  digitalWrite(FULLSPEED_LED, (full_speed_mode ? HIGH : LOW));
  setFanspeed((full_speed_mode ? 100 : LOW_SPEED));
}
