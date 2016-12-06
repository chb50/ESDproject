//Written By: Cedric Blake
//script to test out using the arduino uno's ADC
//tested using a VEX Potentiometer
//digital value ranges from 0 to 1023
//values are displayed on the Serial Monitor


int val = 0;
String namae;

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  Serial.begin(9600);
  Serial.println("TEST");
}

void loop() {
  // put your main code here, to run repeatedly:
  val = analogRead(A0);
  //printf("%d\n",val);
  Serial.print(val);
  Serial.print("\n");
  delay(500);
}
