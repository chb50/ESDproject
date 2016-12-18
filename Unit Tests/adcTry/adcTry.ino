
//Written By: Cedric Blake
//script to test out using the arduino uno's ADC
//tested using a VEX Potentiometer
//digital value ranges from 0 to 1023
//values are displayed on the Serial Monitor

int val = 0;
String namae;
int dPower = 8;
int ledColor = 9; //led color hopefully
int color = 0; //will change throughout runtime to aquire different values
int colorChange = 10; //how drastically the color will change

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  pinMode(ledColor, OUTPUT);
  pinMode(dPower, OUTPUT);
  digitalWrite(dPower, 1);
  Serial.begin(9600);
  Serial.println("TEST");
}

void loop() {
  // put your main code here, to run repeatedly:
  val = analogRead(A0);
  analogWrite(ledColor, color);

  color = color + colorChange;
  
  Serial.print(val);
  Serial.print("\n");
  delay(100);
}
