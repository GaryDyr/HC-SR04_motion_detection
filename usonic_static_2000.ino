/*
  Uses these primary examples from the author of NewPing to drive sonic sensor:
    HC-SR04 NewPing Iteration Demonstration
    HC-SR04-NewPing-Iteration.ino
  
  Displays results on Serial Monitor
*/

// Arduino/sensor set up
// Uses Serial Monitor to display duration and distance readings
// Include NewPing Library for HC-SR04 Ultrasonic Range Finder
#include <NewPing.h>

// Hook up HC-SR04 with Trig to Arduino Pin 4, Echo to Arduino pin 5
// Maximum Distance is 400 cm (4m)
 
#define TRIGGER_PIN  4
#define ECHO_PIN     5
#define MAX_DISTANCE 400
 
//Initialize the sensor pin setup and distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
unsigned int duration;
float distance;
const int iterations = 2000; //must change this to 500 if Uno, because cannot handle greater array size.
//unsigned int pings[iterations]; uncomment this if dumping to array before printing.
int cnt = 0;
void setup() {
  Serial.begin (9600);
 //If dumping to array use the following loop; else comment out  this entire loop out, and 
 //uncomment next loop
  for (cnt = 0; cnt <= iterations; cnt++){
    duration = sonar.ping(); //single pings @delay time.
    distance = (duration / 2) * 0.03445;
    //pings[cnt] = distance;  //uncomment if using array to hold distance values before printing.
    // if using array to hold data, comment out the next 3 lines
    Serial.print(duration); 
    Serial.print(",");
    Serial.println(distance);
    delay(33);
   }
/* 
 //uncomment this block   to test whether Serial causing issues while blocking the Serial.print lines in the previous loop. 
//TURNS OUT TO HAVE NO EFFECT ON OUTPUT VALUES.
  for (cnt = 0; cnt <= iterations; cnt++){
     Serial.print(pings[cnt]); 
     distance = (pings[cnt] / 2) * 0.034445;   
     Serial.print(",");
     Serial.println(distance); 
    delay(33);  
   }
*/
}
// loop() is not used, because only want one pass.
void loop() {
 }
 
