void setup() {
  // put your setup code here, to run once:
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  
}

static int StrobeTimeA = 120;
static int StrobeTimeB = 20;

void loop() 
{
  // put your main code here, to run repeatedly:
  analogWrite(9,0);
  analogWrite(10,0);
  analogWrite(11,0);
  delay(StrobeTimeA);
  analogWrite(9,255);
  analogWrite(10,255);
  analogWrite(11,255);
  delay(StrobeTimeB);
  
}
