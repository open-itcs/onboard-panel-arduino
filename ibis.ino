#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define buttonNext 11
#define buttonBack 10

int destCode = 0;
String desText = "Leerfeld"; 

String codes[] = {"     ", "XY\n Reisen", "Schulbus", "Dienstfahrt", "Pause", "Musterdorf", "Musterstadt", "Schulbus\Musterdorf", "Schulbus\Musterstadt"};
String texts[] = {"Leerfeld", "XY Reisen", "Schulbus", "Dienstfahrt", "Pause", "Musterdorf", "Musterstadt", "Musterdorf Schulb", "Musterstadt Schulb"};
String symbols[] = {"0", "0", "1", "0", "4", "0", "0", "1", "1"};

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
  
void setup() {
  lcd.begin(16,2);
  lcd.home();
  lcd.print("booting...");

  IBIS_init();
  pinMode(buttonNext, INPUT_PULLUP);
  pinMode(buttonBack, INPUT_PULLUP);
  delay(2000);
  IBIS_display(0);
}

void loop() {
  int nextButtonState = digitalRead(buttonNext);
  if (nextButtonState == LOW) {
    if (destCode == 7) {
      IBIS_display(0);
    } else {
      IBIS_display(destCode + 1);
    }
    while(digitalRead(buttonNext) == LOW);
  }
  int backButtonState = digitalRead(buttonBack);
  if (backButtonState == LOW) {
    if (destCode == 0) {
      IBIS_display(8);
    } else {
      IBIS_display(destCode - 1);
    }
    while(digitalRead(buttonBack) == LOW);
  }
}

void IBIS_display(int id) {
  lcd.clear();
  lcd.home();
  lcd.print("Ziel: " + String(id));
  lcd.setCursor ( 0, 1 );
  lcd.print(texts[id]);
  IBIS_symbol(symbols[id]);
  IBIS_DS021t("1", codes[id]);
  IBIS_DS021t("2", codes[id]);
  destCode = id;
  desText = texts[id];
}

char * createtelegram(char vals[]) {
  char result[8 + strlen(vals)];
  strcpy(result, "aA1");
  strcat(result, strchr(vals, 20) ? "2" : "1");
  strcat(result, vals);
  strcat(result, 20);
  strcat(result, 20);
  if (!strchr(vals, 20)) {
    strcat(result, 20);
  }
  strcat(result, 13);

  unsigned char checksum = strtol(12, NULL, 16);
  for (int i=0; i < strlen(result); i++) {
    checksum = checksum ^ strtol(result[i], NULL, 16);
  }

  return result;
}

void IBIS_init() {
  Serial.begin(1200, SERIAL_7E2);
  while (!Serial);
}

void IBIS_processSpecialCharacters(String* telegram) {
  telegram->replace("ä", "{");
  telegram->replace("ö", "|");
  telegram->replace("ü", "}");
  telegram->replace("ß", "~");
  telegram->replace("Ä", "[");
  telegram->replace("Ö", "\\");
  telegram->replace("Ü", "]");
}

String IBIS_vdvHex(byte value) {
  String vdvHexCharacters = "0123456789:;<=>?";
  String vdvHexValue;
  byte highNibble = value >> 4;
  byte lowNibble = value & 15;
  if (highNibble > 0) {
    vdvHexValue += vdvHexCharacters.charAt(highNibble);
  }
  vdvHexValue += vdvHexCharacters.charAt(lowNibble);
  return vdvHexValue;
}

String IBIS_wrapTelegram(String telegram) {
  telegram += '\x0d';
  unsigned char checksum = 0x7F;
  for (int i = 0; i < telegram.length(); i++) {
    checksum ^= (unsigned char)telegram[i];
  }
  // Get ready for a retarded fucking Arduino workaround
  telegram += " ";
  telegram.setCharAt(telegram.length() - 1, checksum); // seriously fuck that
  return telegram;
}

void IBIS_sendTelegram(String telegram) {
  IBIS_processSpecialCharacters(&telegram);
  telegram = IBIS_wrapTelegram(telegram);
  Serial.print(telegram);
}

void IBIS_DS021t(String address, String text) {
  String telegram;
  byte numBlocks = ceil(text.length() / 16.0);
  telegram = "aA";
  telegram += address;
  telegram += IBIS_vdvHex(numBlocks);
  telegram += "A0";
  if (text.indexOf('\n') > 0) {
    text += "\n" ;
  }
  text += "\n\n" ;
  telegram += text;
  byte remainder = text.length() % 16;
  if (remainder > 0) {
    for (byte i = 16; i > remainder; i--) {
      telegram += " ";
    }
  }
  IBIS_sendTelegram(telegram);
}

void IBIS_symbol(String number) {
  String telegram;
  telegram = "lE0";
  telegram += number;
  IBIS_sendTelegram(telegram);
}
