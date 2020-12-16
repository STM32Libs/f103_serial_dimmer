#include <Arduino.h>

#define LED_PIO   PC13
#define DEBUG_PIO PB13
#define RELAY_PIO PB4
//sync test PB6
//sync prod PB5
#define SYNC_PIO  PB6

#define led_on()  digitalWrite(LED_PIO, LOW)
#define led_off()  digitalWrite(LED_PIO, HIGH)

#define debug_on()  digitalWrite(DEBUG_PIO, HIGH)
#define debug_off()  digitalWrite(DEBUG_PIO, LOW)

#define relay_on()    digitalWrite(RELAY_PIO, HIGH)
#define relay_off()   digitalWrite(RELAY_PIO, LOW)

HardwareSerial Serial3(PB11, PB10);

void sync_irq(){
  debug_on();
  delayMicroseconds(5);
  debug_off();
}

void start_timer_freq(TIM_TypeDef *Instance,uint32_t freq,callback_function_t callback){
  HardwareTimer *MyTim = new HardwareTimer(Instance);
  MyTim->setOverflow(freq, HERTZ_FORMAT);
  MyTim->attachInterrupt(callback);
  MyTim->resume();
}

void setup()
{
  pinMode(LED_PIO, OUTPUT);       //led
  pinMode(DEBUG_PIO, OUTPUT);     //debug
  pinMode(RELAY_PIO, OUTPUT);     //relay
  pinMode(SYNC_PIO, INPUT_PULLUP);//sync

  led_on();
  delay(300);
  led_off();

  debug_on();
  delayMicroseconds(100);
  debug_off();


  Serial3.begin(115200);
  //start_timer_freq(TIM1,1000,cdc_consumer);
  attachInterrupt(digitalPinToInterrupt(SYNC_PIO),sync_irq,RISING);

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
