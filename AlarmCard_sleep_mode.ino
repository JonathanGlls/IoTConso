#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <driver/adc.h>

// Configuration Wi-Fi et MQTT
#define WLAN_SSID "iPhone de Tony"
#define WLAN_PASS "bastienbg"
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883 // Utiliser 8883 pour SSL
#define AIO_USERNAME "tonybeatini"
#define AIO_KEY "aio_bshO003qUX7C4Qkki4wguNQWpVU6"

// Client Wi-Fi et objets MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe fire_detected = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/fire-detected");
Adafruit_MQTT_Publish alarm_p = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/alarm-state");
Adafruit_MQTT_Subscribe alarm_r = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/alarm-state");

void MQTT_connect();

// Broches matérielles
const int buttonPin = 19;
const int ledPin = 4;
const int buzzerPin = 13;

// Variables d'état
bool press = false;
bool alert = false;

void setup() {
  Serial.begin(9600);

  // Configuration des broches en entrée/sortie et interruption
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);

  // Connexion Wi-Fi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());

  // Abonnement aux sujets MQTT
  mqtt.subscribe(&fire_detected);
  mqtt.subscribe(&alarm_r);

  // Configuration de la mise en veille profonde (deep sleep)
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_mS_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_19, 1);
}

void loop() {
  // Fonction pour gérer la connexion MQTT
  MQTT_connect();

  if (alert == false || press == true) {
    noTone(buzzerPin);
    digitalWrite(ledPin, LOW);
    Serial.println("ALERT = FALSE");
    Serial.println("Étape 1");

    if (press == true) {
      alert = true;
      press = false;
    } else {
      Serial.println("Étape 2");
      Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription(5000))) {
        if (subscription == &fire_detected) {
          if (String((char *)fire_detected.lastread) == String("1")) {
            Serial.print("INCENDIE DÉTECTÉ !");
            alert = true;
          }
        }
      }
    }

    if (alert) {
      Serial.println("Étape 3");
      tone(buzzerPin, 500);
      digitalWrite(ledPin, HIGH);
      Serial.println("ALARME DÉCLENCHEÉ");
    } else {
      Serial.println("Étape 4");
      Serial.println("AUCUN DANGER DÉTECTÉ");
    }
  } else {
    Serial.println("ALERT = TRUE");

    // Publication d'un état d'alarme
    if (!alarm_p.publish(1)) {
      Serial.println("Échec de la publication");
    } else {
      Serial.println("OK");
    }

    while (alert == true) {
      Serial.println("Étape 5");
      Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription(5000))) {
        if (subscription == &alarm_r) {
          if (String((char *)alarm_r.lastread) == String("0")) {
            Serial.print("ARRÊT DE L'ALARME !");
            alert = false;
            break;
          }
        }
      }
    }
  }
  if (alert == false) {
    esp_light_sleep_start(); // Met en veille légère si l'alarme est désactivée
  }
}

void buttonInterrupt() {
  press = true;
}

void MQTT_connect() {
  int8_t ret;
  // Arrête si déjà connecté.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connexion à MQTT... ");
  uint8_t retries = 20;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println("Nouvelle tentative de connexion MQTT dans 5 secondes...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      while (1);
    }
  }
  Serial.println("MQTT Connecté !");
}
