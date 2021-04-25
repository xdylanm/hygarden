

void setup() {
  Serial.begin(9600);

  pinMode(BUILTIN_LED, OUTPUT);
  
}

void loop() {

  digitalWrite(BUILTIN_LED, HIGH);
  delay(800);
  digitalWrite(BUILTIN_LED, LOW);
  delay(200);

}
