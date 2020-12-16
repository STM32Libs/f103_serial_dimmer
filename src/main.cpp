#include <Arduino.h>

#define led_on()  digitalWrite(PC13, LOW)
#define led_off()  digitalWrite(PC13, HIGH)
#define debug_on()  digitalWrite(PB13, HIGH)
#define debug_off()  digitalWrite(PB13, LOW)

HardwareSerial Serial3(PB11, PB10);

void start_timer_freq(TIM_TypeDef *Instance,uint32_t freq,callback_function_t callback){
  HardwareTimer *MyTim = new HardwareTimer(Instance);
  MyTim->setOverflow(freq, HERTZ_FORMAT);
  MyTim->attachInterrupt(callback);
  MyTim->resume();
}

void setup()
{
  pinMode(PC13, OUTPUT);
  pinMode(PB13, OUTPUT);
  led_on();
  delay(300);
  led_off();

  Serial3.begin(115200);
  //start_timer_freq(TIM1,1000,cdc_consumer);

}

void loop()
{
  static int count = 0;
  Serial3.print(count++);
  Serial3.println(" on Serial 3");
  led_on();
  delay(10);
  led_off();
  delay(2000);
}
