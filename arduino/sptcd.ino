#include <stdio.h>
#include "SimpleTimer.h"
#include <Arduino.h>
#include <Wire.h>                        // Include Wire library (required for I2C devices)
#include <U8g2lib.h>
#include "RTClib.h"
#include "lsem.h"


//------------------------------------------------
//--- DEBUG  FLAGS     ---------------------------
//------------------------------------------------
// Check makefile

//------------------------------------------------
//--- GLOBAL VARIABLES ---------------------------
//------------------------------------------------



U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  
 


RTC_DS3231 rtc;
DateTime GLB_RTCnow;
float GLB_RTCtemp=0.0;

//unsigned long time;
SimpleTimer mainTimers;

int  GLB_timerMisc=-1;
bool GLB_timerMisc_expired=false;
#define TIMEOUT_BUTTON    2000

#define PIN_LED_STRIP  2
#define NUM_LEDS 1
CRGB GLBledStripBufferMem[NUM_LEDS];
LSEM GLBledStrip(GLBledStripBufferMem,NUM_LEDS);


#define PIN_BUTTON_RED     3
#define PIN_BUTTON_GREEN   4
#define PIN_BUTTON_BLUE    5


uint32_t RR=0;
uint32_t GG=0;
uint32_t BB=0;
uint32_t color=0;
bool bRedPressed   = false;
bool bGreenPressed = false;
bool bBluePressed  = false;

//------------------------------------------------
//--- GLOBAL FUNCTIONS ---------------------------
//------------------------------------------------

void display_idle();
bool display_welcome();
void display_button();

 

void STATE_welcome();
void STATE_idle();
void STATE_button();
void goto_idle();
// State pointer function
void (*GLBptrStateFunc)();


//------------------------------------------------
//-------    TIMER CALLBACKS     -----------------
//------------------------------------------------


//------------------------------------------------
void GLBcallbackLogging(void)
{
  Serial.println(F("DEBUG: still alive from timer logging..."));
}

//------------------------------------------------

#ifdef SPTCD_DEBUG_RTC
void GLBcallbackLoggingRTC(void)
{
  Serial.println(F("DEBUG: RTC..."));

  char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("GLB_HCdistance");

  // calculate a date which is 7 days and 30 seconds into the future
  DateTime future (now + TimeSpan(7,12,30,6));

  Serial.print(" now + 7d + 30s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();

  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");

  Serial.println();


}
#endif


//------------------------------------------------
void GLBcallbackTimeoutMisc(void)
{
  if (GLB_timerMisc != -1)     
    mainTimers.deleteTimer(GLB_timerMisc);
  GLB_timerMisc=-1;
  GLB_timerMisc_expired=true;
}


//------------------------------------------------
void resetTimerMisc(void)
{
  GLBcallbackTimeoutMisc();
  GLB_timerMisc_expired=false;
}


void GLBcallbackLoggingLedStrip(void)
{
  Serial.println(F("DEBUG: Led strip..."));
  GLBledStrip.setAllLeds(0xAABBCC);
}
//------------------------------------------------
//------------------------------------------------
//------------------------------------------------

void setup() { 
  // Serial to debug AND comunication protocolo with PI              
  Serial.begin(9600);
  Serial.println(F("BOOT"));

#ifdef SPTCD_DEBUG_STILLALIVE
  mainTimers.setInterval(2000,GLBcallbackLogging);
#endif 


#ifdef SPTCD_DEBUG_LS
  mainTimers.setInterval(10,GLBcallbackLoggingLedStrip);
#endif 


  FastLED.addLeds<WS2812B,PIN_LED_STRIP,GRB>(GLBledStripBufferMem,NUM_LEDS);

  pinMode(PIN_BUTTON_RED, INPUT_PULLUP);
  pinMode(PIN_BUTTON_GREEN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BLUE, INPUT_PULLUP);



#ifdef SPTCD_DEBUG_RTC
  mainTimers.setInterval(5000,GLBcallbackLoggingRTC);
#endif

  Serial.println(F("B RTC"));
  if (! rtc.begin()) {
    Serial.print(F("F!"));
  }
  if (rtc.lostPower()) {
    Serial.print(F("RTC!"));
  }

  Serial.println(F("B u8g2"));
  u8g2.begin(); 
  u8g2.setFontMode(0);		// enable transparent mode, which is faster
  u8g2.setDisplayRotation(U8G2_R2);


  resetTimerMisc();
  GLBptrStateFunc=STATE_welcome; 

  Serial.println(F("BD"));

}

//-------------------------------------------------
void goto_idle()
{
  
  u8g2.clearBuffer();
  #ifdef SPTCD_DEBUG_STATES
  Serial.print(F("NST_idle"));
  #endif
  resetTimerMisc();
  GLBptrStateFunc=STATE_idle; 
}


//-------------------------------------------------
void STATE_welcome()
{
  if (display_welcome())
    goto_idle();  
}

//-------------------------------------------------
void STATE_idle()
{
  display_idle();

  if (bBluePressed || bRedPressed || bGreenPressed ){  
    GLBptrStateFunc=STATE_button; 
    u8g2.clearBuffer();
  }
}

//-------------------------------------------------
void STATE_button()
{
  
  display_button();

  if   (GLB_timerMisc == -1)
  {
    GLB_timerMisc=mainTimers.setTimeout(TIMEOUT_BUTTON,GLBcallbackTimeoutMisc);
  }

  if (bBluePressed || bRedPressed || bGreenPressed )
  {
    resetTimerMisc();
  }

  if (GLB_timerMisc_expired){
    GLB_timerMisc_expired=false;
    goto_idle();
    return;
  }
}



//-------------------------------------------------------
 
char GLBstrTime[9]; //here to avoid stack overflow

void updateGLBstrTime(bool sec=false)
{

  GLBstrTime[5]     = 0;
  if (sec) {
    GLBstrTime[8]     = 0;
    GLBstrTime[7]     = GLB_RTCnow.second() % 10 + 48;
    GLBstrTime[6]     = GLB_RTCnow.second() / 10 + 48;
    GLBstrTime[5]     = ':';
  }
  GLBstrTime[4]     = GLB_RTCnow.minute() % 10 + 48;
  GLBstrTime[3]     = GLB_RTCnow.minute() / 10 + 48;
  GLBstrTime[2]     = ':';
  GLBstrTime[1]     = GLB_RTCnow.hour()   % 10 + 48;
  GLBstrTime[0]     = GLB_RTCnow.hour()   / 10 + 48;


}


void display_button(){
  u8g2.firstPage();
  do {
      u8g2.setFont(u8g2_font_logisoso30_tr);
      u8g2.setCursor(0,32);      
      u8g2.print(RR,HEX);
      u8g2.print(":");
      u8g2.print(GG,HEX);
      u8g2.print(":");
      u8g2.print(BB,HEX);
   } while ( u8g2.nextPage() );
}



bool GLBidleShowTime=false;
void display_idle(){
  bool bk=GLBidleShowTime;
  uint8_t animation=0;
  
  GLBidleShowTime=false;
  if (  (GLB_RTCnow.second() < 5) ||
        (GLB_RTCnow.second() >=20  && GLB_RTCnow.second() < 25) ||
        (GLB_RTCnow.second() >=40  && GLB_RTCnow.second() < 45)
     )
  {
    GLBidleShowTime=true;
  }

  animation=GLB_RTCnow.second()%5;



  if (bk != GLBidleShowTime)   u8g2.clearBuffer();

  updateGLBstrTime();

  u8g2.firstPage();
  do {
    if (GLBidleShowTime) {
      u8g2.setCursor(4,32);
      u8g2.setFont(u8g2_font_logisoso32_tr);
      u8g2.print(GLBstrTime);    
      for (uint8_t i=0;i<animation;i++)
        u8g2.drawCircle(110, 16,i*2, U8G2_DRAW_ALL);
   
    }
    else {
      u8g2.setFont(u8g2_font_logisoso32_tr);
      u8g2.setCursor(15,32);
      u8g2.print(GLB_RTCtemp,1);
      u8g2.drawCircle(90, 5, 3, U8G2_DRAW_ALL);
      u8g2.setCursor(100,32);
      u8g2.print("C");
      for (uint8_t i=0;i<animation;i++)
        u8g2.drawCircle(0, 0,i*3, U8G2_DRAW_ALL);

   }

  } while ( u8g2.nextPage() );



}

unsigned long t_starting_welcome=0;
int wsBK=0;
bool display_welcome(){
  unsigned long t;
  int ws=0;

  t=millis();
  char msg[6];
  for (int i=0;i<6;i++) msg[i]=' ';
  msg[5]=0;

  if (t_starting_welcome ==0)
    t_starting_welcome=t;
   
  t=t-t_starting_welcome;  

  if (t<100){
    msg[0]='H';  
    ws=0;}  
  else if (t<200){
    msg[1]='O';  
    ws=1;}    
  else if (t<300){
    msg[2]='L';  
    ws=2;}  
  else if (t<400){
    msg[3]='A';  
    ws=3;}  
  else if (t<500){
    msg[4]='!';  
    ws=4;}  
  else if (t<600){
    ws=5;}  
  else if (t<700){
    strcpy(msg,"HOLA!"); 
    ws=6;}  
  else if (t<1000){
    ws=7;}  
  else if (t<1100){
    strcpy(msg,"HOLA!"); 
    ws=8;}  
  else if (t<1500){
    return true; }

  if (wsBK != ws)   u8g2.clearBuffer();   
  wsBK=ws;
  
  u8g2.firstPage();
  do { 
    u8g2.setFont(u8g2_font_logisoso32_tr);	
    u8g2.setCursor(10,32);
    u8g2.print(msg);
    
  } while ( u8g2.nextPage() );
  
  return false;

}



//-------------------------------------------------
//-------------------------------------------------
//-------------------------------------------------

void loop() { 
#ifdef SPTCD_DEBUG_LOOP  
  Serial.print("+");
#endif
  //------------- INPUT REFRESHING ----------------
  // Let use ugly global variables in those costly sensors to save ram...


  GLB_RTCnow =rtc.now();
  GLB_RTCtemp=rtc.getTemperature();

  bRedPressed   = (digitalRead(PIN_BUTTON_RED)  ==LOW);
  bGreenPressed = (digitalRead(PIN_BUTTON_GREEN)==LOW);
  bBluePressed  = (digitalRead(PIN_BUTTON_BLUE) ==LOW);
    
  //--------- TIME TO THINK MY FRIEND -------------
  // State machine as main controller execution
#ifdef SPTCD_DEBUG_LOOP  
  Serial.print("S");
#endif
  GLBptrStateFunc();
#ifdef SPTCD_DEBUG_LOOP  
  Serial.print("T");
#endif
  mainTimers.run();

  if (bRedPressed)     {
#ifdef SPTCD_DEBUG_COLOR  
    Serial.print("R"); 
#endif 
    RR+=8;
    if (RR>=0xFF){
      RR=0xFF;
      GG-=8; if (GG>0xFF) GG=0;
      BB-=8; if (BB>0xFF) BB=0;     
    }
  }
  if (bGreenPressed)   {
#ifdef SPTCD_DEBUG_COLOR 
    Serial.print("G");  
#endif 
    GG+=8;
    if (GG>=0xFF){
      GG=0xFF;
      RR-=8; if (RR>0xFF) RR=0;
      BB-=8; if (BB>0xFF) BB=0;
    }

  }
  if (bBluePressed)    {
#ifdef SPTCD_DEBUG_COLOR  
    Serial.print("B");  
#endif 
    BB+=8;
    if (BB>=0xFF){
      BB=0xFF;
      RR-=8; if (RR>0xFF) RR=0;
      GG-=8; if (GG>0xFF) GG=0;

    }
  }


  if (bBluePressed && bRedPressed){
    RR=0; GG=0; BB=0;
  }

#ifdef SPTCD_DEBUG_COLOR
  uint32_t colorBK=color;


  Serial.print("RGB:");
  Serial.print(RR,HEX);
  Serial.print(GG,HEX);
  Serial.println(BB,HEX);


  Serial.print("RGB UINT:");
  Serial.println(color,HEX);
#endif
  color=(RR<<16) | (GG<<8) | BB;


#ifdef SPTCD_DEBUG_COLOR
  Serial.println(color,HEX);
  if (colorBK!=color){
    Serial.println(color,HEX);
    colorBK=color;
  }
#endif
  GLBledStrip.setAllLeds(color);

  // ------------- OUTPUT REFRESHING ---------------
  // In state
  FastLED.show();
}

