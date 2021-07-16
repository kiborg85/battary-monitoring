#include <avr/eeprom.h>
#include "GyverWDT.h"
//#include "AnalogKey.h"
#define BTN_PIN A2        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
#include "GyverButton.h"
GButton butt1(BTN_PIN);
// принимаем запрос через SoftwareSerial
// отвечаем на запрос
// связь двухсторонняя!!!! Это скетч ПРИЁМНИКА
// TX передатчика подключен к RX приёмника
// RX передатчика подключен к TX приёмника

#include <SoftwareSerial.h>
SoftwareSerial mySerial(3, 2); // RX, TX

#include "GBUS.h"
byte n = eeprom_read_byte(1); //1;  //НОМЕР УСТРОЙСТВАh
byte dataB_read = 0;
GBUS bus(&mySerial, n, 5);  // адрес 4, буфер 20 байт

//const int ledRed = 5;
//const int ledElov = 4;
const int ledBlu = 13;
//const int ledGren = 2;
const int ledWite = 6;
//const int button = A2;
bool ledState = LOW; 
bool flagProg = LOW;
float Vcc;  //внутреннее опорное напряжение
#define NUM_READS 20  //количество измерений вольтажа для вычисления среднего
const float typVbg = 1.095; // 1.0 -- 1.2

    static uint32_t tmr;
    static uint32_t tmr_readVolt;
    static uint32_t tmr_whiteLed;
    static uint32_t tmr_prog;
    
// подключение библиотек
//#include <Wire.h> 
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);  // инициализация дисплея

struct Str {
  bool temp;
  byte ledRed_state;
  byte ledElov_state;
  byte ledGren_state;
};
Str buf;

const int analogIn = A1;
float val111 = 0;
int value = 0;
void setup() {
 
  
  // родной сериал открываю для наблюдения за процессом
  Serial.begin(9600);
  Serial.print("ID mode: "); Serial.println(n);

  // этот сериал будет принимать и отправлять данные
  mySerial.begin(9600);

  lcd.begin(16, 2);  // инициализация lcd
  lcd.setCursor(0, 0);  //  установка курсора в начало 1 строки
  lcd.print("       Vin:");  // вывод текста
  lcd.setCursor(0, 1);  // установка курсора на 10 позицию
  lcd.print("Id     Vcc:");  // вывод значения температуры
  lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
  lcd.print(n);  // вывод ID устройства

  pinMode(ledBlu, OUTPUT);
  pinMode(ledWite, OUTPUT);
  digitalWrite(ledBlu, LOW);
  digitalWrite(ledWite, LOW);
  //attachInterrupt(1, isr, CHANGE);
  butt1.setDebounce(100);      // настройка антидребезга (по умолчанию 80 мс)
  butt1.setTimeout(500);      // настройка таймаута на удержание (по умолчанию 500 мс)

  
    Watchdog.enable(RESET_MODE, WDT_PRESCALER_1024); // Режим сторжевого сброса , таймаут ~8с
//     eeprom_write_byte(1, n);
}
void(* resetFunc) (void) = 0;//объявляем функцию reset с адресом 0

void isr() {
  butt1.tick();  // опрашиваем в прерывании, чтобы поймать нажатие в любом случае
}


void loop() {
  butt1.tick();
  if (butt1.isClick()) {
    Serial.println("Click");         // проверка на один клик
    
  }
  if (butt1.isSingle()) {
    Serial.println("Single");       // проверка на один клик
    
  }
  if (butt1.isDouble()) {                               // проверка на двойной клик
    Serial.print("Double");
    dataB_read = eeprom_read_byte(1);
    Serial.print(" Read data "); Serial.print(dataB_read);
    //if (!flagProg) { lcd.setCursor(3, 0); lcd.print(dataB_read);}
  }
  if (butt1.isTriple()) {                               // проверка на тройной клик
    Serial.println("Triple");       
    if (flagProg) {
       value = 1;
    } else {
      tmr_whiteLed = millis();
      digitalWrite(ledWite, LOW);
    }
  }
  if (butt1.hasClicks())                                // проверка на наличие нажатий
    { Serial.println(butt1.getClicks());                  // получить (и вывести) число нажатий
      if (butt1.getClicks() == 4 && flagProg) exitProgNoSave ();
    }
  if (butt1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
  if (butt1.isRelease()) {                              // отпускание кнопки (+ дебаунс)
    Serial.println("Release");     
    if (flagProg) {
      tmr_prog = millis();
      value++;
  //    Serial.println(value);  
//      eeprom_write_byte(1, value);
      n = eeprom_read_byte(1); lcd.setCursor(5, 0); lcd.print(n);  // вывод ID устройства
      /*if (!ledState) {
        lcd.setCursor(3, 1); lcd.print(value);
      } else {
        lcd.setCursor(3, 1); lcd.print("     ");
      } */
    } else {
      value = 0;
      dataB_read = eeprom_read_byte(1);
      n = dataB_read;
      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
      lcd.print("   ");  // вывод ID устройства
      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
      lcd.print(n,1);  // вывод ID устройства
      GBUS bus(&mySerial, n, 5);
    }
    
  }
  if (butt1.isHolded()) {                               // проверка на удержание
    Serial.println("Holded! Programming");
    flagProg = !flagProg;
    tmr_prog = millis();
    if (flagProg) {                                     //Входим в режим программирования
      lcd.setCursor(0, 0);                              // установка курсора на начало первой строки
      lcd.print("PROG");  // 
    } else {
      eeprom_write_byte(1, value);
      resetFunc();
      lcd.setCursor(0, 0);  // установка курсора на начало первой строки
      lcd.print("       ");  // выходим из режима программирования
    }
  }
 // if (butt1.isHold()) Serial.println("Hold");         // возвращает состояние кнопки
//  if (butt1.isStep()) {                                 // если кнопка была удержана (это для инкремента)
 //   value++;                                            // увеличивать/уменьшать переменную value с шагом и интервалом
//    Serial.println(value);                              // для примера выведем в порт
//    eeprom_write_byte(1, value);
 // }
  

  // здесь принимаются данные
  // если это аппаратный сериал - слишком часто его опрашивать даже не нужно
  bus.tick();

    if (millis() - tmr > 500) {
       tmr = millis();

       ledState = !ledState;
       digitalWrite(ledBlu, ledState);
     if (flagProg) {
      if (!ledState) {
        lcd.setCursor(3, 1); lcd.print(value);
      } else {
        lcd.setCursor(3, 1); lcd.print("    ");
      }
     }
    }
    if (millis() - tmr_prog > 120000 && flagProg) { //2 минуты и выходим из программирования без сохранения
      exitProgNoSave ();
    }
    if (millis() - tmr_readVolt > 5000) {  //считываем значение вольтажа аккумулятора
      tmr_readVolt = millis();
      
//digitalWrite(ledElov, HIGH);
//      lcd.setCursor(6, 0);  // установка курсора на 10 позицию
//      lcd.print("  ");  // вывод на экран значения влажности
       Vcc = readVcc(); //хитрое считывание опорного напряжения (функция readVcc() находится ниже)
       val111 = (readAnalog(A1) * Vcc)/1023/0.242424242424242;
      Serial.print ("val111 = "); Serial.print (val111);Serial.print (" Vcc = "); Serial.println (Vcc); //Serial.print (" val_sred = "); Serial.print (val_sred);
      lcd.setCursor(11, 1);  // установка курсора на 10 позицию
      lcd.print("     ");  // вывод на экран значения влажности
      lcd.setCursor(11, 1);  // установка курсора на 10 позицию
      lcd.print(Vcc, 2);  // вывод на экран значения влажности
      lcd.setCursor(11, 0);  // установка курсора на 10 позицию
      lcd.print("     ");  // вывод на экран значения влажности
      lcd.setCursor(11, 0);  // установка курсора на 10 позицию      
      lcd.print(val111, 2);  // вывод на экран значения влажности
 
    }
  
  
    if (millis() - tmr_whiteLed > 120000) digitalWrite(ledWite, HIGH);   // давно не общались с гл.модулем. Светим беленьким

  
  // приняли данные
  if (bus.gotRequest()) {
      tmr_whiteLed = millis();
      digitalWrite(ledWite, LOW);
      bus.readData(buf);
      Serial.print ("temp: "); Serial.print (buf.ledRed_state);Serial.print ("ledRed: "); Serial.print (buf.ledRed_state);Serial.print (" ledGren: "); Serial.print (buf.ledGren_state);Serial.print (" ledElov: "); Serial.println (buf.ledElov_state);


    bus.sendData(bus.getTXaddress(), val111);
    Serial.print("sending: ");
    Serial.println(val111);
  }
  Watchdog.reset(); // Переодический сброс watchdog, означающий, что устройство не зависло
}

//----------фильтр данных (для уменьшения шумов и разброса данных)-------
float readVcc() {
  // read multiple values and sort them to take the mode
  float sortedValues[NUM_READS];
  for (int i = 0; i < NUM_READS; i++) {
    bus.tick();
    butt1.tick();
    float tmp = 0.0;
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    ADCSRA |= _BV(ADSC); // Start conversion
    delay(25);
    while (bit_is_set(ADCSRA, ADSC)); // measuring
    uint8_t low = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both
    tmp = (high << 8) | low;
    float value = (typVbg * 1023.0) / tmp;
    int j;
    if (value < sortedValues[0] || i == 0) {
      j = 0; //insert at first position
    }
    else {
      for (j = 1; j < i; j++) {
        if (sortedValues[j - 1] <= value && sortedValues[j] >= value) {
          // j is insert position
          break;
        }
      }
    }
    for (int k = i; k > j; k--) {
      // move all values higher than current reading up one position
      sortedValues[k] = sortedValues[k - 1];
    }
    sortedValues[j] = value; //insert current reading
  }
  //return scaled mode of 10 values
  float returnval = 0;
  for (int i = NUM_READS / 2 - 5; i < (NUM_READS / 2 + 5); i++) {
    returnval += sortedValues[i];
  }
  return returnval / 10;
}
//----------фильтр данных (для уменьшения шумов и разброса данных) КОНЕЦ-------
//----------Функция точного определения опорного напряжения для измерения напряжения на акуме-------
float readAnalog(int pin) {  
  // read multiple values and sort them to take the mode
  int sortedValues[NUM_READS];
  for (int i = 0; i < NUM_READS; i++) {
    bus.tick();
    butt1.tick();
    delay(25);    
    int value = analogRead(pin);
    int j;
    if (value < sortedValues[0] || i == 0) {
      j = 0; //insert at first position
    }
    else {
      for (j = 1; j < i; j++) {
        if (sortedValues[j - 1] <= value && sortedValues[j] >= value) {
          // j is insert position
          break;
        }
      }
    }
    for (int k = i; k > j; k--) {
      // move all values higher than current reading up one position
      sortedValues[k] = sortedValues[k - 1];
    }
    sortedValues[j] = value; //insert current reading
  }
  //return scaled mode of 10 values
  float returnval = 0;
  for (int i = NUM_READS / 2 - 5; i < (NUM_READS / 2 + 5); i++) {
    returnval += sortedValues[i];
  }
  return returnval / 10;
}
//----------Функция точного определения опорного напряжения для измерения напряжения на акуме КОНЕЦ-------
void exitProgNoSave () {
      flagProg = !flagProg;
      lcd.setCursor(0, 0);  // очищаем поле программирования
      lcd.print("       ");  //
      value = 0;
      n = eeprom_read_byte(1); // перечитываем ID устройства из EEPROM
      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
      lcd.print("   ");  // вывод ID устройства
      lcd.setCursor(3, 1);  // установка курсора на 4 позицию второй строки
      lcd.print(n);  // вывод ID устройства
}
