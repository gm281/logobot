// Connect to relevant serial line with:
// $ screen /dev/$DEVICE 9600
// Type chars in a-z range, each char results in built in led lighting up
// for variable amount of time (a is 50ms, z is 1.3s)

int led = 13;

void setup() {
  Serial.begin(9600);
  pinMode(led, OUTPUT);     
}

void loop() {
  while (Serial.available() > 0) {
    unsigned char input = Serial.read();
    Serial.print("Read: ");
    Serial.println(input, DEC);
    if (input < 'a' || input > 'z')
      return;
    int onTime = (input - 'a' + 1) * 50;
    digitalWrite(led, HIGH);
    delay(onTime);
    digitalWrite(led, LOW);
  }
  delay(1);
}


