
/*
 * This sketch uses the ultrasonic sensor (HC-SR04), a button, 
 *  and a 60 LED/m programmable strip from BTF Lighting. 
 * The process is triggered by clicking the button.
 * At each position the ultrasonic sensor is read 5 times and 
 * the median calculated.
 * The position, the useconds, the distance in cm, and the distance
 * in inches are then printed the serial monitor.
 * The data can be copied via Ctrl-C. for further analysis
 /*
 This also inclues a button to trigger a read. 
 Button setup
  GND Analog side    Btn side A common with usonic GND on breadboard
  2                  Btn side B; NOTE MUST USE INTERNAL PULLUP RESISTOR VIA "INPUT_PULLUP"
  9                  Servo signal Pin 3 for older Airtronics
 
 The button uses the internal PULLUP resistor related to pin 2
 See:
 https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
 for excellent details on NewPing.  
*/
 
// Serial Monitor to display Range Finder distance readings;
// if using vscode be sure "defines": ["USBCON"] added to c_cpp_properties.json
// but the data is also on the usb port.
 
// Include NewPing Library for HC-SR04 Ultrasonic Range Finder
# include <NewPing.h>
# include <FastLED.h>
# include <CircularBuffer.h>
// Hook up HC-SR04 with Trig to Arduino Pin 12, Echo to Arduino pin 11
// Maximum Distance is 400 cm (4m)

//LED variables
#define NUM_LEDS 60 //using a 60 led/m string BTF-LIGHTING WS2812b 60leds 60pixels Waterproof IP65
#define LEDDATA_PIN 3//set up the block of memory that will be used for storing and manipulating the led data
//CRGB is a template containing 3 values: r, g, b, or can be color hex code.
//documentation says 
CRGB leds[NUM_LEDS]; 

// HC-SR04 defined variables not used right now; see below.
// #define TRIGGER_PIN_FRONT  4
// #define ECHO_PIN_FRONT     5
// #define TRIGGER_PIN_SIDE   6
// #define ECHO_PIN_SIDE      7
//all distances in cm; will be converted to useconds

// testing data on person in room.
// All distances in cm (will be converted to times later).
#define PING_NUM 3  //number of pings to read into echo_time array
#define MAX_SENSORS 2
#define MAX_DISTANCE 125 //ignore anything over this max front distance
#define FRONT_DISTANCE 50 //
#define SIDE_DISTANCE 70  // optimum side distance desired.
#define MAX_FRONT 20  // max stop dist allowed ( shift to red if below) from object to collision point.
#define MIN_FRONT 10  // min stop dist allowed from ojbect to collision point.
#define WARN_FRONT 50 // distance from front collision to shift from green to yellow warning color.
#define MAX_SIDE 60   // maximum side range allowed to shart shift right. 
#define MIN_SIDE 50   // min side range to start shifting if below to shift left.

//Real garage data
/*
#define MAX_SENSORS 2  // Number of sensors is array
#define MAX_DISTANCE 4000 //in cm
#define FRONT_DISTANCE 110 
#define SIDE_DISTANCE 58
#define MAX_FRONT 120  // dist in cm from object to stationary point of major motion change direction
#define MIN_FRONT 100  // 
#define WARN_FRONT 240 // distance to shift from green to yellow warning color.
#define MAX_SIDE 62
#define MIN_SIDE 51
*/
#define SONAR_NUM 2
#define PING_INTERVAL 33    // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

uint8_t currentSensor = 0;  // Keeps track of which sensor is active. Max: 255
unsigned long pingTimer[SONAR_NUM];
unsigned long sensor_time[SONAR_NUM];

//Initialize the sensor pin setup and distance; not used right now
// pins in pairs of trigger pin as i, echo pin as i + 1.
// should  be check for maximum with of pins available.
// if more needed can use one ping, but must not use any other hardware/code using timer2
// Uses Timer2

//... be mindful of PWM or timing conflicts messing with Timer2
//  may cause (namely PWM on pins 3 & 11 on Arduino, PWM on pins 9 and 10 on
//  Mega, and Tone library). Simple to use timer interrupt functions you can
//  use in your sketches totaly unrelated to ultrasonic sensors (don't use if
//  you're also using NewPing's ping_timer because both use Timer2 interrupts).

//for (int i = 4; i < SONAR_NUM; i++) { 
// NewPing sonar[SONAR_NUM] = {   // Sensor object array.
//  NewPing(i, i+1, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping. 
//  }
//}
//NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
NewPing sonar[SONAR_NUM] = {   // Sensor object array.
  NewPing(4, 5, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, max distance to ping. 
  NewPing(6, 7, MAX_DISTANCE)
};

long int side_time, front_time;
float sound_vel =  0.03402;  //CHANGE THIS (AS CM/USEC) DEPENDING ON TEMP. AND HUMIDITY
int iterations = 5; //times to read ultrasonic sensor to average
int ledpos = 0;        //default intial position of led string
long advance_limit = 4; // forward motion detection limit, in cm
bool btn_state = false;
int halfstrip;
unsigned int pingSpeed = 50; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
//unsigned long pingTimer;     // Holds the next ping time.
int i, j, k;
long front_echo_time;
long side_echo_time;
long max_time;
long min_front_time;
long max_front_time;
long min_side_time;
long max_side_time;
long caution_yellow_time;
long slope;
long echo_time [2][3];
long echo_ping, echo_sum, echo_aver;

uint8_t goRed, goGrn, goBlue;
// In Arduino world, not necessary to declare functions;
// compiler does it for you. However, vscode intellisense has a fit.
// void frontEchoCheck();
// void pingAllSensors();
// void LeftOfTargetRange();
// void RightOfTargetRange();
// void InTargetRange();
// void echoCheck();
// void pingResult();
// void setPixel();
// void FastLED.show();
// void setAll();
// void setPixel();

void setup(){
  // Set up the serial monitor, if using.
  Serial.begin(9600); //in ping example 115200; must be in setup to communicate with serial monitor
  // setCorrection turns down red and blue a bit. It appears to adjust different strips to 
  // a similar color temperature, and maybe human visual color perception. 
  // addLEDs is a template that is calling a template of: type , pin, and ledorder?
  FastLED.addLeds<WS2812B, LEDDATA_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  // Check if strip contains even number of leds; only set even.
  if (NUM_LEDS % 2 != 0) {
        halfstrip = (NUM_LEDS-1)/2;
      }
      else {
        halfstrip = NUM_LEDS/2;
      }
  // Convert cm distances to round trip usec time, using reasonable sound velocity
  front_echo_time = long(sound_vel * FRONT_DISTANCE*2);
  side_echo_time = long(sound_vel * SIDE_DISTANCE*2);
  max_time = long(sound_vel * MAX_DISTANCE)*2;
  min_front_time = long(sound_vel * MIN_FRONT)*2;
  max_front_time = long(sound_vel * MAX_FRONT)*2;
  min_side_time = long(sound_vel * MIN_SIDE)*2;
  max_side_time = long(sound_vel * MAX_SIDE)*2;
  caution_yellow_time = long(sound_vel * WARN_FRONT);
  advance_limit = long(sound_vel * 4);
  
  //Set up the leds; BTF strips are line NEOPIXEL or WS2812B, i.e., GRB sequence
  // Could probably have used NEOPIXEL HERE
  FastLED.addLeds<WS2812B, LEDDATA_PIN, GRB>(leds, NUM_LEDS);
  delay(15); //delays always in milliseconds
  // For each sensor specify an initial time to read.
  // gives plenty of time to setup the timer read array and start 
  // through the loop, BUT i REALLY CANNOT SET AN INTIAL read
  // because screws the entire pingtimer array
  //pingTimer[0] = millis() + 75;
  //for (uint8_t i = 1; i < SONAR_NUM; i++) {
  //  pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  //}
  // Make sure colors off to start
  // Had beast of time finding this; it is in FastLED
  // fastled.io/docs/3.1/class_c_fast_l_e_d.html#a570df74cf09e6215c3647333d2b479a9
  // which leads to https://github.com/FastLED/FastLED/blob/master/FastLED.h see line ~512
  // do not understand how it shows color; seems like should only set color.
  // Kriegman said showColor does not populate leds array we define, it bypasses it and 
  // directly goes to strip. So color is not retained for reading.
  
  FastLED.showColor(CRGB::Black); //could use CHSV(hue, sat. value), or CRGB(r, g, b)
}
//now use the event driven ping_timer method to deal with the usonics.
void loop() {
  //look for one ping on only the sensor 0 = front sensor
  //echo_ping = 0;
  currentSensor = 0;
  echo_sum = 0;
  echo_aver = 0;
  //echo_ping =  sonar[currentSensor].ping_median(3);
  // Other code that *DOESN'T* analyze ping results can go here.
  //from NewPing.cpp long signed variable ping_result is generated once the ping echo
  //has stopped and the echo bit are set.
  //There are a bunch of cases:
  //Is there any vehicle there to ping off?
  for (i = 0; i < SONAR_NUM; i++) { 
    for (k = 0; k < PING_NUM; k++) {
      currentSensor = i;                          // Sensor being accessed.
      echo_time[currentSensor][k] = 0;
      delay(PING_INTERVAL);
      echo_time[currentSensor][k] =  sonar[currentSensor].ping();

    }
  }
  slope = 0; //'derivative' of 3 measurements 
  //get average echo and slope
  for (k = 0; k < PING_NUM; k++) {
    echo_sum = echo_sum +  echo_time[0][k];
    slope += echo_time[0][0] - echo_time[0][k];  
  }
  echo_aver = long(echo_sum/PING_NUM);
  //slope is positive, only if car moving toward sensor.
  if (slope <= advance_limit) {slope = 0;} //must be stopped or reversing.
  //test anything pings: in range (aver), or stopped, or reversing in range (slope)
  if ((echo_aver > max_time) || (slope = 0)) { 
    // No front echo returned; ensure leds dark
    Serial.print("No object or slope = 0 respectively: ");
    Serial.print(long(echo_aver * sound_vel));
    Serial.print("cm, ");
    Serial.println(slope);
    FastLED.showColor(CRGB::Black); 
    delay(3000); // delay 3 s repeat loop
  }
  //Is car stopped, but in range?
  else { //car moving forward and in range.
    // start fast MONITORING
    // 3 cases possible, object advancin, stopped, receding.
    // start fast polling all sensors 100 ms both sensors. use a 2 sensor x 3 time array
    // "integrate" differences.
    slope = 0; //'derivative' of 3 measurements.
    //Pinging all sensor overkill, but saves one if statement and redundancy.
    //Note that this starts the multiple sensor event driven ping cycle. It will never 
    //return from it, unless we do following.
    //"Remember, to analyze the ping results, you do that in the pingResults() 
    //function in the above sketches or in the oneSensorCycle() function 
    //in the 15 Sensors Example Sketch. 
    //Also, none of these will properly work if you do any delay statements 
    //at any point in your sketch.
    //If you ever want to stop the pings in your sketch, for example to do something 
    //that requires delays or takes longer than 33ms to process, do the following:
    //for (uint8_t i = 0; i < SONAR_NUM; i++) pingTimer[i] = -1;
    //To start the pings again, do the following:
    //pingTimer[0] = millis();
    //for (uint8_t i = 1; i < SONAR_NUM; i++) pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
    //GRD WELL I SHOULD HAVE READ EVERYTHING CLOSER. THIS IS NEAT, BUT I DO HAVE DELAYS, AND 
    //I DO NEED TO GET OUT OF THE LOOP.
    //So need to set the start and stop fast reading conditions.
    pingTimer[0] = millis();
    //for (uint8_t i = 1; i < SONAR_NUM; i++) {pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
    pingAllSensors(); //pings all sensors 3 times;l to array echo_time[# of sensors][3]
  } 
} //void

//https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home#!ping-3-sensors-sketch for details
//https://bitbucket.org/teckel12/arduino-new-ping/wiki/Help%20with%2015%20Sensors%20Example%20Sketch
//BELOW CODE KEEPS TRACK OF WHAT ALL SENSORS ARE DOING VIA THE SENSOR_TIME ARRAY. 
void pingAllSensors() {
  //take 3 sensor readings from each sensor
  // so 2 x3 measurements is 192 ms
  // allow 33ms per reading
 // Loop through all the sensors. 
 /*
     for (i = 0; i < SONAR_NUM; i++) { 
       for (int k = 0; k < PING_NUM; k++) {
          currentSensor = i;                          // Sensor being accessed.
          echo_time[currentSensor][k] = 0;
          delay(PING_INTERVAL);
          echo_time[currentSensor][k] =  sonar[currentSensor].ping();
*/
//Set up the pingTimer array
  pingTimer[0] = millis() + 75;
  for (uint8_t i = 1; i < SONAR_NUM; i++) {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }
  //take 3 sensor readings from each sensor
  for (j = 0; j < PING_NUM; j++) {
    for (i = 0; i <= SONAR_NUM; i++) {
      // Loop through all the sensors.
      if (millis() >= pingTimer[i]) {         // Is it this sensor's time to ping?
          pingTimer[i] += PING_INTERVAL * SONAR_NUM;  // Set next time this sensor will be pinged.
          sonar[currentSensor].timer_stop();          // Make sure previous timer is canceled before starting a new ping (insurance).
          currentSensor = i;                          // Sensor being accessed.
          sensor_time[currentSensor] = 0;             // Make time zero in case there's no ping echo for this sensor.

          //ping_timer's function below is to start the ping timer and return a value. 
          //first calls ping_trigger to set the trigger pin low, to 
          //be sure starting low, then sets it high to read an echo. If an echo
          //comes back, ping_trigger returns True, and ping_timer then
          //calls timer_us(frequency {ECHO_TIMER_FREQ = 24us, check for a ping echo}, echoCheck); 
          //looking at specs; SR04 needs about 10 us for pulse, but then needs to read. The 24us
          //is not an issue for closeness (24us = .73 cm).
          //timer_us first calls timer_setup, which stops the timer and 
          //resets? the countdown time and sets an interrupt, using ASM timer2 register calls to processor. 
          //Timer_setup then calls the user function, in
          //this case, echoCheck, which calls check_timer, which, if an
          //echo is received. puts the time value into variable ping_result and returns True
          //that it got an echo. echoCheck checks if check_timer true and saves
          //ping_result, and calls pingResult to process the echo value. 
          //Finally returning control to ping_timer, which then enables 
          //timer2 (for UNO) function. I think I got it right, but it is complicated.  
          // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
          sonar[currentSensor].ping_timer(echoCheck); 
         //interrupt over and ping_timer has started timer2 for another ping cycle.
      }
    }
  }// Other code that *DOESN'T* analyze ping results can go here.
}

void echoCheck() { // If ping received, set the sensor distance to array.
  if (sonar[currentSensor].check_timer()) {
    echo_time[currentSensor][PING_NUM] = sonar[currentSensor].ping_result;
    //pingResult(currentSensor);
  }
}

//
void pingResult(uint8_t sensor) { // Sensor got a ping, do something with the result.
  //j is globally defined; value available.
  // A ping means that the echo pin must have gone high and a value came through.
  // The following code would be replaced with your code that does something with the ping result.
  //Serial.print(sensor);
  //Serial.print(" ");
  //Serial.print(sensor_time[sensor]);
  echo_time[sensor][PING_NUM] = sensor_time[sensor];
  //Serial.println("cm");
  Serial.println("Processing Sensors");
  //check diff. between first ping and subsequent pings to see if consistent change.
  // slope is not a true derivative; differences from first measurement 
  slope = 0; //'derivative' of 3 measurements.
  for (i=1; i < PING_NUM; i++) {
    slope += echo_time[0][0] - echo_time[0][i];
  }
  // Is car advancing? 
  // Always an error in measurement; use a 4 cm limit for two counts
  // There is at least a 1/2 cm of variation, but assume
  // more like +/- 4 cm or a bit over an 1". especially at larger distance.
  // A while loop more appropriate here, but it could 
  // end up in an infinite loop if advance_limit is off.
  // force slope to 0 if below advance_limit: stopped or moving away.
  for (j = 0; j < MAX_SENSORS; j++){
    Serial.print("Sensor ");
    Serial.print(j);
    for (i=0; i < PING_NUM; i++) {
      Serial.print(", " );
      Serial.print(echo_time[j][i]);
     }
    Serial.println();
  }
  Serial.print("slope is ");
  Serial.println(slope);  

  if (slope <= advance_limit) { //is car receding or stopped?
    slope = 0;
    // kill multisensor read operation and return to loop
    for (uint8_t i = 0; i < SONAR_NUM; i++) {pingTimer[i] = -1;}
  }
  if (slope >= advance_limit) { //vehicle is advancing (moving to back of garage).

     //what is side doing? set 3 cases move left, -1; 0 ok; move right, 1
    //average 3 side measurements;side pins are already in 2nd row.
    long side_aver;
    long sum;
    uint8_t side_move = 0;
    for (uint8_t sx = 0; sx <= 2; sx++) {
      sum += echo_time[1][sx];
    }
    side_aver = sum/3;
    if (side_aver >= max_side_time) {side_move = 1;} 
    if (side_aver <= min_side_time) {side_move = -1;} //move left
    // Fix led strip color based on motion and position of front sensor (0 in array)
    // 3 cases; bit tricky, use aver. or last measurment as to position?
    if ((echo_time[0][3] <= max_time) && (echo_time[0][3] >= caution_yellow_time)) {// car in forward green range
      Serial.println("echo_time is between max_time and yellow");
      delay(20);
      FastLED.showColor(CRGB::Green);
      Serial.print("side_move and min.. & max_side time are ");
      Serial.print(side_move);
      Serial.print(min_side_time);
      Serial.println(max_side_time);
      delay(20);
      switch (side_move) {
        case -1 : 
          LeftOfTargetRange(0, 255, 0, 50);
          break;
        case 0 :
          InTargetRange(0, 0xff, 0, 50);
          break;
        case 1 :  
          RightOfTargetRange(0, 255, 0, 50);
          break;
        default: 
          FastLED.showColor(CRGB::Black);
      }
    }
    //Advancing: In range of front stop? Blink red.
    else if (echo_time[0][3] <= max_front_time) {
      Serial.print('in blink red, echo and max front time are');
      Serial.print(echo_time[0][3]);
      Serial.println(max_front_time);
      FastLED.showColor(CRGB::Red); //orFastLED.showColor(0xff0000)
      delay(50);
      FastLED.showColor(CRGB::Black);
      delay(50);
    }
    //advancing - in range of caution area?
    else if ((echo_time[0][3] >= max_front_time) && (echo_time[0][3] <= caution_yellow_time)) {
      //dark yellow
      Serial.print('in yellow, echo and max front time are');
      Serial.print(echo_time[0][3]);
      Serial.println(max_front_time);
      delay(20);
      goRed = 255;
      goGrn = 140; //0x8c;
      goBlue = 0;
      switch (side_move) {
        case -1 : 
          LeftOfTargetRange(goRed, goGrn, goBlue, 50);
          break;
        case 0 :
          InTargetRange(goRed, goGrn, goBlue, 50);
          break;
        case 1 :  
          RightOfTargetRange(goRed, goGrn, goBlue, 50);
          break;
        default: 
            FastLED.showColor(CRGB::Black);  //uses only this form of CRGB           
      } //switch
    }
    else {// not advancing slope must  be 0 or < advance_limit.
    //3 cses left car is stopped in range moving backward, or is in red zone.
    FastLED.showColor(CRGB::Black);
    }         
  } //slope advancing
  //no echo 
}

//Modified From: https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectThenewKITT

void InTargetRange(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay) {
    
      //HMMM, MAY REALLY WANT THIS TO BE UNDER A WHILE LOOP
    //First, starting at LED 0, shift by one LED, then turn on every third LED
    //from the altered position. 
    //setPixel fcn defined near bottomm, fills each member of the leds array.
    for (int q=0; q < 3; q++) {
      for (int i=0; i < halfstrip; i=i+3) {
        setPixel(i+q, red, green, blue);    //turn every third pixel on
        //Set other half of strip
        setPixel(NUM_LEDS - (i + q) , red, green, blue);
      }
      FastLED.show()
      ;
      delay(SpeedDelay);
     
      for (int i=0; i < halfstrip; i=i+3) {
        setPixel(i+q, 0,0,0);        //turn every third pixel off
        setPixel(NUM_LEDS - (i + q), red, green, blue);
      }
    }
  }

void LeftOfTargetRange(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay) {
  //Indicate object needs to move right
  //First, starting at LED 0, shift by one LED, then turn on every third LED
  //from the altered position.  
  for (uint8_t q=0; q < 3; q++) {
    for (int i=0; i < NUM_LEDS; i=i+3) {
      setPixel(NUM_LEDS - (i + q) , red, green, blue); //turn every third pixel on
    }
    FastLED.show()
    ;
    delay(SpeedDelay);
    for (int i=0; i < NUM_LEDS; i=i+3) {
      setPixel(NUM_LEDS - (i + q), red, green, blue); //turn every third pixel off
    }
  }
}

void RightOfTargetRange(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay) {
  //Indicate move object to left
  //First, starting at LED 0, shift by one LED, then turn on every third LED
  //from the altered position.  
  for (int q=0; q < 3; q++) {
    for (int i=0; i < NUM_LEDS; i=i+3) {
      setPixel(i+q, red, green, blue);    //turn every third pixel on
    }
    FastLED.show()
    ;
    delay(SpeedDelay);
    for (int i=0; i < NUM_LEDS; i=i+3) {
      setPixel(i+q, 0,0,0);        //turn every third pixel off
    }
  }
}

void setAll(uint8_t red, uint8_t green, uint8_t blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
}
// Set a LED color (not yet visible)
void setPixel(int Pixel, uint8_t red, uint8_t green, uint8_t blue) {
// FastLED
leds[Pixel].r = red;
leds[Pixel].g = green;
leds[Pixel].b = blue;
}
