/*
Bydlo code for moisture sensors
*/
#include <Wire.h> //i2c module
#include <JC_Button.h> //easy button module
//#define HardwareSerial_h //disable serial port, significally reduces code size
#define buttonPin 3 //button pin
#define ledRedPin 9 //led pins for testing
#define ledGreenPin 10
#define ledBluePin 11
#define SLAVE_ADDRESS 0x04 // arduino will have that i2c address in slave mode
#define powerPin 5
//timers
unsigned long timing; //timer for collecting cycle
unsigned long fadeTiming; //timer for collecting cycle
unsigned long period; //period for collector in seconds
//data
byte reqType; //request data, input from raspberry
byte countCollects; //number of collects for median calculation
int* dataArray; //data storage for sensors
int* collectArray; //pointer for dynamic array of sensor values creation
bool* sensorArray; //store flags if sensor is requested and must be used in collect function
//pins
const uint8_t analog_pins[] = {A0,A1,A2,A3,A6};
byte sensorPins; //count of sensors calculating from analog_pins, 7 analog pins on nano (used in loops, so starts with zero) minus 2 for i2c (see above)
//debug
bool debugSerial = false; //change to true to enable serial port messages. !!WARNING makes i2c proto almost unusable (dont know why)
//LED
byte ledBrightness; //from 0 to 10
Button ledButton(buttonPin,50); //set button type on buttonPin, with 50 debounce time, input_pullup (JC_button functionality)
byte ledActive; //active led var
byte fadeBrightness = 0; // how bright the LED is
byte fadeAmount = 5; // how many points to fade the LED by
bool fadeOn; // does fading to be used
void setup() {
//set default values:
ledBrightness = 1;
countCollects = 5;
period = 60;
//initialization
Wire.begin(SLAVE_ADDRESS); // I2C start on hard-coded address
Wire.onReceive(receive); //When raspberry sends data, do receive function
Wire.onRequest(send); //response function for answer
ledButton.begin(); //button module init
sensorPins = sizeof(analog_pins); //count of sensor pins calculation
ledActive = sensorPins; //default status for led is off (pins starts with 0, so last pin is (sizeof(analog_pins) - 1).
pinMode(ledRedPin, OUTPUT); // color led init
pinMode(ledGreenPin, OUTPUT);
pinMode(ledBluePin, OUTPUT);
pinMode(powerPin, OUTPUT); //Enable Power pin mode
//creating dynamic arrays:
dataArray = new int [sensorPins]; //creating dynamic array for data storage
collectArray = new int [countCollects]; //creating dynamic array for median calculating
sensorArray = new bool [sensorPins]; //creating dynamic array for flags
//zeroing arrays:
for (byte i = 0; i < sensorPins; i++) { //or i <= 4
pinMode(analog_pins[i],INPUT_PULLUP);
dataArray[i] = 0;
sensorArray[i] = 0;
}
if (debugSerial == true) {
Serial.begin(9600);
Serial.println("Ready!");
}
}
void loop() {
ledButton.read();
if (millis() - timing > (period * 1000 )){ // period of collecting data
timing = millis();
delay(500); //warming sensor for some time
for (byte i = 0; i < sensorPins; i++) {
digitalWrite(powerPin, HIGH);
if (sensorArray[i] == 1 ) { //check array of flags if sensor is used
collect(i); //send to function what sensor need to be processed
if (debugSerial == true) {
Serial.print ("sensorPins = ");
Serial.println (sensorPins);
Serial.print ("Sensor ");
Serial.print (i);
Serial.print (" collected data = ");
Serial.println (dataArray[i]); //debug
Serial.print ("sensorFlag = ");
Serial.println (sensorArray[i]);
}
}
digitalWrite(powerPin, LOW); //collect ended. power off
}
}
//wait for input from button
if (ledButton.wasReleased()) {
if (ledActive < sensorPins) {
ledActive++;
led();
}
else {
ledActive = 0;
led();
}
}
else if (ledButton.pressedFor(1000))
if (digitalRead(buttonPin) == LOW) { //"low" because of pullup
sensorArray[0] = 1;
digitalWrite(powerPin, HIGH);
collect(0); //collecting 0 button for test
delay(500);
digitalWrite(powerPin, LOW); //collect ended. power off
}
if (debugSerial == true) {
Serial.print ("ledActive = ");
Serial.println (ledActive);
Serial.print (" collected data = ");
Serial.println (dataArray[ledActive]); //debug
}
fader();
}
void receive() {
while (Wire.available()) {
reqType = Wire.read();
if (debugSerial == true) {
Serial.print("Get request, type = ");
Serial.println(reqType);
Serial.println(sensorArray[reqType]);
}
if (reqType <= sensorPins) { //check request if it lower than number of maximum available sensors, this is sensor number. Another is service codes
if (sensorArray[reqType] == 0) { //check if flag not set
sensorArray[reqType] = 1; //set flag of sensors array (enable sensor)
if (debugSerial == true) {
Serial.print("Sensor ");
Serial.print(reqType);
Serial.println(" activated");
}
collect(reqType); //collect data from newly activated sensor *!! COMMENT THIS LINE on debug process it is not working with enabled serial port, dont know why*
}
}
else {
if (debugSerial == true) {
Serial.println("Service code received");
}
}
}
}
int collect(byte sensNumber) {
//Here goes loop for calculating median of value
for (byte i = 0; i < countCollects; i++) { //do countCollect number of sensor requests
collectArray[i] = analogRead(sensNumber);
// Serial.println(collectArray[i]); //detailed debug
delay(150); //delay between requests
}
sortArray();
if ( (countCollects % 2) == 0) { //even number of elements
dataArray[sensNumber] = ((collectArray[countCollects/2] + collectArray[(countCollects/2)+1]) / 2);
}
else { //odd number of elements
dataArray[sensNumber] = collectArray[countCollects/2];
}
//send();
led();
}
void send () {
byte metric;
if (debugSerial == true) {
Serial.print("Raw data for sensor ");
Serial.print(reqType);
Serial.print(" is ");
Serial.println(dataArray[reqType]);
}
metric = map(dataArray[reqType], 0, 1023, 0, 255);
metric = constrain(metric, 0, 255);
Wire.write(int(metric));
if (debugSerial == true) {
Serial.print("Sensor ");
Serial.print(reqType);
Serial.print(" data sent = ");
Serial.println(metric);
}
}
void sortArray() {
int out, in, swapper;
for(out=0 ; out < countCollects; out++) { // outer loop
for(in=out; in<(countCollects-1); in++) { // inner loop
if( collectArray[in] > collectArray[in+1] ) { // out of order?
// swap them:
swapper = collectArray[in];
collectArray[in] = collectArray[in+1];
collectArray[in+1] = swapper;
}
}
}
}
void led () {
if (ledActive == sensorPins) {
fadeOn = 0;
analogWrite(ledRedPin, 0);
analogWrite(ledGreenPin, 0);
analogWrite(ledBluePin, 0);
}
else if (dataArray[ledActive] > 900 ) {
fadeOn = 0;
analogWrite(ledRedPin, 25 * ledBrightness);
//Serial.println(50 * ledBrightness);
analogWrite(ledGreenPin, 0);
analogWrite(ledBluePin, 0);
}
else if ((dataArray[ledActive] < 900) && (dataArray[ledActive] > 750)) {
fadeOn = 0;
analogWrite(ledRedPin, 25 * ledBrightness);
analogWrite(ledGreenPin, 15 * ledBrightness);
analogWrite(ledBluePin, 0);
}
else if (dataArray[ledActive] < 750 && (dataArray[ledActive] > 15)) {
fadeOn = 0;
analogWrite(ledRedPin, 0);
analogWrite(ledGreenPin, 25 * ledBrightness);
analogWrite(ledBluePin, 0);
}
else if (dataArray[ledActive] < 15) {
fadeOn = 1;
analogWrite(ledGreenPin, 0);
analogWrite(ledBluePin, 0);
}
}
void fader() {
if ( fadeOn == 1 ) {
if (millis() - fadeTiming > 10 ) { // wait for 30 milliseconds to see the dimming effect
fadeTiming = millis();
analogWrite(ledRedPin, fadeBrightness);
// change the brightness for next time through the loop:
fadeBrightness = fadeBrightness + fadeAmount;
// reverse the direction of the fading at the ends of the fade:
if (fadeBrightness <= 0 || fadeBrightness >= 255) {
fadeAmount = -fadeAmount;
}
}
}
}
