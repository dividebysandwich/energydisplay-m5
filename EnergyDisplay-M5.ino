#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "bgcolor.h"

const char* ssid     = "";
const char* password = "";

const char* host = "";

const int GRAPH_SIZE = 130;
int pvGraph[GRAPH_SIZE];
int useGraph[GRAPH_SIZE];
int displayMode = 1;
uint32_t lastLoadTime = 0;
uint32_t lastAnimationTime = 0;

float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}
  
bool readPastHeader(WiFiClient *pClient)
{
 bool bIsBlank = true;
 while(true)
 {
   if (pClient->available()) 
   {
     char c = pClient->read();
     if(c=='\r' && bIsBlank)
     {
       // throw away the /n
       c = pClient->read();
       return true;
     }
     if(c=='\n')
       bIsBlank = true;
     else if(c!='\r')
       bIsBlank = false;
     }
  }
}

void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

int value = 0;
int useGraphX1 = 0;
int useGraphY1 = 0;
int useGraphX2 = useGraphX1 + 130;
int useGraphY2 = useGraphY1 + 60;
int maxUse = 0;
int battgraphmax = 0;
String battery = "0";
String pv = "0";
String use = "0";
String grid = "0";
String battuse = "0";
String curtime = "0";
String curdate = "0";


void loadData()
{
    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    Serial.println("Getting Data");
    // This will send the request to the server
    client.print(String("GET ") + "/status/soc.txt" + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: Keep-Alive\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    if (client.available()) {
        Serial.println("Data received!");
        readPastHeader(&client);
        battery = client.readStringUntil('\n');
        pv = client.readStringUntil('\n');
        use = client.readStringUntil('\n');
        grid = client.readStringUntil('\n');
        battuse = client.readStringUntil('\n');
        curtime = client.readStringUntil('\n');
        curdate = client.readStringUntil('\n');

        if (pv == "-0")
          pv = "0";
        if (grid == "-0")
          grid = "0";
        if (use == "-0")
          use = "0";
        if (battuse == "-0")
          battuse = "0";
       
      battgraphmax = (int)(battery.toFloat()/100.0*75.0);
    }
    
    Serial.println("Getting PV Histogram");
    client.print(String("GET ") + "/status/lastpv.txt" + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: Keep-Alive\r\n\r\n");
    timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    if (client.available()) {
        readPastHeader(&client);
        Serial.println("PV Histogram received!");

        int x=0;
        while (client.available() && x < 130) {
          float curpvf = client.readStringUntil('\n').toFloat();
          pvGraph[x] = (int)curpvf;
          x++;
        }
    }

    Serial.println("PV Histogram completed!");

    Serial.println("Getting Usage Histogram");
    client.print(String("GET ") + "/status/lastuse.txt" + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    if (client.available()) {
        readPastHeader(&client);
        Serial.println("Usage Histogram received!");

        
        int x=0;
        while (x < 130) {
          if (client.available()) {
            float u = client.readStringUntil('\n').toFloat();
            useGraph[x] = (int)u;
          } else {
            useGraph[x] = 0;
          }
          if (maxUse < useGraph[x]) {
            maxUse = useGraph[x];
          }
          x++;
        }

        if (maxUse < 2000) {
          maxUse = 2000;
        }
        
    }
    Serial.println("Usage Histogram completed!");

 
    Serial.println();
    Serial.println("closing connection");
    client.stop();
//    WiFi.disconnect();
//    WiFi.mode(WIFI_OFF);
}


void drawHistogramUse()
{
  int hx = 0;
  int hy = 110;
  M5.Lcd.fillScreen(TFT_BLACK);
  for (float a=1000.0; a<(float)maxUse; a+=1000.0) {
    M5.Lcd.drawLine(hx+1, hy+120-(int)(a/(float)maxUse*120.0f), hx+260, hy+120-(int)(a/(float)maxUse*120.0f), TFT_DARKGREY);
  }
  M5.Lcd.drawRect(hx, hy, 261, 121, TFT_WHITE);
  int x=1;
  int i=0;
  while (i < 130) {
    float curpvf = (float)useGraph[i];
    float maxy = curpvf / (float)maxUse * 120.0;
    M5.Lcd.drawLine(hx+ x, hy+120, hx+x, hy+120-(int)maxy, TFT_GREEN);
    i++;
    x+=2;
  }
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor( 270, 110);
  M5.Lcd.print(String((float)maxUse / 1000, 1));
  M5.Lcd.setCursor( 270, 217);
  M5.Lcd.print("0");

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor( 20, 30);
  M5.Lcd.print("Use: "+use+" kW");

}

void drawHistogramPV()
{
  int hx = 0;
  int hy = 110;
  M5.Lcd.fillScreen(TFT_BLACK);
  for (float a=1000.0; a<=7000.0; a+=1000.0) {
    M5.Lcd.drawLine(hx+1, hy+120-(int)(a/7800.0*120.0f), hx+260, hy+120-(int)(a/7800.0*120.0f), TFT_DARKGREY);
  }
  M5.Lcd.drawRect(hx, hy, 261, 121, TFT_WHITE);
  int x=1;
  int i=0;
  while (i < 130) {
    float curpvf = (float)pvGraph[i];
    float maxy = curpvf / 7800.0 * 120.0;
    M5.Lcd.drawLine(hx+ x, hy+120, hx+x, hy+120-(int)maxy, TFT_YELLOW);
    i++;
    x+=2;
  }
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor( 270, 110);
  M5.Lcd.print("7.8");
  M5.Lcd.setCursor( 270, 217);
  M5.Lcd.print("0");

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor( 30, 30);
  M5.Lcd.print("PV: "+pv+" kW");

}

void drawHistogramUseSmall()
{
  int hx = 0;
  int hy = 175;
  for (float a=1000.0; a<(float)maxUse; a+=1000.0) {
    M5.Lcd.drawLine(hx+1, hy+60-(int)(a/(float)maxUse*60.0f), hx+130, hy+60-(int)(a/(float)maxUse*60.0f), TFT_DARKGREY);
  }
  M5.Lcd.drawRect(hx, hy, 131, 62, TFT_WHITE);
  int x=1;
  while (x < 130) {
    float curpvf = (float)useGraph[x];
    float maxy = curpvf / (float)maxUse * 60.0;
    M5.Lcd.drawLine(hx+ x, hy+60, hx+x, hy+60-(int)maxy, TFT_GREEN);
    x++;
  }
}

void drawHistogramPVSmall()
{
  int hx = 0;
  int hy = 0;
  for (float a=1000.0; a<=7000.0; a+=1000.0) {
    M5.Lcd.drawLine(hx+1, hy+60-(int)(a/7800.0*60.0f), hx+130, hy+60-(int)(a/7800.0*60.0f), TFT_DARKGREY);
  }
  M5.Lcd.drawRect(hx, hy, 131, 62, TFT_WHITE);
  int x=1;
  while (x < 130) {
    float curpvf = (float)pvGraph[x];
    float maxy = curpvf / 7800.0 * 60.0;
    M5.Lcd.drawLine(hx+ x, hy+60, hx+x, hy+60-(int)maxy, TFT_YELLOW);
    x++;
  }
}

void drawBatteryGaugeSmall()
{
  M5.Lcd.fillRect(14, 160 - battgraphmax, 40, battgraphmax, TFT_BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(17, 115);
  if (battery == "100") {
    M5.Lcd.print("99%");
  } else {
    M5.Lcd.print(battery +"%");
  }
}

void drawOverview(bool drawHistograms)
{
  M5.Lcd.drawBitmap(0, 0, 320, 240, bgimage);
  if (drawHistograms) {
    drawHistogramPVSmall();
    drawHistogramUseSmall();
  }
  drawBatteryGaugeSmall();
  
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setCursor(210, 15);
  M5.Lcd.print(pv +"kW");

  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.setCursor(210, 200);
  M5.Lcd.print(use+"kW");
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_BLUE);
  M5.Lcd.setCursor(75, 75);
  M5.Lcd.print(battuse +"kW");

  M5.Lcd.setTextColor(TFT_RED);
  M5.Lcd.setCursor(210, 75);
  M5.Lcd.print(grid+"kW");

}

int animationOffset = 0;
int maxAnimationOffset = 140;
void drawHorizontalPowerAnimation(int startx, int endx, int starty, int endy, int num_dots, int dir)
{
  if (dir == 0) {
    int x = 0;
    while (x < (endx - startx)) {
      if (x == animationOffset || (num_dots > 1 && x == animationOffset - 15) || (num_dots > 2 && x == animationOffset - 30) || (num_dots > 3 && x == animationOffset - 45) ) {
        M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
        x++;
        if (x < (endx - startx)) {
          M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
          x++;
        }
        if (x < (endx - startx)) {
          M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
          x++;
        }
      } else {
        M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_DARKGREY);
        x++;
      }
    }
  } else {
    int x = (endx - startx);
    while (x > 0) {
      if (x == (maxAnimationOffset-animationOffset) || (num_dots > 1 && x == (maxAnimationOffset-animationOffset) - 15) || (num_dots > 2 && x == (maxAnimationOffset-animationOffset) - 30) || (num_dots > 3 && x == (maxAnimationOffset-animationOffset) - 45)) {
        M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
        x--;
        if (x > 0) {
          M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
          x--;
        }
        if (x > 0) {
          M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_WHITE);
          x--;
        }
      } else {
        M5.Lcd.drawLine(x+startx, starty, x+startx, endy, TFT_DARKGREY);
        x--;
      }
    }
  }
}

void drawVerticalPowerAnimation(int startx, int endx, int starty, int endy, int num_dots, int dir)
{
  if (dir == 0) {
    int y = 0;
    while (y < (endy - starty)) {
      if (y == animationOffset || (num_dots > 1 && y == animationOffset - 15) || (num_dots > 2 && y == animationOffset - 30) || (num_dots > 3 && y == animationOffset - 45) ) {
        M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
        y++;
        if (y < (endy - starty)) {
          M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
          y++;
        }
        if (y < (endy - starty)) {
          M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
          y++;
        }
      } else {
        M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_DARKGREY);
        y++;
      }
    }
  } else {
    int y = (endy - starty);
    while (y > 0) {
      if (y == (maxAnimationOffset-animationOffset) || (num_dots > 1 && y == (maxAnimationOffset-animationOffset) - 15) || (num_dots > 2 && y == (maxAnimationOffset-animationOffset) - 30) || (num_dots > 3 && y == (maxAnimationOffset-animationOffset) - 45)) {
        M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
        y--;
        if (y > 0) {
          M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
          y--;
        }
        if (y > 0) {
          M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_WHITE);
          y--;
        }
      } else {
        M5.Lcd.drawLine(startx, y+starty, endx, y+starty, TFT_DARKGREY);
        y--;
      }
    }
  }
}


void advanceAnimation()
{
  animationOffset++;
  if (animationOffset > maxAnimationOffset) {
    animationOffset = 0;
  }
}

void handleInput()
{
  lastAnimationTime = 0;
  switch(displayMode) {
    case 0: drawOverview(false); break;
    case 1: drawOverview(true); break;
    case 2: drawHistogramPV(); break;
    case 3: drawHistogramUse(); break;
  }
}

void setup()
{
    M5.begin();
    M5.setWakeupButton(BUTTON_B_PIN);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("Connecting...");
    if (WiFi.status() != WL_CONNECTED) {
      wifiReconnect();
    }
    M5.Lcd.println("Loading...");
    loadData();
    handleInput();
}

void loop()
{
    if(M5.BtnB.wasPressed()) {
      M5.powerOFF();
    } else if(M5.BtnA.wasPressed()) {
      displayMode--;
      if (displayMode < 0) {
        displayMode = 3;
      }
      handleInput();
    } else if(M5.BtnC.wasPressed()) {
      displayMode++;
      if (displayMode > 3) {
        displayMode = 0;
      }
      handleInput();
    }

    if ((displayMode == 0 || displayMode == 1) && millis() > lastAnimationTime + 5) {

      //Battery usage animation
      int numBattArrows = 0;
      int battDirection = 0;
      float fbattuse = battuse.toFloat();
      if (fbattuse < -0.1) {
        if (fbattuse >= -0.7) {
          numBattArrows = 1;
        } else if (fbattuse >= -1.4) {
          numBattArrows = 2;
        } else {
          numBattArrows = 3;
        }
        battDirection = 0;
        drawHorizontalPowerAnimation(62, 135, 118, 123, numBattArrows, battDirection);
      } else if (fbattuse > 0.1) {
        if (fbattuse <= 0.7) {
          numBattArrows = 1;
        } else if (fbattuse <= 1.4) {
          numBattArrows = 2;
        } else {
          numBattArrows = 3;
        }
        battDirection = 1;
        drawHorizontalPowerAnimation(62, 135, 118, 123, numBattArrows, battDirection);
    }

      //Grid usage animation
      int numGridArrows = 0;
      int gridDirection = 0;
      float fgrid = grid.toFloat();
      if (fgrid < -0.1) {
        if (fgrid >= -1.0) {
          numGridArrows = 1;
        } else if (fgrid >= -2.0) {
          numGridArrows = 2;
        } else if (fgrid >= -6.0) {
          numGridArrows = 3;
        } else {
          numGridArrows = 4;
        }
        gridDirection = 0;
        drawHorizontalPowerAnimation(185, 272, 118, 123, numGridArrows, gridDirection);
      } else if (fgrid > 0.1) {
        if (fgrid <= 1.0) {
          numGridArrows = 1;
        } else if (fgrid <= 2.0) {
          numGridArrows = 2;
        } else if (fgrid <= 6.0) {
          numGridArrows = 3;
        } else {
          numGridArrows = 4;
        }
        gridDirection = 1;
        drawHorizontalPowerAnimation(185, 272, 118, 123, numGridArrows, gridDirection);
      }

      //PV usage animation
      int numPVArrows = 0;
      int pvDirection = 0;
      float fpv = pv.toFloat();
      if (fpv < -0.1) {
        if (fpv >= -1.0) {
          numPVArrows = 1;
        } else if (fpv >= -2.0) {
          numPVArrows = 2;
        } else if (fpv >= -6.0) {
          numPVArrows = 3;
        } else {
          numPVArrows = 4;
        }
        pvDirection = 1;
        drawVerticalPowerAnimation(158, 162, 43, 96, numPVArrows, pvDirection);
      } else if (fpv > 0.1) {
        if (fpv <= 1.0) {
          numPVArrows = 1;
        } else if (fpv <= 2.0) {
          numPVArrows = 2;
        } else if (fpv <= 6.0) {
          numPVArrows = 3;
        } else {
          numPVArrows = 4;
        }
        pvDirection = 0;
        drawVerticalPowerAnimation(158, 162, 43, 96, numPVArrows, pvDirection);
      }

      //house consumer usage animation
      int numUseArrows = 0;
      int useDirection = 0;
      float fuse = use.toFloat();
      if (fuse < -0.1) {
        if (fuse >= -1.0) {
          numUseArrows = 1;
        } else if (fuse >= -2.0) {
          numUseArrows = 2;
        } else if (fuse >= -6.0) {
          numUseArrows = 3;
        } else {
          numUseArrows = 4;
        }
        useDirection = 1;
        drawVerticalPowerAnimation(158, 162, 145, 185, numUseArrows, useDirection);
      } else if (fuse > 0.1) {
        if (fuse <= 1.0) {
          numUseArrows = 1;
        } else if (fuse <= 2.0) {
          numUseArrows = 2;
        } else if (fuse <= 6.0) {
          numUseArrows = 3;
        } else {
          numUseArrows = 4;
        }
        useDirection = 0;
        drawVerticalPowerAnimation(158, 162, 145, 185, numUseArrows, useDirection);
      }


      advanceAnimation();
      lastAnimationTime = millis();
    }
    
    if (millis() > lastLoadTime + (1000 * 60)) {
      if (WiFi.status() != WL_CONNECTED) {
        wifiReconnect();
      }
      loadData();
      handleInput();
      lastLoadTime = millis();
    }
    M5.update();
}

