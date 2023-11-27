#include <ThingsBoard.h>
#include <SPI.h>
#include <LoRa.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#include "PageIndex.h" 
#define ss 5
#define rst 14
#define dio0 26
const char* ssid = "VQT1997";
const char* password = "06091997";
// Thông tin đăng nhập
const char* http_username = "1953020011";
const char* http_password = "vaa";
IPAddress local_ip(172,20,10,9);
IPAddress gateway(172,20,10,1);
IPAddress subnet(255,255,255,240);
#define TOKEN "onux82d3ptu4ihlqgtv2"
char thingsboardServer[] = "demo.thingsboard.io";
WiFiClient client1;
ThingsBoard device1(client1);
int status = WL_IDLE_STATUS;
unsigned long lastSend;
String Incoming = "";
String Message = ""; 
byte LocalAddress = 0x01;               //Địa chỉ Lora Gateway
byte Destination_ESP32_Slave_1 = 0x02;  //Địa chỉ Lora Slave 1
byte Destination_ESP32_Slave_2 = 0x03;  //Địa chỉ Lora Slave 2
byte Destination_ESP32_Slave_3 = 0x04;  //Địa chỉ Lora Slave 3
const byte get_Data_Mode = 1;           
const byte led_Control_Mode = 2;        
unsigned long previousMillis_SendMSG_to_GetData = 0;
const long interval_SendMSG_to_GetData = 1000;
unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;
int Humd[3];
float Temp[3];
float voltage;
float current;
float power;
float energy;
float frequency;
float pf;
String LED_1_State_str = "";
String LED_2_State_str = "";
String RAIN_State_str = "";
String receive_Status_Read_DHT11 = "";
String receive_Status_Read_RAIN = "";
String receive_Status_Read_PZEM = "";
bool LED_1_State_bool = false;
bool LED_2_State_bool = false;
bool RAIN_State_bool = false;

const char* PARAM_INPUT_1 = "Slave_Num";
const char* PARAM_INPUT_2 = "LED_Num";
const char* PARAM_INPUT_3 = "LED_Val";

String Slave_Number = "";
String LED_Number = "";
String LED_Value = "";
byte Slv = 0;
byte slave_Address;
byte count_to_Rst_LORA = 0;
bool finished_Receiving_Message = false;
bool finished_Sending_Message = false;
bool send_Control_LED = false;
JSONVar JSON_All_Data_Received;
AsyncWebServer server(80);
AsyncEventSource events("/events");
void manhinh()  // HIỂN THỊ MÀN HÌNH CHÍNH
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  DO AN TOT NGHIEP  ");
  lcd.setCursor(0,1);
  lcd.print("GVHD: TRAN QUOC KHAI");
  lcd.setCursor(0,2);
  lcd.print("SVHT: VU QUYET THANG");
  lcd.setCursor(0,3);
  lcd.print("MSSV: 1953020011    ");   
}
void tb()
{
  device1.sendTelemetryData("temperature1", Temp[0]);
  device1.sendTelemetryData("humidity1", Humd[0]);
  device1.sendTelemetryData("voltage",voltage);
  device1.sendTelemetryData("current",current);
  device1.sendTelemetryData("power",power);
  device1.sendTelemetryData("energy",energy);
  device1.sendTelemetryData("frequency",frequency);
  device1.sendTelemetryData("pf",pf);
  device1.sendTelemetryData("temperature2", Temp[1]);
  device1.sendTelemetryData("humidity2", Humd[1]);
  device1.sendTelemetryData("temperature3", Temp[2]);
  device1.sendTelemetryData("humidity3", Humd[2]);
}
void sendMessage(String Outgoing, byte Destination, byte SendMode) { 
  finished_Sending_Message = false;
  Serial.println();
  Serial.println("Tr to  : 0x" + String(Destination, HEX));
  Serial.print("Mode   : ");
  if (SendMode == 1) Serial.println("Getting Data");
  if (SendMode == 2) Serial.println("Controlling LED.");
  Serial.println("Message: " + Outgoing);
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.write(SendMode);           //--> 
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
  
  finished_Sending_Message = true;
}
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  finished_Receiving_Message = false;

  //---------------------------------------- read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length
  Incoming = "";
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }
  count_to_Rst_LORA = 0;
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("error: message length does not match length");
    finished_Receiving_Message = true;
    return;
  }
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    finished_Receiving_Message = true;
    return;
  }
  Serial.println();
  Serial.println("Rc from: 0x" + String(sender, HEX));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + Incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  slave_Address = sender;
  Processing_incoming_data();
  finished_Receiving_Message = true;
}
void Processing_incoming_data() {
  //////////Điều kiện data và mess của Phòng khí tượng gửi cho esp32////////////
  if (slave_Address == Destination_ESP32_Slave_1) {
    receive_Status_Read_DHT11 = GetValue(Incoming, ',', 0);
    if (receive_Status_Read_DHT11 == "f") receive_Status_Read_DHT11 = "FAILED";
    if (receive_Status_Read_DHT11 == "s") receive_Status_Read_DHT11 = "SUCCEED";
    Humd[0] = GetValue(Incoming, ',', 1).toInt();
    Temp[0] = GetValue(Incoming, ',', 2).toFloat();
    
    LED_1_State_str = GetValue(Incoming, ',', 3);
    LED_2_State_str = GetValue(Incoming, ',', 4);
    if (LED_1_State_str == "1" || LED_1_State_str == "0") {
      LED_1_State_bool = LED_1_State_str.toInt();
    }
    if (LED_2_State_str == "1" || LED_2_State_str == "0") {
      LED_2_State_bool = LED_2_State_str.toInt();
    }
    RAIN_State_str = GetValue(Incoming, ',', 5);
    if (receive_Status_Read_RAIN == "f") receive_Status_Read_RAIN = "FAILED";
    if (receive_Status_Read_RAIN == "s") receive_Status_Read_RAIN = "SUCCEED";
    if (RAIN_State_str == "1" || RAIN_State_str == "0") {
      RAIN_State_bool = RAIN_State_str.toInt();
    }
    Send_Data_to_WS("S1", 1);
  }
   //////////Điều kiện data và mess của Ngoài trời gửi cho esp32////////////
  if (slave_Address == Destination_ESP32_Slave_2) {
    receive_Status_Read_DHT11 = GetValue(Incoming, ',', 0);
    if (receive_Status_Read_DHT11 == "f") receive_Status_Read_DHT11 = "FAILED";
    if (receive_Status_Read_DHT11 == "s") receive_Status_Read_DHT11 = "SUCCEED";
    Humd[1] = GetValue(Incoming, ',', 1).toInt();
    Temp[1] = GetValue(Incoming, ',', 2).toFloat();
    LED_1_State_str = GetValue(Incoming, ',', 3);
    LED_2_State_str = GetValue(Incoming, ',', 4);
    if (LED_1_State_str == "1" || LED_1_State_str == "0") {
      LED_1_State_bool = LED_1_State_str.toInt();
    }
    if (LED_2_State_str == "1" || LED_2_State_str == "0") {
      LED_2_State_bool = LED_2_State_str.toInt();
    }
    Send_Data_to_WS("S2", 2);
  }
    //////////Điều kiện data và mess của đài dẫn đường gửi cho esp32////////////
  if (slave_Address == Destination_ESP32_Slave_3) {
    receive_Status_Read_DHT11 = GetValue(Incoming, ',', 0);
    if (receive_Status_Read_DHT11 == "f") receive_Status_Read_DHT11 = "FAILED";
    if (receive_Status_Read_DHT11 == "s") receive_Status_Read_DHT11 = "SUCCEED";
    Humd[2] = GetValue(Incoming, ',', 1).toInt();
    Temp[2] = GetValue(Incoming, ',', 2).toFloat();
    LED_1_State_str = GetValue(Incoming, ',', 3);
    LED_2_State_str = GetValue(Incoming, ',', 4);
    if (LED_1_State_str == "1" || LED_1_State_str == "0") {
      LED_1_State_bool = LED_1_State_str.toInt();
    }
    if (LED_2_State_str == "1" || LED_2_State_str == "0") {
      LED_2_State_bool = LED_2_State_str.toInt();
    }
    receive_Status_Read_PZEM = GetValue(Incoming, ',', 5);
    if (receive_Status_Read_PZEM == "f") receive_Status_Read_PZEM = "FAILED";
    if (receive_Status_Read_PZEM == "s") receive_Status_Read_PZEM = "SUCCEED";
    voltage = GetValue(Incoming, ',', 6).toFloat();
    current = GetValue(Incoming, ',', 7).toFloat();
    power = GetValue(Incoming, ',', 8).toFloat();
    energy = GetValue(Incoming, ',', 9).toFloat();
    frequency = GetValue(Incoming, ',', 10).toFloat();
    pf = GetValue(Incoming, ',', 11).toFloat();
    Send_Data_to_WS("S3", 3);
  }
}
void Send_Data_to_WS(char ID_Slave[5], byte Slave) {
  JSON_All_Data_Received["ID_Slave"] = ID_Slave;
  JSON_All_Data_Received["StatusReadDHT11"] = receive_Status_Read_DHT11;
  JSON_All_Data_Received["StatusReadRAIN"] = receive_Status_Read_RAIN;
  JSON_All_Data_Received["Humd"] = Humd[Slave-1];
  JSON_All_Data_Received["Temp"] = Temp[Slave-1];
  JSON_All_Data_Received["LED1"] = LED_1_State_bool;
  JSON_All_Data_Received["LED2"] = LED_2_State_bool; 
  JSON_All_Data_Received["RAIN"] = RAIN_State_bool; 
  JSON_All_Data_Received["StatusReadPZEM"] = receive_Status_Read_PZEM;
  JSON_All_Data_Received["voltage"] = voltage;
  JSON_All_Data_Received["current"] = current;
  JSON_All_Data_Received["power"] = power;
  JSON_All_Data_Received["energy"] = energy;
  JSON_All_Data_Received["frequency"] = frequency;
  JSON_All_Data_Received["pf"] = pf;
  String jsonString_Send_All_Data_received = JSON.stringify(JSON_All_Data_Received);
  events.send(jsonString_Send_All_Data_received.c_str(), "allDataJSON", millis());
}
String GetValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1; 
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void Rst_LORA() {
  LoRa.setPins(ss, rst, dio0);
  Serial.println();
  Serial.println("Restart LoRa...");
  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {           
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       
  }
  Serial.println("LoRa init succeeded.");
  count_to_Rst_LORA = 0; // reset giá trị đếm của lora
}
void setup() {
  Serial.begin(115200);
  Wire.begin(22,21); //SDA,SCL 
  lcd.init();
  lcd.backlight(); 
  manhinh();
  for (byte i = 0; i < 3; i++) {
    Humd[i] = 0;
    Temp[i] = 0.00;
  }
  voltage = 0.00;
  current = 0.00;
  power = 0.00;
  energy = 0.00;
  frequency = 0.00;
  pf = 0.00;
  delay(10);
  InitWiFi();
  lastSend = 0;
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : AP");
  WiFi.mode(WIFI_AP);
  Serial.println("-------------");
  delay(100);
  Serial.println();
  Serial.println("-------------");
  Serial.println("Setting up ESP32 to be an Access Point.");
  WiFi.softAP(ssid, password);
  delay(1000);
  Serial.println("Setting up ESP32 softAPConfig.");
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("-------------");
  delay(500);
  Serial.println();
  Serial.println("Setting Up the Main Page on the Server.");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MAIN_page);
  });
//  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//  if (!request->authenticate(http_username, http_password)) 
//  {
//    return request->requestAuthentication();
//  }
//  request->send_P(200, "text/html", MAIN_page);
//  });
  Serial.println();
  Serial.println("Setting up event sources on the Server.");
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.on("/set_LED", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam(PARAM_INPUT_3)) {
      Slave_Number = request->getParam(PARAM_INPUT_1)->value();
      LED_Number = request->getParam(PARAM_INPUT_2)->value();
      LED_Value = request->getParam(PARAM_INPUT_3)->value();

      String Rslt = "Slave : " + Slave_Number + " || LED : " + LED_Number + " || Set to : " + LED_Value;
      Serial.println();
      Serial.println(Rslt);
      send_Control_LED = true;
    }
    else {
      send_Control_LED = false;
      Slave_Number = "No message sent";
      LED_Number = "No message sent";
      LED_Value = "No message sent";
      String Rslt = "Slave : " + Slave_Number + " || LED : " + LED_Number + " || Set to : " + LED_Value;
      Serial.println();
      Serial.println(Rslt);
    }
    request->send(200, "text/plain", "OK");
  }
  );
  Serial.println();
  Serial.println("Adding event sources on the Server.");
  server.addHandler(&events);
  Serial.println();
  Serial.println("Starting the Server.");
  server.begin();
  Rst_LORA();
  Serial.println();
  Serial.print("SSID name : ");
  Serial.println(ssid);
  Serial.print("IP address : ");
  Serial.println(WiFi.softAPIP());
  Serial.println();
  Serial.println("Kết nối máy tính.");
  Serial.println("Hãy truy cập vào địa chỉ IP.");
  Serial.println();
}
void loop() {
  if (!device1.connected()) 
  { 
    reconnect();
  }

  if (millis() - lastSend > 1000) 
  {
    tb();
    lastSend = millis();
  }
  unsigned long currentMillis_SendMSG_to_GetData = millis();
  if (currentMillis_SendMSG_to_GetData - previousMillis_SendMSG_to_GetData >= interval_SendMSG_to_GetData) {
    previousMillis_SendMSG_to_GetData = currentMillis_SendMSG_to_GetData;

    Slv++;
    // Nếu thời gian gửi ở giây thứ 4 trở lên, quay về slave 1 
    if (Slv > 3) Slv = 1;
    //Điều kiện gửi cho Slave 1: giây 1
    if (Slv == 1) {
      Humd[0] = 0;
      Temp[0] = 0.00;
      sendMessage("", Destination_ESP32_Slave_1, get_Data_Mode);
      
    }
    //Điều kiện gửi cho Slave 2: giây 2
    if (Slv == 2) {
      Humd[1] = 0;
      Temp[1] = 0.00;
      sendMessage("", Destination_ESP32_Slave_2, get_Data_Mode);
    }
    //Điều kiện gửi cho Slave 3: giây 3
    if (Slv == 3) {
      Humd[2] = 0;
      Temp[2] = 0.00;
      voltage = 0.00;
      current = 0.00;
      power = 0.00;
      energy = 0.00;
      frequency = 0.00;
      pf = 0.00;
      sendMessage("", Destination_ESP32_Slave_3, get_Data_Mode);
    }
  }
  if (finished_Sending_Message == true && finished_Receiving_Message == true) {
    if (send_Control_LED == true) {
      delay(250);
      send_Control_LED = false;
      if (Slave_Number == "S1") {
        Message = "";
        Message = LED_Number + "," + LED_Value;
        sendMessage(Message, Destination_ESP32_Slave_1, led_Control_Mode);
      }
      if (Slave_Number == "S2") {
        Message = "";
        Message = LED_Number + "," + LED_Value;
        sendMessage(Message, Destination_ESP32_Slave_2, led_Control_Mode);
      }
      if (Slave_Number == "S3") {
        Message = "";
        Message = LED_Number + "," + LED_Value;
        sendMessage(Message, Destination_ESP32_Slave_3, led_Control_Mode);
      }
      delay(250);
    }
  }
  unsigned long currentMillis_RestartLORA = millis(); 
  if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA) {
    previousMillis_RestartLORA = currentMillis_RestartLORA;
    count_to_Rst_LORA++;
    if (count_to_Rst_LORA > 30) {
      LoRa.end();
      Rst_LORA();
    }
  }
  onReceive(LoRa.parsePacket());
}
void InitWiFi()
{
  Serial.println("Waiting......");
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}
void reconnect() {
  while (!device1.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(ssid,password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected!");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if ( device1.connect(thingsboardServer, TOKEN) ) 
    {
      Serial.println( "[DONE]" );
    } 
    else 
    {
      Serial.print( "[FAILED]" );
      Serial.println( " [ retrying in 5 seconds]" );
      // Wait 5 detik
      delay( 5000 );
    }
  }
}
