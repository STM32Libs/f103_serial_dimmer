#include <Arduino.h>
#include <ArduinoJson.h>

#define LED_PIO   PC13
#define DEBUG_PIO PB13
#define RELAY_PIO PB4
//sync test PB6
//sync prod PB5
#define SYNC_PIO  PB5

#define PWM_LIGHT_1  PA8
#define PWM_LIGHT_2  PA9
#define PWM_LIGHT_3  PA10
#define PWM_LIGHT_4  PA11
#define PWM_LIGHT_5  PA15
#define PWM_LIGHT_6  PB3
#define PWM_LIGHT_7  PB0
#define PWM_LIGHT_8  PB1

#define led_on()  digitalWrite(LED_PIO, LOW)
#define led_off()  digitalWrite(LED_PIO, HIGH)

#define debug_on()  digitalWrite(DEBUG_PIO, HIGH)
#define debug_off()  digitalWrite(DEBUG_PIO, LOW)

#define relay_on()    digitalWrite(RELAY_PIO, LOW)
#define relay_off()   digitalWrite(RELAY_PIO, HIGH)

HardwareSerial Serial3(PB11, PB10);
StaticJsonDocument<1024> command;//1 KB
StaticJsonDocument<1024> response;//1 KB

uint32_t latches[8];
uint32_t intCount;
bool alive = false;

namespace level {
    const uint16_t min = 185;
    const uint16_t tim_per = 9900;
}

void start_timer_freq(TIM_TypeDef *Instance,uint32_t freq,callback_function_t callback){
  HardwareTimer *MyTim = new HardwareTimer(Instance);
  MyTim->setOverflow(freq, HERTZ_FORMAT);
  MyTim->attachInterrupt(callback);
  MyTim->resume();
}

void sync_irq(){

  //Timer down counting, small values get small pulses at the end, big pulses start sooner
  TIM1->CCR1 = latches[0];
  TIM1->CCR2 = latches[1];
  TIM1->CCR3 = latches[2];
  TIM1->CCR4 = latches[3];
  TIM1->CR1 |= TIM_CR1_CEN;       //Start the counter for one cycle

  TIM2->CCR1 = latches[4];
  TIM2->CCR2 = latches[5];
  TIM2->CR1 |= TIM_CR1_CEN;       //Start the counter for one cycle

  TIM3->CCR3 = latches[6];
  TIM3->CCR4 = latches[7];
  TIM3->CR1 |= TIM_CR1_CEN;       //Start the counter for one cycle

  intCount++;
}

void dimmer_init(){

  relay_off();
  latches[0] = 0;
  latches[1] = 0;
  latches[2] = 0;
  latches[3] = 0;
  latches[4] = 0;
  latches[5] = 0;
  latches[6] = 0;
  latches[7] = 0;

  pinMode(PWM_LIGHT_1, OUTPUT);  analogWrite(PWM_LIGHT_1, 0);
  pinMode(PWM_LIGHT_2, OUTPUT);  analogWrite(PWM_LIGHT_2, 0);
  pinMode(PWM_LIGHT_3, OUTPUT);  analogWrite(PWM_LIGHT_3, 0);
  pinMode(PWM_LIGHT_4, OUTPUT);  analogWrite(PWM_LIGHT_4, 0);
  pinMode(PWM_LIGHT_5, OUTPUT);  analogWrite(PWM_LIGHT_5, 0);
  pinMode(PWM_LIGHT_6, OUTPUT);  analogWrite(PWM_LIGHT_6, 0);
  pinMode(PWM_LIGHT_7, OUTPUT);  analogWrite(PWM_LIGHT_7, 0);
  pinMode(PWM_LIGHT_8, OUTPUT);  analogWrite(PWM_LIGHT_8, 0);

  //Timer 1---------------------------------------------------------
  TIM1->CR1 &= TIM_CR1_CEN;       //Stop the counter
  
  TIM1->CR1 &= ~TIM_CR1_CMS_Msk;  //stay with Edge alligned mode
  TIM1->CR1 |= TIM_CR1_OPM;       //One Pulse Mode
  TIM1->CR1 |= TIM_CR1_DIR;       //Down counting
  
  TIM1->PSC = 63;//[ 64 MHz / (63+1) ] = 1MHz => 1us
  TIM1->ARR = level::tim_per;//9900   10 ms / half period of 50 Hz

  //Timer 2---------------------------------------------------------
  TIM2->CR1 &= TIM_CR1_CEN;       //Stop the counter

  TIM2->CR1 &= ~TIM_CR1_CMS_Msk;  //stay with Edge alligned mode
  TIM2->CR1 |= TIM_CR1_OPM;       //One Pulse Mode
  TIM2->CR1 |= TIM_CR1_DIR;       //Down counting

  TIM2->PSC = 63;//[ 64 MHz / (63+1) ] = 1MHz => 1us
  TIM2->ARR = level::tim_per;//9900   10 ms / half period of 50 Hz

  //Timer 3---------------------------------------------------------
  TIM3->CR1 &= TIM_CR1_CEN;       //Stop the counter

  TIM3->CR1 &= ~TIM_CR1_CMS_Msk;  //stay with Edge alligned mode
  TIM3->CR1 |= TIM_CR1_OPM;       //One Pulse Mode
  TIM3->CR1 |= TIM_CR1_DIR;       //Down counting

  TIM3->PSC = 63;//[ 64 MHz / (63+1) ] = 1MHz => 1us
  TIM3->ARR = level::tim_per;//9900   10 ms / half period of 50 Hz

  //enable the sync
  pinMode(SYNC_PIO, INPUT);//sync
  attachInterrupt(digitalPinToInterrupt(SYNC_PIO),sync_irq,RISING);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);

}

void set_level(uint8_t channel,uint16_t value)
{
    if(value < level::min)
    {
        value = level::min;
    }
    if(value > level::tim_per)
    {
        value = level::tim_per;
    }
    //uint16_t delay = level::tim_per - (value - level::min);
    if(channel <= 7)
    {
        latches[channel] = value;
    }
}

void set_all_levels(uint16_t value)
{
  for(int i=0;i<8;i++){
    set_level(i,value);
  }
}

void startup_switchon()
{
    int vals[8];

    //185 - 9900
    for(int i=0;i<18000;i++)
    {
        for(int j=0;j<8;j++)        
        {
            vals[7-j] = i - (j*1000);//first gets counter, second is shifted by 1000,...
            if(vals[j] < 0)
            {
                vals[j] = 0;
            }
            //the set level is protected against max so no issues for first overflowing
            //the set_level is also latch protected, so no influence on change light count
            set_level(j,vals[j]);
            //if((i%1000) == 0){rasp.printf("%d ",vals[j]);}
        }
        //if((i%1000) == 0){rasp.printf("\r\n");}
        delayMicroseconds(80);// ~ 
    }
}

String process_message(String str_message){
  String resp_str;
  deserializeJson(command,str_message);

  if(command.containsKey("all")){
    uint16_t val = command["all"];
    set_all_levels(val);
  }else if(command.containsKey("list")){
    JsonArray array = command["list"].as<JsonArray>();
    int i=0;
    for(JsonVariant v : array) {
      uint16_t value = v.as<uint16_t>();
      set_level(i++,value);
    }
  }else if(command.containsKey("get")){
    String get = command["get"];
    if(get.compareTo("status") == 0){
      //nothing to do, as status always returned
    }
  }else{
    if(command.containsKey("L1")){set_level(0,command["L1"].as<uint16_t>());}
    if(command.containsKey("L2")){set_level(1,command["L2"].as<uint16_t>());}
    if(command.containsKey("L3")){set_level(2,command["L3"].as<uint16_t>());}
    if(command.containsKey("L4")){set_level(3,command["L4"].as<uint16_t>());}
    if(command.containsKey("L5")){set_level(4,command["L5"].as<uint16_t>());}
    if(command.containsKey("L6")){set_level(5,command["L6"].as<uint16_t>());}
    if(command.containsKey("L7")){set_level(6,command["L7"].as<uint16_t>());}
    if(command.containsKey("L8")){set_level(7,command["L8"].as<uint16_t>());}
  }

  response.clear();
  response["alive"] = alive;
  JsonArray array = response["list"].to<JsonArray>();
  for(int i=0;i<8;i++){
    array.add(latches[i]);
  }
  serializeJson(response,resp_str);
  return resp_str;
}

void sync_alive(){
  if(intCount != 0){
    alive = true;
  }else{
    if(!alive){//just died
      set_all_levels(0);
    }
    alive = false;
  }
  intCount = 0;
}

void setup()
{
  pinMode(LED_PIO, OUTPUT);       //led
  pinMode(DEBUG_PIO, OUTPUT);     //debug
  pinMode(RELAY_PIO, OUTPUT);     //relay


  Serial3.begin(115200);
  Serial3.setTimeout(200);

  dimmer_init();

  start_timer_freq(TIM4,5,sync_alive);

  //switch on - still might jitter depending on phase
  //TODO might think to sync the relay with the ISR
  relay_on();
  delay(300);

  set_all_levels(800);
  //startup_switchon();//max is too much for default, and should use ramps

}

void loop()
{
  if(Serial3.available()>0){
    String msg = Serial3.readString();//default 1000 ms timeout
    String response = process_message(msg);
    Serial3.println(response);
  }
  if(alive){
    led_off();
  }else{
    led_on();  delay(200);  led_off();delay(100);
    led_on();  delay(200);  led_off();delay(500);
  }
}
