  // отправляем запрос через SoftwareSerial
// ожидаем ответа
// связь двухсторонняя!!!! Это скетч ПЕРЕДАТЧИКА
// TX передатчика подключен к RX приёмника
// RX передатчика подключен к TX приёмника
// подключение библиотек
//#include "GyverWDT.h"
#include <avr/eeprom.h>
#include <EEPROM.h>
#define BTN_PIN A0        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
#include "GyverButton.h"
GButton butt1(BTN_PIN);
#define status220v 5  //220V status  
#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[6]; // = {  0xD4, 0x81, 0xD7, 0x98, 0x6D, 0x9A };

EthernetServer server(80);

#include <Wire.h> 
#include <LiquidCrystal.h>
LiquidCrystal lcd(6, 7, A2, A3, A4, A5);  // инициализация дисплея

#include "DHT.h"
// назначение PIN и выбор типа датчика DHT
#define DHTPIN 2 // 
#define DHTTYPE DHT22 // DHT 22 (AM2302)
byte devises = eeprom_read_byte(20);     //количество девайсов

//byte i_max = devises;
float volts[24];
//float volt_max = 0;
byte devises_new = 1;     //кол-во новых устройст
byte i = 0;

//bool ledreset = LOW;
//float volt_max_last;
//const byte led = 4;
//const byte button = 5;
//int buttonState = 0;
byte posit;               //для перемещения по меню

byte value;               // переменная для вывода для блинка

static uint32_t tmr_prog;
struct MyFlags {
//  bool all: 1;
  bool flagProg: 1;// = LOW;
  bool ProgBlinck: 1;// = 1;      //для блинка при программировании
  bool prog_dev_flag: 1;// = LOW; //флаг для программирования кол-ва устройст (для подсчёта)

};
MyFlags flags;


//float val_sred;
DHT dht(DHTPIN, DHTTYPE);   // инициализация сенсора DHT

#include <SoftwareSerial.h>
SoftwareSerial mySerial(21, 22); // RX, TX
// сначала объявляем обработчик
// это может почти быть любая интерфейсная либа,
// например софтСериал на любой другой платформе

// указываем "обработчик" интерфейса перед подключением библиотеки
#include "GBUS.h"

// адрес 200, буфер 5 байт, скорость 9600 бод
GBUS bus(&mySerial, 200, 5);

/*struct Str {
  bool temp;
  byte ledRed_state;
  byte ledElov_state;
  byte ledGren_state;
}; */
void(* resetFunc) (void) = 0;//объявляем функцию reset с адресом 0
void setup() {
  if (devises > 32) {
    eeprom_write_byte(20, 1);
    devises = eeprom_read_byte(20);     //перечитываем из ЕЕПРОМ количество девайсов
  }

//  Watchdog.enable(RESET_MODE, WDT_PRESCALER_1024); // Режим сторжевого сброса , таймаут ~8с
   flags.flagProg = LOW;
//   flags.all = LOW;
   flags.ProgBlinck = 1;    //для блинка при программировании
   flags.prog_dev_flag = LOW; //флаг для программирования кол-ва устройст (для подсчёта)
  pinMode(status220v, INPUT);
  randomSeed(analogRead(1));

  //float volts[i_max];
  // родной сериал открываю для наблюдения за процессом
  Serial.begin(9600);
  lcd.begin(16, 2); // инициализация lcd
  //lcd.backlight(); // включение подсветки дисплея
  //lcd.createChar(1, symb_grad);  // регистрируем собственный символ с кодом 1
  dht.begin();  //  запуск датчика DHT
  // этот сериал будет принимать и отправлять данные
  mySerial.begin(9600);
//  i = 1;
  //  i_max = 32;
  ////lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
  //lcd.print("H:     % T:      C");  // вывод текста
  //val_sred = 0;
   lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
   lcd.print("starting ethernet");  // очищаем
    ethernet();  //стартуем сеть


   for (byte q = 0; q < devises; q++) {
      volts[q]=0;
   }
//   pinMode(led, OUTPUT);
//   digitalWrite(led, LOW);
//   pinMode(button, INPUT);
     butt1.setDebounce(100);      // настройка антидребезга (по умолчанию 80 мс)
  butt1.setTimeout(500);      // настройка таймаута на удержание (по умолчанию 500 мс)
}



void isr() {
  butt1.tick();  // опрашиваем в прерывании, чтобы поймать нажатие в любом случае
}
void loop() {

  butt1.tick();
  if (butt1.isClick()) {
//   Serial.println("Click once");         // проверка на один клик
    
  }
  if (butt1.isRelease()) {                              // отпускание кнопки (+ дебаунс) сбрасываем счётчик таймера программирования
//    Serial.println("Push off");     
    if (flags.flagProg) {
      tmr_prog = millis();
    }
  }
  if (butt1.isSingle()) {
//    Serial.println("Single");       // проверка на один клик
    
      if (!flags.flagProg) posit ++;      // пеперходим в следю меню если не программируем
      if (posit > 5) posit=1;
      if (posit == 1) DisplayMac(); //дисплей МАКа
      if (posit == 2) {             // выводим IP адрес
        lcd.clear();
//        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
        if (!flags.flagProg) {
          lcd.print("IP adres:");
//          lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
//          lcd.print("                ");  // очищаем
          lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
          lcd.print(Ethernet.localIP());  // очищаем          
        }
      }
      if (posit == 3) {             // выводим количество опрашиваемых устройств
        lcd.clear();
//        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
        lcd.print("Perif devises");
//        lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
        if (!flags.flagProg) {
          lcd.print(devises);
        } else {

          if (!flags.prog_dev_flag) {
            devises_new = 1;
          } else {
             devises_new++;
          }
          flags.prog_dev_flag = 1;
          //devises_new++;
          if (devises_new > 32) devises_new = 1;
          //eeprom_write_byte(10, devises_new);
          //devises = devises_new;
          value = devises_new;
          
        }
      }
      if (posit == 4) {             // выводим состояние 220В
        lcd.clear();        
//        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
        lcd.print("Status 220V");
//        lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
        if (digitalRead(status220v)) {
          lcd.print("220V is OK");
        } else {
          lcd.print("No 220V");
        } 
        
      }

      if (posit == 5) {
        lcd.clear();        
//        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
//        lcd.print("                ");  // очищаем
        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
        lcd.print("ClimatControl");
        lcd.setCursor(0, 1);  //  установка курсора в начало 2 строки
        lcd.print("H:     % T:     C");  // вывод текста
        lcd.setCursor(2, 1);  //  установка курсора 
        lcd.print(dht.readHumidity());
        lcd.setCursor(11, 1);  //  установка курсора 
        lcd.print(dht.readTemperature());
      }
        //    }
  }
  if (butt1.isHolded()) {                               // проверка на удержание
    tmr_prog = millis();
    if (posit == 1 || posit == 3) {
    
        if (!flags.flagProg) {                                     //Входим в режим программирования
  //        Serial.println("Holded! Programming");
          lcd.setCursor(15, 0);                              // установка курсора на начало первой строки
          lcd.print("*");  // 
          flags.flagProg = 1;
        } else {                //выходим из режима программирования с сохранением
          flags.prog_dev_flag = LOW;
          flags.flagProg = LOW;
//        eeprom_write_byte(1, value);
          if (posit == 3){
            eeprom_write_byte(20, devises_new);
            devises = devises_new;
            lcd.setCursor(0, 1);
            lcd.print(devises);
            resetFunc(); //вызываем reset
          } 
          if (posit == 1) {
            lcd.setCursor(0, 1);  //  установка курсора в начало 2 строки
            for (byte octet = 0; octet < 6; octet++) { lcd.print(mac[octet], HEX); EEPROM.update(octet,mac[octet]);}
            ethernet();
         // devises = devises_new;
         //   lcd.setCursor(0, 1);
            //lcd.print(devises);
         //   resetFunc(); //вызываем reset
          }
    //      lcd.setCursor(15, 0);  // установка курсора на начало первой строки
    //      lcd.print(" ");  // выходим из режима программирования
        }
    
    } 
  }  
  if (butt1.isTriple()) {                               // проверка на тройной клик
//    Serial.println("Triple");       
    if (flags.flagProg) {
      exitProgNoSave();
      flags.prog_dev_flag = LOW;
//      Serial.println("Cansel programming");
       //value = 1;
      if (posit = 1) DisplayMac();
    }
  }
  // здесь принимаются данные
  // если это аппаратный сериал - слишком часто его опрашивать даже не нужно
  if (millis() - tmr_prog > 120000 && flags.flagProg) { //2 минуты и выходим из программирования без сохранения
      exitProgNoSave ();
    }
  bus.tick();
  static uint32_t tmr_send;
  static uint32_t tmr;
    if (millis() - tmr > 500) {
        //Serial.print("tmr = ");Serial.print(tmr);Serial.print(" dht.readTemperature() is "); if (isnan(dht.readTemperature())) {Serial.println("Error reading from DHT");} else {Serial.println(dht.readTemperature());}; //Serial.print(" flags.flagProg ");Serial.println(flags.flagProg);
      tmr = millis();
      if (flags.flagProg) {
        flags.ProgBlinck = !flags.ProgBlinck;
        if (flags.ProgBlinck ) {
          lcd.setCursor(15, 0);                              // установка курсора на начало первой строки
          lcd.print(" ");  //
          lcd.setCursor(0, 1);  //  установка курсора в начало 2 строки
          if(posit == 1) {
            for (byte octet = 0; octet < 6; octet++) lcd.print(mac[octet], HEX);
          } 
          if (posit == 3) {
            lcd.print(value);  // выводим редактируемое значение
          }
        } else {
          lcd.setCursor(15, 0);                              // установка курсора на последнюю позицию первой строки
          lcd.print("*");  //
          lcd.setCursor(0, 1);  //  установка курсора в начало 2 строки
          lcd.print("                ");  // очищаем
        }
      }
    }
  
  if (millis() - tmr_send > 2000) {
   tmr_send = millis();
   if (i > (devises-1)) {
    i = 1;
//    Serial.print ("Volt max = "); Serial.println (volt_max);
//    flags.all = 1;
//    volt_max_last = volt_max;
//    volt_max = volts[1];
    } else {
      i++;
    }

 // буфер на отправку
//  Str buf;
 // Serial.print ("All = "); Serial.println (flags.all);
/*  if (flags.all = 1) {
//    buf.temp = 0;
//    buf.ledElov_state = 0;
//    buf.ledGren_state = 0;
//    buf.ledRed_state = 0;
  /*  if (volt_max_last - volts[i] > 1) {
      buf.ledRed_state = 1;
      Serial.print ("RED Volt modul "); Serial.println (i);
      } else {
        if (volt_max_last - volts[i] > 0.5) {
          buf.ledElov_state = 1;
          Serial.print ("ELOV Volt modul "); Serial.println (i); 
        } else {
         // if (volt_max_last - volts[i] < 0.05) {
            buf.ledGren_state = 1;
            Serial.print ("GREN Volt modul "); Serial.println (i);
   //       }
        }
      }
      bus.sendData(i, buf);
  }*/
//  Serial.print ("ledRed: "); Serial.print (buf.ledRed_state);Serial.print (" ledGren: "); Serial.print (buf.ledGren_state);Serial.print (" ledElov: "); Serial.println (buf.ledElov_state);

    
    delay (200);
/*    Serial.print("Send to ");
    Serial.print(i);
    Serial.print(" Hum: ");
    Serial.print(buf.Hum);
    Serial.print(" Temp: ");
    Serial.println(buf.Tem);  */
//    bus.sendRequest(3);
    Serial.print("Send request to "); Serial.println(i);
    bus.sendRequest(i);
    }
    
/*  buttonState = digitalRead(button);
  if (buttonState == HIGH) {
    if (ledreset == LOW) {
      ledreset = HIGH;
      // turn LED on:
      digitalWrite(led, HIGH);
           Str buf1;
    buf1.temp = 0;
    buf1.ledElov_state = 0;
    buf1.ledGren_state = 0;
    buf1.ledRed_state = 0;
    
      
      bus.sendData(GBUS_BROADCAST, buf1);
      bus.tick();
 //     bus.sendRequest(GBUS_BROADCAST);
       bus.tick();

    }
  } else {
    // turn LED off:
    digitalWrite(led, LOW);
    ledreset = LOW;
  } */

  // ждём ответа от 3
  // пытаемся достучаться через таймаут 500мс 3 раза
   byte state = bus.waitAck(i, 3, 500);
  //lcd.setCursor(0, 1);  // установка курсора на 10 позицию
  //lcd.print("stat:");  // вывод значения температуры
  //lcd.setCursor(6, 1);  // установка курсора на 10 позицию
  //lcd.print(state,1);  // вывод значения температуры 
  switch (state) {
        case ACK_IDLE: //Serial.println("idle");
      break;
    case ACK_WAIT: //Serial.println("wait");
      break;
    case ACK_ERROR: Serial.print("ack error to "); Serial.println(i);  volts[i]=0;
      break;
    case ACK_ONLY:// Serial.println("got ack");
      break;
    case ACK_DATA: //Serial.print("got data: ");
      // читаем и выводим
      
      bus.readData(volts[bus.getTXaddress()]);
//      volts[i]=val_sred;
//      if (volt_max < volts[i]) {
//       volt_max = volts[i];
 //     }
      Serial.print("Resiv from "); Serial.print(bus.getTXaddress());Serial.print(" volts ");
      Serial.println(volts[bus.getTXaddress()]);
      //lcd.setCursor(8, 1);  // установка курсора на 10 позицию
  //lcd.print("Res:");  // вывод значения температуры
  //lcd.setCursor(13, 1);  // установка курсора на 10 позицию
  //lcd.print(val_sred,1);  // вывод значения температуры 
      break;
    }
 /////////////Ethernet
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
//    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<head> ");
          client.println("<meta http-equiv='Content-Type' content='text/html; charset=utf-8' /> ");
          client.println("<title> :: Упр.Arduino:: V0.2</title>");
          client.println("</head> ");
          
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int n = 1; n < (devises+1); n++) {
            //int sensorReading = analogRead(analogChannel);
            client.print("Modul ");
            client.print(n);
            client.print(" is ");
            client.print(volts[n]);
            client.println("<br />");
          } 
     //     dhtdata();
          //client.print("volt_max "); client.print(volt_max); client.print(" volt_max_last "); client.print(volt_max_last); client.println("<br />");
            client.print("T = ");    //Температура с DHT 22
            client.print(dht.readTemperature());
            client.print(" C");
              client.println("<br>"); //перенос на след. строчку
            client.print("H = ");    //Влажность с DHT 22
            client.print(dht.readHumidity());
            client.println(" %<br>");
//              client.println(""); //перенос на след. строчку
/*
          client.print("Voltag input ");
            client.print(i);
            client.print(" is ");
            client.print(val_sred);
            client.println("<br />");*/
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
//    Serial.println("client disconnected");
  }
  
//Watchdog.reset(); // Переодический сброс watchdog, означающий, что устройство не зависло
 }

/*
 void dhtdata() {
     float h = dht.readHumidity();   // считывание влажности
    float t = dht.readTemperature();   // считывание температуры

 } */

void ethernet() {
  // start the Ethernet connection and the server:
  //  Ethernet.begin(mac, ip);
  Serial.println();
  for(byte i=0;i<6;i++) {
//    EEPROM.update(i,mac1[i]);
    mac[i]=EEPROM[i];
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print('-');
  }
//  
//  byte mac[6] = {  0x00, 0x48, 0x5A, 0x0B, 0x04, 0x2E };

 // byte macBuffer[6];  // create a buffer to hold the MAC address
 // Ethernet.MACAddress(macBuffer); // fill the buffer
//  Serial.print("The MAC address is: ");
/*  for (byte octet = 0; octet < 6; octet++) {
    Serial.print(macBuffer[octet], HEX);
    if (octet < 5) {
      Serial.print('-');
    }
  } */
  Serial.println();
  // Check for Ethernet hardware present
    // start the Ethernet connection:
//  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
////      lcd.setCursor(0, 1);                              // установка курсора на начало первой строки
////      lcd.print("Failed Ethernet");  //Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
//      lcd.setCursor(0, 1);                              // установка курсора на начало первой строки
//      lcd.print("No hardware Ethernet");  //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
////      lcd.setCursor(0, 1);                              // установка курсора на начало первой строки
////      lcd.print("No cable");  //Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  } 
  // print your local IP address:
//  Serial.print("My IP address: ");
//  Serial.println(Ethernet.localIP());

/*  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
//    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
//      Serial.println("RESET MAC");
//      GenNewMAC();
      
//      resetFunc(); //вызываем reset
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  } */
/*  if (Ethernet.linkStatus() == LinkOFF) {
      lcd.setCursor(0, 1);                              // установка курсора на начало первой строки
      lcd.print("No cable");  //
//    Serial.println("Ethernet cable is not connected.");
  } */

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
  lcd.print("I get IP adress");  // выводим текущий IP
  
//  lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
//  lcd.print("                ");  // выводим текущий IP
  lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
  lcd.print(Ethernet.localIP());  // выводим текущий IP

 }

void GenNewMAC() {
       // No a locally managed address, generate random address and store it.
//    #ifdef ENABLE_MAC_INIT_MESSAGES
//      Serial.println("GENERATE NEW MAC ADDR");
//    #endif
    lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
    for(int i=0;i<6;i++) {
      mac[i]=random(255);
      if(i==0) {mac[0]&=0xFC;mac[0]|=0x2;} // Make locally administered address

 //     EEPROM.update(i,mac[i]);

//      #ifdef ENABLE_MAC_INIT_MESSAGES
        if(mac[i]<11) mac[i]=mac[i]+10; //{Serial.print('0');}  // Print two digets
//        Serial.print(mac[i],HEX);Serial.print(":");
//      #endif
    }
    for (byte octet = 0; octet < 6; octet++) lcd.print(mac[octet], HEX);
//    #ifdef ENABLE_MAC_INIT_MESSAGES
//      Serial.println();
//    #endif
  //ethernet();
}

void exitProgNoSave () {
      flags.flagProg = 0;
      lcd.setCursor(15, 0);  // очищаем поле программирования
      lcd.print(" ");  //
      if (posit == 3){
        // eeprom_write_byte(10, devises_new);
        // devises = devises_new;
         lcd.setCursor(0, 1);
         lcd.print(devises);
      }
 //     value = 0;
 //     n = eeprom_read_byte(1); // перечитываем ID устройства из EEPROM
//      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
//      lcd.print("   ");  // вывод ID устройства
//      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
 //     lcd.print(n);  // вывод ID устройства
//      Serial.println("EXIT programming for timeout");
}

void DisplayMac () {
               // выводим МАК адрес
        lcd.clear();
 //       lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
 //       lcd.print("                ");  // очищаем
        lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
        if (!flags.flagProg) {
          lcd.print("My MAC adres:");
 //         lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
 //         lcd.print("                ");  // очищаем
          lcd.setCursor(0, 1);  //  установка курсора в начало 2 строки
         // byte macBuffer[6];  // create a buffer to hold the MAC address
         // Ethernet.MACAddress(macBuffer); // fill the buffer
          for (byte octet = 0; octet < 6; octet++) lcd.print(mac[octet], HEX);
        } else {
//          lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
          lcd.print("New MAC adress:");  // очищаем
          lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
          GenNewMAC();
/*          lcd.print("New MAC adres:");
          lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
          lcd.print("                ");  // очищаем
          lcd.setCursor(0, 1);  //  установка курсора в начало 1 строки
 //         lcd.print(NewMAC); */
        
      }
}
