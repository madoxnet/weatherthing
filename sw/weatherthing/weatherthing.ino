#include <BME280I2C.h>
#include "SSD1306.h"

//OLED Screen
SSD1306  display(0x3c, 4, 5);
char screen_buf[100]="";

//Temperature sensors and vaiarbles
BME280I2C bme_i(1,1,1,3,5,0,0,0x77);
BME280I2C bme_o(1,1,1,3,5,0,0,0x76);

uint8_t bme_i_detected=0;
uint8_t bme_o_detected=0;
uint8_t pressure_unit=1; //hPa
bool metric=true;

float temp(NAN), hum(NAN), pres(NAN), alt(NAN), dew(NAN);

void setup() {
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  Serial.println();
  bme_i_detected = bme_i.begin();
  bme_o_detected = bme_o.begin();
  Serial.print("Inside :");
  Serial.println(bme_i_detected);
  Serial.print("Outside :");
  Serial.println(bme_o_detected);
}

void loop() {
  //sprintf(screen_buf, "Temp: %3.2f\nHumidity: %3.2f\nPressure: %5.2f", temp, hum, pres);
  sprintf(screen_buf, "Temp: %d.%02d\nHumidity: %2d\nPressure: %4d", (int)temp,(int)(temp*100)%100, (int)hum, (int)pres);
  Serial.println(screen_buf);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Shauna");
  display.drawString(0,16,"Temp");
  display.drawString(0,32,"Humidity");
  display.drawString(0,48,"Pressure");
  
  if(bme_i_detected){
    bme_i.read(pres, temp, hum, metric, pressure_unit);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(85, 0, "Inside");
    display.drawString(85,16,String(temp));
    display.drawString(85,32,String(hum));
    display.drawString(85,48,String(pres));
  }
  if(bme_o_detected){
     bme_o.read(pres, temp, hum, metric, pressure_unit);
    display.drawString(128, 0, "Outside");
    display.drawString(128,16,String(temp));
    display.drawString(128,32,String(hum));
    display.drawString(128,48,String(pres));
    display.display();
  }
  
  delay(1000);
}
