#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPert.h>

const char* ssid = "*******";
const char* password = "********";
const char* ota_hostname = "appstackesp";
const char* mqtt_server = "mqtt.espert.io";

unsigned long time_oled = millis();
unsigned long time_task = millis();
ESPert espert;

String outTopic = "ESPert/" + String(espert.info.getChipId()) + "/DHT";
void setup()
{
    espert.init();
    espert.dht.init(12,DHT11);
    espert.oled.init();
    delay(2000);
    espert.oled.clear();
    espert.oled.setTextSize(1);
    espert.oled.setTextColor(ESPERT_WHITE);
    espert.oled.setCursor(0, 0);
    espert.oled.println("    AppStack.CC");

    espert.oled.setCursor(0, 20);
    espert.oled.println("    DHT Firmware");
    espert.oled.update();

    Serial.begin(115200);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(ota_hostname);

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]()
    {
        espert.oled.clear();
        espert.oled.setCursor(0, 20);
        espert.oled.println("   Start Upload");
        Serial.println("Start");
    });

    ArduinoOTA.onEnd([]()
    {
        espert.oled.clear();
        espert.oled.setCursor(0, 20);
        espert.oled.println("   Upload Done");
        Serial.println("End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        if (millis() - time_oled > 1000)
        {
            time_oled = millis();
            espert.oled.clear();
            espert.oled.setCursor(10, 20);
            espert.oled.print("Uploading : ");
            espert.oled.print(progress / (total / 100));
            espert.oled.println("%");
        }
        Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECIEVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    espert.mqtt.init(mqtt_server, 1883);
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

}

void dht_task()
{
    if (espert.mqtt.connect())
    {
      espert.println("MQTT: Connected");
      espert.println("MQTT: Out Topic " + outTopic);
    }

    bool isFarenheit = false;
    float t = espert.dht.getTemperature(isFarenheit);
    float h = espert.dht.getHumidity();

    if (!isnan(t) && !isnan(h))
    {
        String outString  = "{\"temperature\":\"" + String(t) + "\", ";
        outString += "\"humidity\":\"" + String(h) + "\", ";
        outString += "\"name\":\"" + String(espert.info.getId()) + "\"}";
        espert.println(outString);
        espert.mqtt.publish(outTopic, outString);

        // update oled
        String dht = "Temperature: " + String(t) + (isFarenheit ?  " F" : " C") + "\r\n\r\n";
        dht += "Humidity...: " + String(h) + " %\n";

        espert.oled.clear();
        espert.oled.println("    AppStack.CC");
        espert.oled.setCursor(0, 15);   // Fix oled display
        espert.oled.println(dht);
        espert.println(dht);
    }
}
void loop()
{
    ArduinoOTA.handle();

    /* read sensor*/
    if (millis() - time_task > 5000)
    {
        time_task = millis();
        dht_task();
    }
}
