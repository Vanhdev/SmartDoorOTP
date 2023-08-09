#include <HardwareSerial.h>
#include <Arduino.h>
#include <string.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Define the serial communication pins for the SIM module
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883
#define LOCK 15


#define SEND_OTP_TOPIC "ESP32/SEND_OTP"

const char* ssid = "V13t4nhtr4n";
const char* password = "v13t4nhtr4n";

// Initialize the HardwareSerial library
HardwareSerial simSerial(1);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void initSim800() {
  simSerial.println("AT");
  delay(1000);

  // Set the functionality level to full functionality
  simSerial.println("AT+CFUN=1,1");
  delay(6000);
  
  simSerial.println("AT+CSQ");
  delay(3000);

  simSerial.println("AT+CPIN?");
  delay(1000);

  simSerial.println("AT+CCID");
  delay(1000);

  simSerial.println("AT+CREG?");
  delay(1000);

  // Send AT command to check if the SIM800L module is responding
  String response = "";
  while (simSerial.available()) {
    response += simSerial.readString();
  }

  Serial.println(response);
  if (response.indexOf("READY") != -1) {
    Serial.println("SIM card unlocked successfully");
  } else {
    Serial.println("Failed to unlock SIM card");
  }
  // Set the SMS message format to text mode
  simSerial.println("AT+CMGF=1");
  delay(1000);
  simSerial.println("AT+CMGL=\"ALL\"");
  delay(6000);
  simSerial.println("AT+cmgd=?");
  delay(2000);

  simSerial.println("AT+cmgd=,4");
  delay(5000);
  
}

void connect_to_broker() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(SEND_OTP_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void send_sms(String phone, String otp) {
  String message = "Ma OTP cua ban la ";
  simSerial.print("AT+CMGS=\"+84");
  simSerial.print(phone);
  simSerial.println("\"");
  delay(1000);
  simSerial.print(message);
  simSerial.print(otp);
  simSerial.write((byte)0x1A);
  delay(2000);
  String response = "";
  while (simSerial.available()) {
    response += simSerial.readString();
  }
  if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
    Serial.println("SMS message sent successfully");
  } else {
    Serial.println("Failed to send SMS message");
  }
  delay(1000);
}

void callback(char* topic, byte *payload, unsigned int length) {
  char otp[5] = "\0";
  char phone[10] = "\0";
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.write(payload, length);
  Serial.println();
  Serial.println(length);
  for(int i = 0; i<length; i++)
  {
    if(i < 4)
    {
      otp[i] = payload[i];
    }
    else
    {
      phone[i - 4] = payload[i];
    }
  }
  Serial.println(otp);
  Serial.println(phone);
  if(String(topic) == SEND_OTP_TOPIC)
  {
    send_sms(String(phone), String(otp));
  }
   
}

void setup() {
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Initialize serial communication with the SIM module
  simSerial.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  Serial.begin(115200);

  initWiFi();

  delay(1000);

  initSim800();

  client.setServer(MQTT_SERVER, MQTT_PORT );
  client.setCallback(callback);
  connect_to_broker();
  Serial.println("Start transfer");

}

String buff;
void loop() {
  // Send an SMS message
  client.loop();
  if (!client.connected()) {
    connect_to_broker();
  }
  // delay(15000);
  // while (simSerial.available()) {
  //   int v = simSerial.read();
  //   buff += (char)v;

  //   if (v == '\r')  {
  //     Serial.print(buff);
  //     if (buff.indexOf("SMS Ready") >= 0) {
  //       Serial.print("Co the gui tin nhan\n");
  //     }
  //     buff = "";
  //   }
  // }
//  
}