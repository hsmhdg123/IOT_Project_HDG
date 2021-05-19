void setup() {
  pinMode(2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(16, OUTPUT);
}

// the loop function runs over and over again forever
void loop() 
{
  digitalWrite(2, LOW);  
  for(int i=1023;i>800;i--)
  {
    analogWrite(16, i);
    delay(5);
  }

  digitalWrite(2, HIGH);
  for(int i=799;i<1023;i++)
  {
    analogWrite(16, i);
    delay(5);   
  }

}
