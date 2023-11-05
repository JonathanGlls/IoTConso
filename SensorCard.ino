#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <driver/adc.h>

#define WLAN_SSID "iPhone de Tony"
#define WLAN_PASS "bastienbg"
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME "tonybeatini"
#define AIO_KEY "aio_bshO003qUX7C4Qkki4wguNQWpVU6"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish fire_detected = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/fire-detected");

void MQTT_connect();

void setup() {
    Serial.begin(9600);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

void loop() {
    bool alarm;
    int value = adc1_get_raw(ADC1_CHANNEL_0);
    float voltage = (float)value*1100./4095.;
    float temperature = voltage/10;
    Serial.println("Température (en °C) : " + String(temperature));
    MQTT_connect();

    if (! temp.publish(temperature)) {
        Serial.println(F("Failed"));
    } else {
        Serial.println(F("OK!"));
    }

    if (temperature > 20){
        alarm = 1;
    }
    else {
        alarm = 0;
    }

    if (! fire_detected.publish(alarm)) {
        Serial.println(F("Failed"));
    } else {
        Serial.println(F("OK!"));
    }

    delay(10000);
}

void MQTT_connect() {
    int8_t ret;
    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }
    Serial.print("Connecting to MQTT... ");
    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0) {
            // basically die and wait for WDT to reset me
            while (1);
        }
    }
    Serial.println("MQTT Connected!");
}