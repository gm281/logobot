// Connect to relevant serial line with:
// $ screen /dev/$DEVICE 9600
// Type chars in a-z range, each char results in built in led lighting up
// for variable amount of time (a is 50ms, z is 1.3s)
#include <AFMotor.h>

AF_Stepper motor(200, 1);
AF_Stepper motor2(200, 2);

int led = 13;
void setup() {
  Serial.begin(9600);
  motor.setSpeed(4);
  motor2.setSpeed(4);
  pinMode(led, OUTPUT);
}

void sleep_many_microseconds(int delay_microseconds) {
  if (delay_microseconds > 16000)
      delay(delay_microseconds / 1000);
  else
      delayMicroseconds(delay_microseconds);
}

void loop() {
  int i,rpm,limit;
  int delay_ms, delay_us;
  while (Serial.available() > 0) {
    unsigned char input = Serial.read();
    Serial.print("Read: ");
    Serial.println(input, DEC);
    if (input < 'a' || input > 'z')
      return;
    limit = 600;
    rpm = (16*16 - 16) * (input - 'a' + 1) / ('z' - 'a');
    Serial.print("RPM: ");
    Serial.println(rpm);
    delay_ms = (60000 / 200) / rpm / 2;
    delay_us = (60000 / 200) * 1000 / rpm / 2;
    Serial.println(rpm);
    Serial.println(delay_ms);
    Serial.println(delay_us);

    int onTime = (input - 'a' + 1) * 50;
    digitalWrite(led, HIGH);

    for(i=0; i<limit; i++) {
      motor.onestep(FORWARD, INTERLEAVE);
      sleep_many_microseconds(delay_us);
      motor2.onestep(BACKWARD, INTERLEAVE);
      sleep_many_microseconds(delay_us);
    }
    digitalWrite(led, LOW);

    for(i=0; i<limit; i++) {
      motor.onestep(BACKWARD, INTERLEAVE);
      sleep_many_microseconds(delay_us);
      motor2.onestep(FORWARD, INTERLEAVE);
      sleep_many_microseconds(delay_us);
    }
    Serial.println("Done");  
  }
  delay(1);
}


