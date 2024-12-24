#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPingESP8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>

#define Baudrate 9600
#define X_LCD 16
#define Y_LCD 2

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht11(DHTPIN, DHTTYPE);

#define DS_PIN 23
OneWire oneWire(DS_PIN);
DallasTemperature sensors(&oneWire);

#define TRIG_PIN 18
#define ECHO_PIN 19
#define MaxDistance 300
NewPingESP8266 sonar(TRIG_PIN, ECHO_PIN, MaxDistance);

#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LED_YELLOW 13 // LED Yellow
#define LED_GREEN 11  // LED Green

#define RELAYPUMP_PIN 4 // Relay untuk Pompa Air
#define BUZZER_PIN 5    // Buzzer

int treshold = 0;

// blynk virtual pin
BLYNK_WRITE(V0)
{
  treshold = param.asInt(); // Membaca  V0 dari server blynk
}

int displayMode = 0; // 0 untuk suhu, 1 untuk kelembaban, 2 untuk data lainnya

int dhtProvider(String ValueOption)
{
  return (ValueOption == "humi")   ? dht11.readHumidity()
         : (ValueOption == "temp") ? dht11.readTemperature()
                                   : -1;
}
int dsTemp()
{
  sensors.requestTemperatures();
  int temp = sensors.getTempCByIndex(0); // ambil indek pertama;
  return temp;
}
int distanceSensor()
{
  return sonar.ping_cm();
}
// Template fungsi untuk menampilkan teks
template <typename T>
void displayText(int line, int column, T msg)
{
  lcd.setCursor(column, line); // Set posisi kursor
  lcd.print(msg);              // Cetak teks ke LCD
}
// kontrol pompa sistem
unsigned long previousMillis = 0;
const long intervalGreen = 1000;
const long intervalYellow = 500;

void kontrolPompa(int tempDS, int treshold)
{
  int intensity = map(tempDS, 0, 100, 0, 100);
  static bool ledGreenState = LOW;
  static bool ledYellowState = LOW;

  if (millis() - previousMillis >= min(intervalGreen, intervalYellow))
  {
    previousMillis = millis(); // Reset waktu

    // LED Hijau berkedip (setiap intervalGreen)
    if (millis() % intervalGreen == 0)
    {
      ledGreenState = !ledGreenState;
      digitalWrite(LED_GREEN, ledGreenState);
    }

    /** Treshold Zone: treshold - 3 s/d treshold + 3 **/
    if (intensity < (treshold - 3))
    {
      // Suhu rendah: Matikan semua indikator
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(RELAYPUMP_PIN, LOW);
    }
    else if (intensity > (treshold + 3))
    {
      // LED Kuning & Buzzer berkedip (setiap intervalYellow)
      if (millis() % intervalYellow == 0)
      {
        ledYellowState = !ledYellowState;
        digitalWrite(LED_YELLOW, ledYellowState);
        digitalWrite(BUZZER_PIN, ledYellowState);
      }
      digitalWrite(RELAYPUMP_PIN, HIGH); // Pompa aktif
    }
    else
    {
      // Zona Aman: Matikan indikator & pompa
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(RELAYPUMP_PIN, LOW);
    }
  }
}

// virtual pin v0 sebgai kontrol variabel treshold

// main function
void setup()
{
  Serial.begin(Baudrate);

  pinMode(DHTPIN, OUTPUT);
  dht11.begin();

  lcd.init(X_LCD, Y_LCD);
  lcd.backlight();
  sensors.begin();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
}
void loop()
{
  // put your main code here, to run repeatedly:

  if (displayMode == 0)
  {
    lcd.setCursor(1, 0);
    lcd.print("lingkungan: ");
    displayText(1, 13, dhtProvider("temp")); // tampilkan di kolom ke 13
    lcd.print(" C");
  }
  else if (displayMode == 1)
  {
    lcd.setCursor(1, 0);
    lcd.print("Kolam : ");
    displayText(1, 13, dsTemp()); // tampilkan di kolom ke 13
    lcd.print(" C");
  }
  else if (displayMode == 2)
  {
    lcd.setCursor(1, 0);
    lcd.print("Ketinggian : ");
    displayText(1, 13, distanceSensor()); // tampilkan di kolom ke 13
    lcd.print(" cm");
  }

  displayMode = (displayMode + 1) % 3; // Pergantian antara suhu, kelembaban, dan data lainnya

  // kirim data ke blynk v2
  Blynk.virtualWrite(2, dhtProvider("temp"));
  // kirim data ke blynk v3
  Blynk.virtualWrite(3, dsTemp());
  // kirim data ke blynk v4
  Blynk.virtualWrite(4, distanceSensor());

  int treshold = 27; // andaikan suhu batas == 27 C
  kontrolPompa(dsTemp(), treshold);
}
