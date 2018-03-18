#include <math.h>
#include <TimeLib.h>
#include <LiquidCrystal.h>
#include <SDS011.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int pinRX = 6;
int pinTX = 7;
float pm10,pm25;
int error;

bool haveData = false;
int sleepTime = 1000; // Sleep 1 second between measurements
int averageResultTime = 60; // Get an average result every 60 seconds
time_t startSeconds = 0;
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

SDS011 mySDS;

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  mySDS.begin(pinRX, pinTX);
  
  Serial.begin(9600);
  Serial.println("Starting SDS011...");
  lcd.print("Starting...");
}

float AQI(float I_high, float I_low, float C_high, float C_low, float C) {
    return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

float getAverageDustDensity() {
    if (startSeconds == 0) {
        startSeconds = now();
    }
    mySDS.wakeup();
    error = mySDS.read(&pm25,&pm10);
    if (!error) {
        averagePm25DustDensity = (averagePm25DustDensity + pm25);
        Serial.println("PM2.5: " + String(pm25DustDensityToAQI(pm25)) + " (" + String(pm25) + ")");
        
        averagePm10DustDensity = (averagePm10DustDensity + pm10);
        Serial.println("PM10:  " + String(pm10DustDensityToAQI(pm10)) + " (" + String(pm10) + ")");
        
        divider++;

        // If there is no data, print the first values
        if (haveData == false) {
            lcd.setCursor(0, 0);
            lcd.clear();
            lcd.print("PM2.5: " + String((int)round(pm25DustDensityToAQI(pm25))));
            lcd.setCursor(0, 1);
            lcd.print("PM10:  " + String((int)round(pm10DustDensityToAQI(pm10))));

            haveData = true;
        }
    }
    
    if ((startSeconds != 0) && (now() >= (startSeconds + averageResultTime))) {
        finalPm25DustDensity = (averagePm25DustDensity / divider);
        finalPm10DustDensity = (averagePm10DustDensity / divider);

        averagePm25AQISum = (averagePm25AQISum + (int)round(pm25DustDensityToAQI(pm25)));
        averagePm10AQISum = (averagePm25AQISum + (int)round(pm10DustDensityToAQI(pm10)));
        averageAQIDivider++;
        if (averageAQIDivider == averageOverAQIValues) {
            averagePm25AQI = (averagePm25AQISum / averageAQIDivider);
            averagePm10AQI = (averagePm10AQISum / averageAQIDivider);

            averagePm25AQISum = 0;
            averagePm10AQISum = 0;
            averageAQIDivider = 0;
        }

        Serial.println("Average PM2.5: " + String(pm25DustDensityToAQI(finalPm25DustDensity)) + " (" + String(finalPm25DustDensity) + ")");
        lcd.setCursor(0,0);
        lcd.clear();
        lcd.print("PM2.5: " + String((int)round(pm25DustDensityToAQI(finalPm25DustDensity))));
        lcd.setCursor(15,0);
        lcd.print(getPm25AQIAverageSign());

        Serial.println("Average PM10:  " + String(pm10DustDensityToAQI(finalPm10DustDensity)) + " (" + String(finalPm10DustDensity) + ")");
        lcd.setCursor(0,1);
        lcd.print("PM10:  " + String((int)round(pm10DustDensityToAQI(finalPm10DustDensity))));
        lcd.setCursor(15,1);
        lcd.print(getPm10AQIAverageSign());
        
        startSeconds = now();
        averagePm25DustDensity = 0;
        averagePm10DustDensity = 0;
        finalPm25DustDensity = 0;
        averagePm10DustDensity = 0;
        divider = 0;
    }
}

String getPm10AQIAverageSign() {
    if ((int)pm10DustDensityToAQI(finalPm10DustDensity) < averagePm10AQI) {
        return "-"; 
    }
    else if ((int)pm10DustDensityToAQI(finalPm10DustDensity) == averagePm10AQI) {
        return "|"; 
    }
    else {
        return "+";
    }
}

String getPm25AQIAverageSign() {
    if ((int)pm25DustDensityToAQI(finalPm25DustDensity) < averagePm25AQI) {
        return "-"; 
    }
    else if ((int)pm25DustDensityToAQI(finalPm25DustDensity) == averagePm25AQI) {
        return "|"; 
    }
    else {
        return "+";
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

void loop() {
    getAverageDustDensity();
    mySDS.sleep();
    delay(sleepTime); 
}
