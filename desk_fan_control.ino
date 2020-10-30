#include <OneWire.h>
#include <DallasTemperature.h>

const word PWM_FREQ_HZ = 25000;
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);

#define LOW_TEMP 30
#define HIGH_TEMP 50
#define OFF_FRONT_TEMP 20
#define OFF_BACK_TEMP 15

#define EMERGENCY_SPEED 0
#define FULLSPEED_SPEED 100
#define LOW_SPEED 30

#define ONE_WIRE_BUS 7

#define EMERGENCY_PIN 2
#define EMERGENCY_LED 4
#define FULLSPEED_PIN 3
#define FULLSPEED_LED 5

boolean emergency_mode = false;
boolean full_speed_mode = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {

  //FAN SETUP
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1 << COM1A1) |(1 << COM1B1)| (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;
  setFanspeed(LOW_SPEED, LOW_SPEED);

  digitalWrite(EMERGENCY_LED, HIGH);
  pinMode(EMERGENCY_PIN,INPUT_PULLUP);

  digitalWrite(FULLSPEED_LED, LOW);
  pinMode(FULLSPEED_PIN,INPUT_PULLUP);

  sensors.begin();

  // TEMP SETUP
  Serial.begin(9600);
}

void loop() {
  int front_fanspeed,back_fanspeed = 0;
  attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), enableEmergency, RISING);
  attachInterrupt(digitalPinToInterrupt(FULLSPEED_PIN), enableFullSpeed, RISING);
  digitalWrite(EMERGENCY_LED, LOW);
  
  if(emergency_mode){
    setFanspeed(EMERGENCY_SPEED,EMERGENCY_SPEED);
    delay(250);
    digitalWrite(EMERGENCY_LED, HIGH);
    delay(250);
  }
  else  {
    float temperature = measureTemp();
    Serial.print("Temperature: ");
    Serial.println(temperature);
  
    if(full_speed_mode){
      back_fanspeed = front_fanspeed = FULLSPEED_SPEED;
    }
    else{
      front_fanspeed = getFanSpeed(temperature, "front");
      back_fanspeed = getFanSpeed(temperature, "back");
      
    }
    setFanspeed(front_fanspeed,back_fanspeed);
    Serial.println();
    delay(2500);
  }
}

void setFanspeedFront(byte duty) {
  OCR1A = (word) (duty*TCNT1_TOP)/100;
}

void setFanspeedBack(byte duty) {
  OCR1B = (word) (duty*TCNT1_TOP)/100;
}

void setFanspeed(int speed_front, int speed_back){
  Serial.print("Fanspeed front: ");
  Serial.println(speed_front);
  setFanspeedFront(speed_front);
   
  Serial.print("Fanspeed back: ");
  Serial.println(speed_back);
  setFanspeedBack(speed_back);
}

int getFanSpeed(float temperature, String position){
  int fanspeed = LOW_SPEED;

  int OFF_TEMP = (position == "front" ? OFF_FRONT_TEMP : OFF_BACK_TEMP);
  
  if(temperature > HIGH_TEMP){
    fanspeed = 100;
  }
  else if(temperature > LOW_TEMP){
    fanspeed += ceil(((100-40) / (HIGH_TEMP - LOW_TEMP)) * (temperature - LOW_TEMP));      
  }
  else if(temperature < OFF_TEMP){
    fanspeed = 0;
  }

  return fanspeed;
}

float measureTemp(){
  sensors.requestTemperatures();   
  return sensors.getTempCByIndex(0);
}

void enableEmergency(){
  emergency_mode = !emergency_mode;
  full_speed_mode = false;
  digitalWrite(FULLSPEED_LED, LOW);
  
  digitalWrite(EMERGENCY_LED, (emergency_mode ? HIGH : LOW));
  setFanspeedFront((emergency_mode ? EMERGENCY_SPEED : LOW_SPEED));
}

void enableFullSpeed(){
  full_speed_mode = !full_speed_mode;
  emergency_mode = false;
  digitalWrite(EMERGENCY_LED, LOW);
  
  digitalWrite(FULLSPEED_LED, (full_speed_mode ? HIGH : LOW));
  setFanspeedFront((full_speed_mode ? FULLSPEED_SPEED : LOW_SPEED));
  setFanspeedBack((full_speed_mode ? FULLSPEED_SPEED : LOW_SPEED));
}
