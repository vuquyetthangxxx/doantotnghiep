#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0X27,20,4); //SCL A5 SDA A4
PZEM004Tv30 pzem(4,3); 
byte customChar[] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000};
#define DHTPIN      5
#define DHTTYPE     DHT11
#define LED_1_Pin   6
#define LED_2_Pin   7
#define ss 10
#define rst 9
#define dio0 2

DHT dht11(DHTPIN, DHTTYPE);

String Incoming = "";
String Message = "";
String LED_Num = "";
String LED_State = "";

byte LocalAddress = 0x04; 
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).

int Humd = 0;
float Temp = 0.00;
float voltage;
float current;
float power;
float energy;
float frequency;
float pf;
String send_Status_Read_DHT11 = "";
String send_Status_Read_PZEM = "";
unsigned long previousMillis_UpdateDHT11 = 0;
const long interval_UpdateDHT11 = 2000;
unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;
byte Count_to_Rst_LORA = 0;
void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;         
  int recipient = LoRa.read();       
  byte sender = LoRa.read();         
  byte incomingLength = LoRa.read();  
  byte master_Send_Mode = LoRa.read();
  if (sender != Destination_Master) {
    Serial.println();
    Serial.println("Not from Master, Ignore");
    Count_to_Rst_LORA = 0;
    return; 
  }
  Incoming = "";
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }
  Count_to_Rst_LORA = 0;
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("error: message length does not match length");
    return;
  }
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    return;
  } else {
    Serial.println();
    Serial.println("Rc from: 0x" + String(sender, HEX));
    Serial.println("Message: " + Incoming);
    if (master_Send_Mode == 1) Processing_incoming_data();
    if (master_Send_Mode == 2) Processing_incoming_data_for_Ctrl_LEDs();
  }
}

void Processing_incoming_data() {
  // Get the last state of the LEDs.
  byte LED_1_State = digitalRead(LED_1_Pin);
  byte LED_2_State = digitalRead(LED_2_Pin);
  Message = "";
  Message = send_Status_Read_DHT11 + "," +  String(Humd) + "," + String(Temp) + "," + String(LED_1_State) + "," + String(LED_2_State) + "," + send_Status_Read_PZEM + "," + String(voltage)+ "," + String(current) + "," + String(power)+ "," + String(energy)+ "," + String(frequency)+ "," + String(pf); 
  Serial.println();
  Serial.println("Tr to  : 0x" + String(Destination_Master, HEX));
  Serial.println("Message: " + Message);
  sendMessage(Message, Destination_Master);
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.setCursor(2,0);
    lcd.print(Temp);
    lcd.setCursor(4,0);
    lcd.write(byte(0)); 
    lcd.setCursor(5,0);
    lcd.print("C");
    lcd.setCursor(6,0);
    lcd.print("H:");
    lcd.setCursor(8,0);
    lcd.print(Humd);
    lcd.setCursor(10,0);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("U:");
    lcd.print(voltage,1);
    lcd.print("V");
    
    lcd.setCursor(0,2);
    lcd.print("I:");
    lcd.print(current,2);
    lcd.print("A");
    
    lcd.setCursor(0,3);
    lcd.print("P:");
    lcd.print(power,2);
    lcd.print("W");

    lcd.setCursor(9,1);
    lcd.print("E:");
    lcd.print(energy,2);
    lcd.print("kwh");

    lcd.setCursor(9,2);
    lcd.print("F:");
    lcd.print(frequency,1);
    lcd.print("Hz");

    lcd.setCursor(9,3);
    lcd.print("pF:");
    lcd.print(pf,2);

    lcd.setCursor(11,0);
    lcd.print("L1:");
    lcd.setCursor(16,0);
    lcd.print("L2:");
}
void Processing_incoming_data_for_Ctrl_LEDs() {
  LED_Num = GetValue(Incoming, ',', 0);
  LED_State = GetValue(Incoming, ',', 1);
  if (LED_Num == "1") 
  {
    if (LED_State == "t")
    {
      lcd.setCursor(14,0);
      lcd.print("O");
      digitalWrite(LED_1_Pin, HIGH);
    }
    if (LED_State == "f")
    {
      lcd.setCursor(14,0);
      lcd.print("F");
      digitalWrite(LED_1_Pin, LOW);
    }
  }
  if (LED_Num == "2") 
  {
    if (LED_State == "t")
    {
      lcd.setCursor(19,0);
      lcd.print("O");
      digitalWrite(LED_2_Pin, HIGH);
    }
    if (LED_State == "f")
    {
      lcd.setCursor(19,0);
      lcd.print("F");
      digitalWrite(LED_2_Pin, LOW);
    }
  }
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
  Serial.println(F("Restart LoRa..."));
  Serial.println(F("Start LoRa init..."));
  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 or 433 MHz
    Serial.println(F("LoRa init failed. Check your connections."));
    while (true);                       // if failed, do nothing
  }
  Serial.println(F("LoRa init succeeded."));

  Count_to_Rst_LORA = 0;
}

void setup() {

  Serial.begin(115200);
  lcd.init();       
  lcd.backlight();   
  lcd.createChar(0,customChar);
  lcd.setCursor(0,0);
  pinMode(LED_1_Pin, OUTPUT);
  pinMode(LED_2_Pin, OUTPUT);
  digitalWrite(LED_1_Pin, LOW);
  digitalWrite(LED_2_Pin, LOW);
  // Calls the Rst_LORA() subroutine.
  Rst_LORA();
  dht11.begin();
}
void loop() {
  unsigned long currentMillis_UpdateDHT11 = millis();
  if (currentMillis_UpdateDHT11 - previousMillis_UpdateDHT11 >= interval_UpdateDHT11) {
    previousMillis_UpdateDHT11 = currentMillis_UpdateDHT11;
    Humd = dht11.readHumidity();
    Temp = dht11.readTemperature();
    voltage = pzem.voltage();
    current = pzem.current();
    power = pzem.power();
    energy = pzem.energy();
    frequency = pzem.frequency();
    pf = pzem.pf();
    if (isnan(Humd) || isnan(Temp)) {
      Humd = 0;
      Temp = 0.00;
      
      send_Status_Read_DHT11 = "f";
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    } else {
      send_Status_Read_DHT11 = "s";
    }
    
    if( isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf)){
      voltage = 0.00;
      current = 0.00;
      power = 0.00;
      energy = 0.00;
      frequency = 0.00;
      pf = 0.00;
      send_Status_Read_PZEM = "f";
      Serial.println(F("Failed to read from PZEM!"));
      return;
    }
    else 
    {
      send_Status_Read_PZEM = "s";
    }
    
  }
  unsigned long currentMillis_RestartLORA = millis();
  if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA) {
    previousMillis_RestartLORA = currentMillis_RestartLORA;
    Count_to_Rst_LORA++;
    if (Count_to_Rst_LORA > 30) {
      LoRa.end();
      Rst_LORA();
    }
  }
  onReceive(LoRa.parsePacket());
}
