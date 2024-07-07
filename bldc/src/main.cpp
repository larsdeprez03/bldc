#include <Arduino.h>
#include <Servo.h>

Servo esc; // Create a Servo object to control the ESC

int potPin = A7; // Analog pin connected to the potentiometer
int escPin = 2;  // PWM pin connected to the ESC signal wire
int potValue;    // Variable to store the potentiometer value
int escValue;    // Variable to store the ESC signal value

void setup()
{
  esc.attach(escPin);          // Attach the ESC to the PWM pin
  esc.writeMicroseconds(1000); // Initialize ESC with a safe minimum value
  delay(1000);                 // Wait for a second for the ESC to initialize
  Serial.begin(9600);
}

void loop()
{
  while(digitalRead(A6));
  potValue = analogRead(potPin);                       // Read the potentiometer value (0-1023)
  escValue = map(potValue, 520, 1023, 1000, 2000);     // Map the value to ESC signal range (1000-2000 microseconds)
  if (potValue < 520) escValue = 1000;                 // Ensure escValue is 1000 for potValue below 520
  esc.writeMicroseconds(escValue);                     // Send the signal to the ESC
  Serial.print(potValue); Serial.print(":"); 
  Serial.println(escValue);
  delay(20);                                           // Small delay to allow the ESC to respond
}
