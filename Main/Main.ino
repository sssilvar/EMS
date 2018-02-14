#include <avr/sleep.h>
#include "PWMFrequency.h"
#include "Wire.h"


/*
 * VARIABLES AND OBJECTS
 */
//HMI pins definition
#define BUT       B11111110 // Pin B0 for button
# define BUT_VAL  PINB & B00000001 // Value button

#define LED_R 13      // Pin D13 for LEDR (~)
#define LED_G 5       // Pin D5 for LEDG (~)
#define LED_B 10      // Pin D10 for LEDB (~)

//Signal inverter pins
#define BRIGDE_EN 2   // Bridge enable (D2)
#define POS_SIG   8   // Positive set signal (D8)
#define NEG_SIG   12  // Negative set signal (D12)

#define AMP_CTRL  3   // Amplitude Control PWM (D3)
#define TIME_STEP 15  // Change step animation [ms]

// Session defines
#define RISING_TIME 5000  //Rising time in ms
#define STEADY_TIME 10000  //Rising time in ms
#define FALLING_TIME 5000  //Rising time in ms
#define D_MIN 0.58
#define D_MAX 0.97

// Intensity variables
byte intensity = 30;

// UI variables
int function      = 1;
int pressed_time  = 0;
int released_time = 0;
int delta_time    = 0;

long int rising_time_counter = (int)RISING_TIME/0.3;
long int steady_time_counter = (int)STEADY_TIME/0.3;
long int falling_time_counter = (int)FALLING_TIME/0.3;

// Communication variables (PROVISIONAL)
int id = 0;
int max_amplitude = 200;   //Amplitude max in duty (on a 500Ohm load)

/*
 * Setup Section
 */
void setup() {
  //IOs definitions
    //HMI pins
    DDRB = DDRB & BUT;  // Button as input
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    //Signal Inversion Pins
    pinMode(BRIGDE_EN, OUTPUT);
    pinMode(POS_SIG, OUTPUT);
    pinMode(NEG_SIG, OUTPUT);
    pinMode(AMP_CTRL, OUTPUT);

  //Initialization
    //HMI
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);
    //Signal
    digitalWrite(BRIGDE_EN, LOW);
    digitalWrite(POS_SIG, LOW);
    digitalWrite(NEG_SIG, LOW);
    analogWrite(AMP_CTRL, 0);
    delay(100);

    // Potentiometer set first intensity
    Wire.begin(); // join i2c bus (address optional for master)
}

void loop() {
  // Start I2C communication (POTENCIOMETER)
  delay(300);
  Wire.beginTransmission(48); // transmit to device
  Wire.write(byte(0x00));            // sends instruction byte
  Wire.write(intensity);             // sends potentiometer value byte
  delay(200);
  Wire.endTransmission();     // stop transmitting

  // Start Interface
  while(!readButton()); // IDLE State
  waitForOnSignal(); // Start LOOP
}


/*
 * 
 * SET OF FUNCTIONS OR COMMANDS
 * 
 */
// Execute USB Mode
void executeUSBMode(){
  nBlink(function);
  int i;
  for(i=0; i <=255; i++){
    setColor(0,i,255);
    delay(10);
  }
  while(readButton()){
    i++;
    int val = (int)(155+100*cos(0.01*i));
    setColor(0.005*val, val, val);
    delay(10);
  }

  setColor(255,0,0);
  delay(1000);
  function =3;
  executePowerOff();

  //TODO: Fernando's part
}

// Execute Session Mode
void executeSessionMode(){
  nBlink(function);
  long int i;
  for(i=0; i <=255; i++){
    setColor(0,255-i,0);
    delay(25);
  }
  i = 0;
  while(readButton()){
    setColor(0,0,100); //test
    setAmplitude(255);
    while(i <= steady_time_counter && readButton()){
      // Signal Generation
      setSignalPositive();
      delayMicroseconds(100);
      turnSignalOff();
      delayMicroseconds(100);
      setSignalNegative();
      delayMicroseconds(100);
      turnSignalOff();
      delayMicroseconds(100);
      i++;
    }
    i = 0; // reset counter
/*
    setAmplitude(max_amplitude);
    while(i <= steady_time_counter && readButton()){
      setColor(0,200,0); //test
      // Signal Generation
      setSignalPositive();
      delayMicroseconds(100);
      turnSignalOff();
      delayMicroseconds(100);
      setSignalNegative();
      delayMicroseconds(100);
      i++;
    }
    i = falling_time_counter;

    setAmplitude(0);
    while(i >= 0 && readButton()){
      setColor(200,0,0); //test
      // Signal Generation
      setSignalPositive();
      delayMicroseconds(100);
      turnSignalOff();
      delayMicroseconds(100);
      setSignalNegative();
      delayMicroseconds(100);
      i--;
    }*/
    setAmplitude(0);
    turnLedsOff();
    turnSignalOff();
    delay(3000);
  }
    function = 3;
    executePowerOff();
}

// Execute Power OFF command
void executePowerOff(){
  nBlink(function);
  int i;
  for(i=0; i <=255; i++){
    setColor(255-i,i,i);
    delay(5);
  }
  
  function      = 1;
  pressed_time  = 0;
  released_time = 0;
  delta_time    = 0;
  
  turnSignalOff();
  turnOffAnimation();
  loop();
}

// Working functions
void waitForOnSignal(){
  while(1){
    if(!readButton()){
      pressed_time = millis();
      while(!readButton()){
      released_time = millis();
      delta_time = released_time - pressed_time;

      if(delta_time >= 2000){
        turnOnAnimation();
        delay(500);
        turnLedsOff();
        selectFunction(); // NEXT STATUS
      }
      }
    }
  }
}

// Execute Function or command
void executeFunction(int function){
  switch(function){
    case 1: //USB Mode
        executeUSBMode();
        break;
    case 2: //Session Mode
        executeSessionMode();
        break;
    case 3: //Poweroff Mode
        executePowerOff();
        break;
  }
}

// Function selection
void selectFunction(){
  while(1){
    // Update function selection
            switch(function){
          case 1: setColor(0,0,255);  //USB mode (Blue)
                  break;
          case 2: setColor(0,255,0);  //Session mode (Green)
                  break;
          case 3: setColor(255,0,0);  //Poweroff mode (Red)
                  break;
          default: function = 1;
                  break;
            }

    // Function selection
    if(!readButton()){
      pressed_time = millis();    
      while(!readButton()){
        released_time = millis();
        delta_time = released_time - pressed_time;
        
        if(delta_time > 2000){
          // Execute function
          executeFunction(function);
          turnLedsOff();
          waitForOnSignal();
        }
      }

      // Function selection
      if(delta_time >= 150 && delta_time <= 2000){
        function ++;
      }
    }
  }
}


/*
 * END OF FUNCTIONS OR COMMANDS
 */







// Test functions
boolean readButton(){
  return ~BUT_VAL;
}

void testButton(){
  boolean var = readButton();
  digitalWrite(LED_B, var);
}

/*
 * LED control fucntions
 */
void nBlink(int function){
  char led;
  int i = 0;
  switch(function){
    case 1: led = 'B';
      break;
    case 2: led = 'G';
      break;
    case 3: led = 'R';
      break;
  }

    turnLedOn(led);
    delay(200);
    turnLedOff(led);
    delay(200);
      
    for(i=0; i<=2; i++){
      turnLedOn(led);
      delay(100);
      turnLedOff(led);
      delay(100);
    }
}

void turnOnAnimation(){
  turnLedsOff();
  int cnt;
  for(cnt = 0; cnt<255; cnt++)
  {
      if(cnt>255)
    {
      cnt = 255;
    }
    
    setColor(0, cnt, cnt);
    delay((int)TIME_STEP*1.5);
  }
}
void turnOffAnimation(){
  setColor(0, 255, 255);
  int cnt;
  for(cnt = 0; cnt<255; cnt+=1)
  {
      if(cnt>255)
    {
      cnt = 255;
    }
    
    setColor(0, 255-cnt, 255-cnt);
    delay((int)TIME_STEP*0.8);
  }
  turnLedsOff();
}
 
void setColor(int r, int g, int b) {
  analogWrite(LED_R, 255 - r);
  analogWrite(LED_G, 255 - g);
  analogWrite(LED_B, 255 - b);
}
void turnLedOn(char led){
  switch(led){
    case 'R':
          digitalWrite(LED_R, LOW);
          break;
    case 'G':
          digitalWrite(LED_G, LOW);
          break;
    case 'B':
          digitalWrite(LED_B, LOW);
          break;
  }
}

void turnLedOff(char led){
  switch(led){
    case 'R':
          digitalWrite(LED_R, HIGH);
          break;
    case 'G':
          digitalWrite(LED_G, HIGH);
          break;
    case 'B':
          digitalWrite(LED_B, HIGH);
          break;
  }
}

void turnLedsOff(){
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}
/*
 * End of LED control fucntions
 */

//Turn On possitive signal
void setSignalPositive(){
  digitalWrite(NEG_SIG, LOW);
  digitalWrite(POS_SIG, HIGH);
  digitalWrite(BRIGDE_EN, HIGH);
}

//Turn On negative signal
void setSignalNegative(){
  digitalWrite(POS_SIG, LOW);
  digitalWrite(NEG_SIG, HIGH);
  digitalWrite(BRIGDE_EN, HIGH);
}

//Turn off all signals
void turnSignalOff(){
  digitalWrite(BRIGDE_EN, LOW);
  digitalWrite(POS_SIG, LOW);
  digitalWrite(NEG_SIG, LOW);
}

// Set signal amplitude
void setAmplitude(int duty){
  analogWrite(AMP_CTRL, duty);
}
