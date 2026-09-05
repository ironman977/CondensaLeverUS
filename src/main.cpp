#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

#define trigPin D6
#define echoPin D7

#define wifi_ssid "WIFIHOME2"
#define wifi_password "8648002233"
#define DEEP_SLEEP_DURATION 15*60e6 // 15 minuti in microsecondi
#define SAMPLE_COUNT 5
#define TELEGRAM_BOT_TOKEN "7912486058:AAGefrulYuR3JXMkCqzW_XuEbhqTO3-0vZY"
#define CHAT_ID "5006717867"
#define WARNING_DISTANCE 60 // distanza in mm per inviare l'allarme
#define ALARM_DISTANCE 30 // distanza in mm per inviare l'allarme
#define HISTERESIS 15 // distanza in mm per evitare falsi allarmi

// salva in eeprom se il segnale di warning è stato inviato o meno
#define EEPROM_WARNING_SENT_ADDRESS 0
#define EEPROM_SIZE 4
bool warningSent = false;

void saveFlagIfChanged(bool value) {
  if (EEPROM.read(EEPROM_WARNING_SENT_ADDRESS) != (value ? 1 : 0)) {
    EEPROM.write(EEPROM_WARNING_SENT_ADDRESS, value ? 1 : 0);
    EEPROM.commit();
  }
}

bool readWarningSentFromEEPROM() {
  bool sent = EEPROM.read(EEPROM_WARNING_SENT_ADDRESS) == 1;
  EEPROM.end();
  return sent;
}


long durata, cm, mm;
bool sendWelcomeMessage = false;

long readBatteryVoltage() {
  int analogValue = analogRead(A0);
  float voltage = (float)analogValue * 0.0041016; // Converti il valore analogico in tensione VREF=1V e risoluzione 10 bit (1024 livelli)
  return voltage * 1000; // Restituisci la tensione in millivolt
}

// Connette il modulo alla rete WiFi come ts_client (station)
void connectWiFi() {
  Serial.println();
  Serial.print("Connessione a ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
      delay(500);
      yield();
      Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connesso, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Connessione WiFi fallita");
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  warningSent = readWarningSentFromEEPROM();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(A0, INPUT);

  connectWiFi();
}

int misuraDurata() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  return pulseIn(echoPin, HIGH, 30000);
}

void sendTelegramMessage(const String& message) {
  WiFiClientSecure telegram_client;
  telegram_client.setInsecure();
  telegram_client.setTimeout(3000);

  if (telegram_client.connect("api.telegram.org", 443)) {
    String path = "/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
    String body = "chat_id=" + String(CHAT_ID) + "&text=" + message;
    telegram_client.print(String("POST ") + path + " HTTP/1.1\r\n" +
                          "Host: api.telegram.org\r\n" +
                          "Content-Type: application/x-www-form-urlencoded\r\n" +
                          "Content-Length: " + String(body.length()) + "\r\n" +
                          "Connection: close\r\n\r\n" + body);
    delay(100);
    while(telegram_client.available()) {
      String line = telegram_client.readStringUntil('\n');
      Serial.println(line);
    }
    telegram_client.stop();
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi non connesso, tentativo di riconnessione...");
    connectWiFi();
    yield();
  }

  // Misura la distanza 3 volte e calcola la media scartando la misura più alta e quella più bassa
  long misure[SAMPLE_COUNT];
  int index = 0;
  while (index < SAMPLE_COUNT) {
    long misura = misuraDurata();
    if (misura > 0) {
      misure[index] = misura;
      index++;
    }
    delay(50);
  }
  // calcola la media scartando la misura più alta e quella più bassa
  long somma = 0;
  long min = misure[0];
  long max = misure[0];
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    somma += misure[i];
    if (misure[i] < min) {
      min = misure[i];
    }
    if (misure[i] > max) {
      max = misure[i];
    }
  }
  somma = somma - min - max;
  long durata = somma / (SAMPLE_COUNT - 2); // media delle misure rimanenti
 

  long cm = durata / 58;
  long mm = durata / 5.8;
  long batteryVoltage = readBatteryVoltage();
  


  Serial.print("Cm = ");
  Serial.println(cm);
  Serial.print("Mm = ");
  Serial.println(mm);
  Serial.print("Battery Voltage (mV) = ");
  Serial.println(batteryVoltage);

  if (! sendWelcomeMessage) {
    String welcomeMessage = "Sistema di monitoraggio della distanza attivo. Distanza attuale: " + String(mm) + " mm, Tensione batteria: " + String(batteryVoltage) + " mV. " +
    "Invio dati a ThingSpeak e monitoraggio allarmi attivo. Letture ogni " + String(DEEP_SLEEP_DURATION / 60000 / 1000) + " minuti. " +
    " Soglia di warning: " + String(WARNING_DISTANCE) + " mm, soglia di allarme: " + String(ALARM_DISTANCE) + " mm." +
    " Histeresi: " + String(HISTERESIS) + " mm. Indirizzo IP del dispositivo: " + WiFi.localIP().toString();
    sendTelegramMessage(welcomeMessage);
    sendWelcomeMessage = true;
  }

  WiFiClient ts_client;
  WiFiClientSecure telegram_client;
  telegram_client.setInsecure();
  telegram_client.setTimeout(3000);
  ts_client.setTimeout(3000);

  if (ts_client.connect("api.thingspeak.com", 80)) {
      String postStr = "api_key=2BZSTXSJHG3YMU11&field1=";
      postStr += String(cm);
      postStr += "&field2=";
      postStr += String(mm);
      postStr += "&field3=";
      postStr += String(batteryVoltage);
      postStr += "\r\n\r\n";

      ts_client.print("POST /update HTTP/1.1\r\n");
      ts_client.print("Host: api.thingspeak.com\r\n");
      ts_client.print("Connection: close\r\n");
      ts_client.print("Content-Type: application/x-www-form-urlencoded\r\n");
      ts_client.print("Content-Length: ");
      ts_client.print(postStr.length());
      ts_client.print("\r\n\r\n");
      ts_client.print(postStr);
      delay(100);

      while(ts_client.available()) {
          String line = ts_client.readStringUntil('\n');
          Serial.println(line);
      }      
      ts_client.stop();

      Serial.println("Dati inviati");
  }

  // Controlla se la distanza è inferiore alla soglia di warning o di allarme e invia un messaggio su Telegram se necessario
  if (mm < (WARNING_DISTANCE) && mm > (ALARM_DISTANCE)) {
    if (!warningSent) {
      if (telegram_client.connect("api.telegram.org", 443)) {
        String message = "WARNING: distanza inferiore a " + String(WARNING_DISTANCE) + " mm!";
        String path = "/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
        String body = "chat_id=" + String(CHAT_ID) + "&text=" + message;
        telegram_client.print(String("POST ") + path + " HTTP/1.1\r\n" +
                              "Host: api.telegram.org\r\n" +
                              "Content-Type: application/x-www-form-urlencoded\r\n" +
                              "Content-Length: " + String(body.length()) + "\r\n" +
                              "Connection: close\r\n\r\n" + body);
        delay(100);
        while(telegram_client.available()) {
          String line = telegram_client.readStringUntil('\n');
          Serial.println(line);
        }
        telegram_client.stop();
        warningSent = true;
        saveFlagIfChanged(warningSent);
        Serial.println("Messaggio di warning inviato e stato salvato in EEPROM");
      }
    } else {
      Serial.println("Messaggio di warning già inviato, non invio nuovamente");
    }
  } else if (mm > (ALARM_DISTANCE + HISTERESIS)) {
    warningSent = false;
    saveFlagIfChanged(warningSent);
    Serial.println("Distanza superiore alla soglia di warning, stato di warning resettato e salvato in EEPROM");
  }
  // Controlla se la distanza è inferiore alla soglia di allarme e invia un messaggio su Telegram se necessario senza memorizzare lo stato di warning
  if (mm < (ALARM_DISTANCE)) {
      String message = "ALARM: distanza inferiore a " + String(ALARM_DISTANCE) + " mm!";
      sendTelegramMessage(message);
      Serial.println("Messaggio di allarme inviato");
  }
  
  Serial.println("vado in deep sleep");
  ESP.deepSleep(DEEP_SLEEP_DURATION); // 15 minuti (il parametro è in microsecondi)

  yield();  

}