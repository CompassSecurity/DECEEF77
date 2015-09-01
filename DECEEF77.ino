#include <LiquidCrystal.h>

//DCF77 Pin:
#define OutputPin 11
#define CntValue 16000000 / 77500 / 2
#define Firmware "V.1.3.0"

int timer1_counter;

int menuPlus = 8;
int menuMinus = 10;
int menuEnter = 9;

char* nameWochentag[] = {"Montag", "Dienstag", "Mittwoch", "Donnerst.", "Freitag", "Samstag", "Sonntag"};
char* nameJahreszeit[] = {"Winter", "Sommer"};
boolean DCFbitCode[59];

byte minuten = 0;
byte stunden = 0;
byte tag = 1;
byte wochentag = 1; // Montag=1
byte monat = 1;
byte jahr = 15;
boolean sommerzeit = true;

// ************** Display Pins ******************
LiquidCrystal lcd(12, 7, 5, 4, 3, 2);

byte customTXchar[8] = {
  0b11100,
  0b01000,
  0b01000,
  0b00000,
  0b10100,
  0b01000,
  0b10100,
  0b00000
};

byte customBarChar[8] = {
  0b00000,
  0b00001,
  0b00001,
  0b00101,
  0b00101,
  0b10101,
  0b10101,
  0b10101
};


void setup()
{
  lcd.createChar(0, customTXchar);
  lcd.createChar(1, customBarChar);
  // ************************** 77.5kHz Clock *****************************
  pinMode(OutputPin, INPUT); // 77.5kHz Clock
  TCCR2B = 0;                // Stop timer2
  TCNT2 = 0;                 // Clear timer2 counter
  TCCR2B = _BV(WGM22);       // Fast PWM mode, TOP = OCR2A
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2A |= _BV(COM2A0);     // Toggle OC2A on Compare Match
  OCR2A = CntValue;          // Set timer2 TOP value
  TCCR2B |= _BV(CS20);       // Enable timer2, divide-by-1 prescale

  // ***** set up the LCD's number of columns and rows: **********
  lcd.begin(8, 2);

  // **************** Set Time ********************
  // initialize the digital pin as an output.
  //  pinMode(OutputPin, OUTPUT);
  pinMode(menuPlus, INPUT_PULLUP);
  pinMode(menuMinus, INPUT_PULLUP);
  pinMode(menuEnter, INPUT_PULLUP);

  if (!digitalRead(menuEnter)) {
    displayVersion();
  }

  setStunden();
  setMinuten();
  setTag();
  setWochentag();
  setMonat();
  setJahr();
  setSommer();

  DispCurrTime();
  plusOneMin();
}



void loop()
{
  DCFcalc();

  pinMode(OutputPin, OUTPUT);
  delay(996);
  for (int i = 0; i < 59; i++) {
    if (DCFbitCode[i]) {
      pinMode(OutputPin, INPUT);  // Bit0=0
      lcd.setCursor(7, 0);
      lcd.write(" ");
      delay(200);
      lcd.setCursor(7, 0);
      lcd.write((uint8_t)1);
      pinMode(OutputPin, OUTPUT);
      delay(800);
    } else {
      pinMode(OutputPin, INPUT);  // Bit0=0
      lcd.setCursor(7, 0);
      lcd.write(" ");
      delay(100);
      lcd.setCursor(7, 0);
      lcd.write((uint8_t)1);
      pinMode(OutputPin, OUTPUT);
      delay(900);
    }
  }
  pinMode(OutputPin, OUTPUT);
  DispCurrTime();
  plusOneMin();
}

void displayVersion() {

  lcd.clear();
  lcd.print("DECEEF77");
  lcd.setCursor(0, 1);
  lcd.print(Firmware);
  while (!digitalRead(menuEnter)) {}
  delay(300);
  lcd.clear();

}

void plusOneMin() {
  minuten = minuten + 1;
  if (minuten == 60) {
    minuten = 0;
    stunden = stunden + 1;
  }
  if (stunden == 24) {
    stunden = 0;
    tag = tag + 1;
    wochentag = wochentag + 1;
  }
  if ((tag == 32) and ((monat == 1) or (monat == 3) or (monat == 5) or (monat == 7) or (monat == 8) or (monat == 10) or (monat == 12))) {
    tag = 1;
    monat = monat + 1;
  }
  if ((tag == 31) and ((monat == 4) or (monat == 6) or (monat == 9) or (monat == 11))) {
    tag = 1;
    monat = monat + 1;
  }
  if ((tag == 29) and (monat == 2)) {
    tag = 1;
    monat = monat + 1;
  }
  if (monat == 13) {
    monat = 1;
    jahr = jahr + 1;
  }
  if (wochentag == 8) {
    wochentag = 1;
  }
}



void DispCurrTime() {
  //  lcd.print();
  lcd.clear();
  if (stunden < 10) {
    lcd.print("0");
  }
  lcd.print(stunden);
  lcd.print(":");
  if (minuten < 10) {
    lcd.print("0");
  }
  lcd.print(minuten);

  lcd.print(" ");
  lcd.write((uint8_t)0);
  lcd.write((uint8_t)1);

  lcd.setCursor(0, 1);

  lcd.print(tag);
  lcd.print(".");
  lcd.print(monat);
  lcd.print(".");
  lcd.print(jahr);
}

void DCFcalc() {
  boolean parity = 0;
  for (int j = 0; j < 14 ; j++) { // Wetterdaten = 0
    DCFbitCode[j] = 0;
  }
  DCFbitCode[15] = 0; // Antennenbit
  DCFbitCode[16] = 0; // am Ende der Stunde wird MEZ/MESZ geändert
  if (sommerzeit) {
    DCFbitCode[17] = 1; //
    DCFbitCode[18] = 0; //
  } else {
    DCFbitCode[17] = 0; //
    DCFbitCode[18] = 1; //
  }
  DCFbitCode[19] = 0; // am Ende wird Schaltsekunde eingeführt
  DCFbitCode[20] = 1; //

  byte basesystem_min[7] = {40, 20, 10, 8, 4, 2, 1};
  parity = 0;
  int minutenTemp = minuten;

  for (int j = 0; j < 7; j++) {
    if (minutenTemp >= basesystem_min[j]) {
      DCFbitCode[27 - j] = 1;
      minutenTemp = minutenTemp - basesystem_min[j];
      parity = !parity;
    } else {
      DCFbitCode[27 - j] = 0;
    }
  }
  DCFbitCode[28] = parity;



  byte basesystem_std[6] = {20, 10, 8, 4, 2, 1};
  parity = 0;
  int stundenTemp = stunden;

  for (int j = 0; j < 6; j++) {
    if (stundenTemp >= basesystem_std[j]) {
      DCFbitCode[34 - j] = 1;
      stundenTemp = stundenTemp - basesystem_std[j];
      parity = !parity;
    } else {
      DCFbitCode[34 - j] = 0;
    }
  }
  DCFbitCode[35] = parity;


  byte basesystem_tag[6] = {20, 10, 8, 4, 2, 1};
  parity = 0;
  int tagTemp = tag;

  for (int j = 0; j < 6; j++) {
    if (tagTemp >= basesystem_tag[j]) {
      DCFbitCode[41 - j] = 1;
      tagTemp = tagTemp - basesystem_tag[j];
      parity = !parity;
    } else {
      DCFbitCode[41 - j] = 0;
    }
  }


  byte basesystem_wt[3] = {4, 2, 1};
  int wtTemp = wochentag;

  for (int j = 0; j < 3; j++) {
    if (wtTemp >= basesystem_wt[j]) {
      DCFbitCode[44 - j] = 1;
      wtTemp = wtTemp - basesystem_wt[j];
      parity = !parity;
    } else {
      DCFbitCode[44 - j] = 0;
    }
  }

  byte basesystem_monat[5] = {10, 8, 4, 2, 1};
  int monTemp = monat;

  for (int j = 0; j < 5; j++) {
    if (monTemp >= basesystem_monat[j]) {
      DCFbitCode[49 - j] = 1;
      monTemp = monTemp - basesystem_monat[j];
      parity = !parity;
    } else {
      DCFbitCode[49 - j] = 0;
    }
  }


  byte basesystem_jahr[8] = {80, 40, 20, 10, 8, 4, 2, 1};
  int jahrTemp = jahr;

  for (int j = 0; j < 8; j++) {
    if (jahrTemp >= basesystem_jahr[j]) {
      DCFbitCode[57 - j] = 1;
      jahrTemp = jahrTemp - basesystem_jahr[j];
      parity = !parity;
    } else {
      DCFbitCode[57 - j] = 0;
    }
  }
  DCFbitCode[58] = parity;

}


void setStunden()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stunden:");
  lcd.setCursor(0, 1);
  lcd.print(stunden);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      stunden = stunden + 1;
      if (stunden > 23) {
        stunden = 0;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stunden:");
      lcd.setCursor(0, 1);
      lcd.print(stunden);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      stunden = stunden - 1;
      if (stunden > 23) {
        stunden = 23;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stunden:");
      lcd.setCursor(0, 1);
      lcd.print(stunden);
      delay(300);
    }
  }
}

void setMinuten()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Minuten:");
  lcd.setCursor(0, 1);
  lcd.print(minuten);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      minuten = minuten + 1;
      if (minuten > 59) {
        minuten = 0;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Minuten:");
      lcd.setCursor(0, 1);
      lcd.print(minuten);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      minuten = minuten - 1;
      if (minuten > 59) {
        minuten = 59;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Minuten:");
      lcd.setCursor(0, 1);
      lcd.print(minuten);
      delay(300);
    }
  }
}

void setTag()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tag:");
  lcd.setCursor(0, 1);
  lcd.print(tag);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      tag = tag + 1;
      if (tag > 31) {
        tag = 1;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tag:");
      lcd.setCursor(0, 1);
      lcd.print(tag);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      tag = tag - 1;
      if ((tag > 31) or (tag == 0)) {
        tag = 31;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tag:");
      lcd.setCursor(0, 1);
      lcd.print(tag);
      delay(300);
    }
  }
}


void setWochentag()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(nameWochentag[wochentag - 1]);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      wochentag = wochentag + 1;
      if (wochentag > 7) {
        wochentag = 1;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(nameWochentag[wochentag - 1]);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      wochentag = wochentag - 1;
      if ((wochentag > 7) or (wochentag <= 0)) {
        wochentag = 7;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(nameWochentag[wochentag - 1]);
      delay(300);
    }
  }
}


void setMonat()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monat:");
  lcd.setCursor(0, 1);
  lcd.print(monat);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      monat = monat + 1;
      if (monat > 12) {
        monat = 1;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Monat:");
      lcd.setCursor(0, 1);
      lcd.print(monat);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      monat = monat - 1;
      if ((monat > 12) or (monat == 0)) {
        monat = 12;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Monat:");
      lcd.setCursor(0, 1);
      lcd.print(monat);
      delay(300);
    }
  }
}



void setJahr()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Jahr:");
  lcd.setCursor(0, 1);
  lcd.print(jahr);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      jahr = jahr + 1;
      if (jahr > 99) {
        jahr = 0;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jahr:");
      lcd.setCursor(0, 1);
      lcd.print(jahr);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      jahr = jahr - 1;
      if ((jahr > 99)) {
        jahr = 99;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jahr:");
      lcd.setCursor(0, 1);
      lcd.print(jahr);
      delay(300);
    }
  }
}


void setSommer()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(nameJahreszeit[sommerzeit]);
  delay(600);
  while (digitalRead(menuEnter)) {
    if (!digitalRead(menuPlus)) {
      sommerzeit = !sommerzeit;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(nameJahreszeit[sommerzeit]);
      delay(300);
    }
    if (!digitalRead(menuMinus)) {
      sommerzeit = !sommerzeit;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(nameJahreszeit[sommerzeit]);
      delay(300);
    }
  }
}

