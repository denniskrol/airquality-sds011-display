#include <math.h>
#include <TimeLib.h>
#include <LiquidCrystal.h>
#include <SDS011.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte upArrow[8] = {
    B00000,
    B00100,
    B01110,
    B10101,
    B00100,
    B00100,
    B00000,
};

byte downArrow[8] = {
    B00000,
    B00100,
    B00100,
    B10101,
    B01110,
    B00100,
    B00000,
};

int sds011Rx = 7;
int sds011Tx = 6;
int sds011Error;

bool haveData = false;
int sleepTime = 1000; // Sleep 1 second between measurements

float pm25CurrentValue, pm25Average1MinValue, pm25Average1MinSum, pm25Average15MinValue, pm25Average15MinSum;
float pm10CurrentValue, pm10Average1MinValue, pm10Average1MinSum, pm10Average15MinValue, pm10Average15MinSum;

int average1MinDivider, average15MinDivider;

time_t average1MinStart, average15MinStart;

float averagePm10DustDensity = 0;
float averagePm25DustDensity = 0;
float finalPm10DustDensity = 0;
float finalPm25DustDensity = 0;
int divider = 0;

int averageOverAQIValues = 15;
int averagePm10AQI = 0;
int averagePm10AQISum = 0;
int averagePm25AQI = 0;
int averagePm25AQISum = 0;
int averageAQIDivider = 0;

SDS011 sds011;

void setup() {
  lcd.begin(16,2);
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
  lcd.setCursor(0,0);
  sds011.begin(sds011Rx, sds011Tx);
  
  Serial.begin(9600);
  Serial.println("Starting SDS011...");
  
  lcd.print("Starting...");

  average1MinStart = now();
  average15MinStart = now();
}

float AQI(float I_high, float I_low, float C_high, float C_low, float C) {
    return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

float getAverageDustDensity() {
    sds011.wakeup();
    sds011Error = sds011.read(&pm25CurrentValue,&pm10CurrentValue);
    if (!sds011Error) {
        pm25Average1MinSum = (pm25Average1MinSum + pm25CurrentValue);
        pm25Average15MinSum = (pm25Average15MinSum + pm25CurrentValue);
       
        pm10Average1MinSum = (pm10Average1MinSum + pm10CurrentValue);
        pm10Average15MinSum = (pm10Average15MinSum + pm10CurrentValue);

        Serial.println("PM2.5: " + String(pm25DustDensityToAQI(pm25CurrentValue)) + " (" + String(pm25CurrentValue) + ")");
        Serial.println("PM10:  " + String(pm10DustDensityToAQI(pm10CurrentValue)) + " (" + String(pm10CurrentValue) + ")");
        
        average1MinDivider++;
        average15MinDivider++;

        // If there is no data, print the first values
        if (haveData == false) {
            // Set the average to current value to show initial values on LCD
            pm25Average1MinValue = pm25CurrentValue;
            pm10Average1MinValue = pm10CurrentValue;
            updateLCD();

            haveData = true;
        }
    }

    // Calculate and show 1 minute averages;
    if (now() >= (average1MinStart + 60)) {
        pm25Average1MinValue = (pm25Average1MinSum / average1MinDivider);
        pm10Average1MinValue = (pm10Average1MinSum / average1MinDivider);

        Serial.println("Average 1 min PM2.5: " + String(pm25DustDensityToAQI(pm25Average1MinValue)) + " (" + String(pm25Average1MinValue) + ")");
        Serial.println("Average 1 min PM10:  " + String(pm10DustDensityToAQI(pm10Average1MinValue)) + " (" + String(pm10Average1MinValue) + ")");

        updateLCD();
        
        average1MinStart = now();
        pm25Average1MinSum = 0;
        pm10Average1MinSum = 0;
        average1MinDivider = 0;
    }

    // Calculate 15 minute averages;
    if (now() >= (average15MinStart + 900)) {
        pm25Average15MinValue = (pm25Average15MinSum / average15MinDivider);
        pm10Average15MinValue = (pm10Average15MinSum / average15MinDivider);

        Serial.println("Average 15 min PM2.5: " + String(pm25DustDensityToAQI(pm25Average15MinValue)) + " (" + String(pm25Average15MinValue) + ")");
        Serial.println("Average 15 min PM10:  " + String(pm10DustDensityToAQI(pm10Average15MinValue)) + " (" + String(pm10Average15MinValue) + ")");

        updateLCD();
        
        average15MinStart = now();
        pm25Average15MinSum = 0;
        pm10Average15MinSum = 0;
        average15MinDivider = 0;
    }
}

void printPm10AQIAverageSign() {
    lcd.setCursor(15,1);
    if (!pm10Average15MinValue) {
        lcd.print(" ");
        
        return;
    }
    
    
    if ((int)pm10DustDensityToAQI(pm10Average1MinValue) < (int)pm10DustDensityToAQI(pm10Average15MinValue)) {
        lcd.write(byte(1));
    }
    else if ((int)pm10DustDensityToAQI(pm10Average1MinValue) > (int)pm10DustDensityToAQI(pm10Average15MinValue)) {
        lcd.write(byte(0));
    }
    else {
        lcd.print((char)B01111110);
    }
}

void printPm25AQIAverageSign() {
    lcd.setCursor(15,0);
    if (!pm25Average15MinValue) {
        lcd.print(" ");
        return;
    }
    
    if ((int)pm25DustDensityToAQI(pm25Average1MinValue) < (int)pm25DustDensityToAQI(pm25Average15MinValue)) {
        lcd.write(byte(1));
    }
    else if ((int)pm25DustDensityToAQI(pm25Average1MinValue) > (int)pm25DustDensityToAQI(pm25Average15MinValue)) {
         lcd.write(byte(0));
    }
    else {
        lcd.print((char)B01111110);
    }
}

float pm10DustDensityToAQI(float density) {
    int d10 = (int) (density * 10);

    if (d10 <= 0) {return 0;}
    else if (d10 <= 540) {return AQI(50, 0, 540, 0, d10);}
    else if (d10 <= 1540) {return AQI(100, 51, 1540, 550, d10);}
    else if (d10 <= 2540) {return AQI(150, 101, 2540, 1550, d10);}
    else if (d10 <= 3540) {return AQI(200, 151, 3540, 2550, d10);}
    else if (d10 <= 4240) {return AQI(300, 201, 4240, 3550, d10);}
    else if (d10 <= 5040) {return AQI(400, 301, 5040, 4250, d10);}
    else if (d10 <= 6040) {return AQI(500, 401, 6040, 5050, d10);}
    else if (d10 <= 10000) {return AQI(1000, 501, 10000, 6050, d10);}
    else {return 1001;}
}

float pm25DustDensityToAQI(float density) {
    int d10 = (int) (density * 10);

    if (d10 <= 0) {return 0;}
    else if (d10 <= 120) {return AQI(50, 0, 120, 0, d10);}
    else if (d10 <= 354) {return AQI(100, 51, 354, 121, d10);}
    else if (d10 <= 554) {return AQI(150, 101, 554, 355, d10);}
    else if (d10 <= 1504) {return AQI(200, 151, 1504, 555, d10);}
    else if (d10 <= 2504) {return AQI(300, 201, 2504, 1505, d10);}
    else if (d10 <= 3504) {return AQI(400, 301, 3504, 2505, d10);}
    else if (d10 <= 5004) {return AQI(500, 401, 5004, 3505, d10);}
    else if (d10 <= 10000) {return AQI(1000, 501, 10000, 5005, d10);}
    else {return 1001;}
}

void updateLCD() {
    lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.print("PM2.5: " + String((int)round(pm25DustDensityToAQI(pm25Average1MinValue))));
    printPm25AQIAverageSign();
    
    lcd.setCursor(0,1);
    lcd.print(" PM10: " + String((int)round(pm10DustDensityToAQI(pm10Average1MinValue))));
    lcd.setCursor(15,1);
    printPm10AQIAverageSign();
}

void loop() {
    getAverageDustDensity();
    sds011.sleep();
    delay(sleepTime); 
}
