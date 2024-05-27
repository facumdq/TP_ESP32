#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

Servo servo;
#define pin_servo 4
#define botonAceptado 32
#define botonDenegado 33
#define buzzerPasivo 15
#define ledRojo 17
#define ledVerde 16

#define echoPin 12
#define trigPin 13

int distancia;

// WiFi
const char *ssid = "****";
const char *password = "****";

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "cerradura/mdp/esp32";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(115200);
    
    pinMode(botonAceptado,INPUT_PULLUP);
    pinMode(botonDenegado,INPUT_PULLUP);
    pinMode(buzzerPasivo,OUTPUT);
    pinMode(ledRojo,OUTPUT);
    pinMode(ledVerde,OUTPUT);
    pinMode(trigPin,OUTPUT);
    pinMode(echoPin,INPUT);

    servo.attach(pin_servo, 500, 2500);
    
    Serial.println("INICIO");
    servo.write(180);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the Wi-Fi network");

    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str())) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    client.subscribe(topic);
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
    if ((char) payload[0] == '1') grant();
}

void grant() {
  client.publish(topic, "ACCESO PERMITIDO");
  client.publish(topic, "PUERTA ABIERTA");
  digitalWrite(ledVerde,1);
  digitalWrite(buzzerPasivo,1);
  servo.write(0);
  Serial.println("BIENVENIDO");
  delay(500);
  digitalWrite(ledVerde,0);
  digitalWrite(buzzerPasivo,0);
  client.publish(topic, "PUERTA CERRADA");
  servo.write(180);
}

void deny() {
  client.publish(topic, "ACCESO DENEGADO");
  digitalWrite(ledRojo,1);

  digitalWrite(buzzerPasivo,1);
  delay(100);
  digitalWrite(buzzerPasivo,0);
  delay(100);
  digitalWrite(buzzerPasivo,1);
  delay(100);
  digitalWrite(buzzerPasivo,0);
  delay(100);

  Serial.println("ACCESO DENEGADO");
  delay(250);
  digitalWrite(ledRojo,0);
}

void loop() {
  client.loop();

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  distancia = pulseIn(echoPin, HIGH);
  distancia = (distancia/2) / 29;
  delay(20);
  digitalWrite(trigPin, LOW);

  if(distancia < 15)
    if(digitalRead(botonAceptado) == 0) grant();
    else if(digitalRead(botonDenegado) == 0) deny();
}