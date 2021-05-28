const int potPin = 27;
const int butPin = 16;
const int U_Led = 14;
const int L_Led = 12;
const int R_Led = 15;
const int D_Led = 13;
int potVal = 0;
bool butState = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(U_Led, OUTPUT);
  pinMode(L_Led, OUTPUT);
  pinMode(R_Led, OUTPUT);
  pinMode(D_Led, OUTPUT);
  pinMode(butPin, INPUT_PULLUP);
}

void loop() {
// put your main code here, to run repeatedly:
//potVal = analogRead(potPin);
//Serial.println(potVal);
butState = digitalRead(butPin);
if (butState == LOW) {
    // turn LED on:
    digitalWrite(U_Led, HIGH);
    digitalWrite(L_Led, HIGH);
    digitalWrite(R_Led, HIGH);
    digitalWrite(D_Led, HIGH);
  } else {
    // turn LED off:
    digitalWrite(U_Led, LOW);
    digitalWrite(L_Led, LOW);
    digitalWrite(R_Led, LOW);
    digitalWrite(D_Led, LOW);
  }
delay(20);
}
