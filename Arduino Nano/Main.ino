#define encoderPinA 2
#define encoderPinB 3
#define LED 13
const int pwm = 9;
const int dir = 7;
const int analogInPin = A6;

volatile byte encRead= 0;//value to store chA, chB
                //encRead will be sequentially: 00 10 11 01 then loop back to 00 (CW)
volatile byte pre_enc=7;

int set = 0;
volatile int newPosition = 0;
int lastPosition= 0;
float error = 0, e_sum=0;
float Kp = 0.5, Ki=0.1 ;
int output=0;
void pwm1(int x) {
    if(x > 0)
    { 
        digitalWrite(dir, HIGH);         // quay cung chieu
        if (x>240) x = 240;
        analogWrite(pwm,x);         
    }
    else 
    { 
        digitalWrite(dir, LOW);        //quay nguoc chieu
        if (x<-240) x = -240;
        analogWrite(pwm,-x);
    }     
}
void setup() {
  TCCR1B = TCCR1B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 9 and 10
  TCCR2B = TCCR2B & 0b11111000 | 0x01; // set PWM frequency @ 31250 Hz for Pins 11 and 3
  pinMode(pwm, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  attachInterrupt(0, readEncoder, CHANGE);
  attachInterrupt(1, readEncoder, CHANGE);
  Serial.begin(57600);
  Serial.print("Start\n");
}

void loop() 
{
  error=set-newPosition;
  e_sum= e_sum+error;
  if(e_sum>50) e_sum=50;
  if(e_sum<-50) e_sum=-50;
  output=Kp*error+Ki*e_sum;
  if(output>100) output=100;
  if(output<-100) output=-100;
  pwm1(output);
  /*if(lastPosition!=newPosition)
  {
     Serial.print(newPosition);
     Serial.print("\n");
     lastPosition=newPosition;
  }*/
  if(error==0) digitalWrite(LED,HIGH);
  else digitalWrite(LED,LOW);
//  delayMicroseconds(100);
}

void readEncoder()
{
    encRead= (digitalRead(encoderPinA)<<1) | digitalRead(encoderPinB);
    switch(pre_enc)
    {
      case 0b00:
        if(encRead==0b10) 
        {
          newPosition++;
          break;
        }
        if(encRead==0b01)
        {
          newPosition--;
          break;
        }
        break;
      case 0b01:
        if(encRead==0b00) 
        {
          newPosition++;
          break;
        }
        if(encRead==0b11)
        {
          newPosition--;
          break;
        }
        break;
      case 0b11:
        if(encRead==0b01) 
        {
          newPosition++;
          break;
        }
        if(encRead==0b10)
        {
          newPosition--;
          break;
        }
        break;
      case 0b10:
      if(encRead==0b11) 
        {
          newPosition++;
          break;
        }
        if(encRead==0b00)
        {
          newPosition--;
          break;
        }
        break;
      default:
        break;
    }
    pre_enc=encRead;
}

