#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MPU6050.h>
#include <Servo.h>
#include <Keypad.h>

const int kirmiziLED = 2;
const int yesilLED = 3;
const int buzzer = 4;
const int RST_PIN = 9;
const int SS_PIN = 10;
const int SERVO_PIN = 5;

const byte SATIRLAR = 4;
const byte SUTUNLAR = 3;
byte satirPinleri[SATIRLAR] = {8, 7, 6, A0}; // satır pinleri
byte sutunPinleri[SUTUNLAR] = {A1, A2, A3}; // sütun pinleri

// Keypad tuş haritası (4x3)
char tuslar[SATIRLAR][SUTUNLAR] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu;
Servo servoMotor;
Keypad keypad = Keypad(makeKeymap(tuslar), satirPinleri, sutunPinleri, SATIRLAR, SUTUNLAR);

byte yetkiliKart[4] = {0x23, 0x16, 0x23, 0x2A};

const String DOGRU_SIFRE = "1717";
String girilenSifre = "";
bool sifreModuAktif = false;
unsigned long sifreBaslamaZamani = 0;
const unsigned long SIFRE_TIMEOUT = 15000; // 15 saniye

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t oncekiAx = 0, oncekiAy = 0, oncekiAz = 0;
bool hareketAlgilandi = false;
unsigned long sonHareketZamani = 0;
const int HAREKET_ESIGI = 20000;
const int ALARM_SURESI = 3000;

int servoBaslangicAci = 0;
int servoAcikAci = 90;
bool servoAcik = false;

unsigned long sonZamanGuncelleme = 0;
unsigned long sistemBaslamaZamani = 0;

#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_C6  1047

const int melody[] = {
  NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6
};

const int durations[] = {
  8, 8, 8, 4  
};

void setup() {
  pinMode(kirmiziLED, OUTPUT);
  pinMode(yesilLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(servoBaslangicAci);
  delay(500);
  
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();
  rfid.PCD_Init();
  
  sistemBaslamaZamani = millis();
  
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 bağlantısı başarılı!");
  } else {
    Serial.println("MPU6050 bağlantı hatası!");
  }
  
  lcd.init();
  lcd.backlight();
  
  Serial.println("=== GUVENLIK SISTEMI + SERVO + KEYPAD ===");
  Serial.println("RFID + Hareket + Servo + Keypad Aktif");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GUVENLIK SISTEMI");
  lcd.setCursor(3, 1);
  lcd.print("HAZIR");
  delay(2000);
  
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  oncekiAx = ax;
  oncekiAy = ay;
  oncekiAz = az;
  
  Serial.println("Servo motor başlangıç pozisyonunda (0 derece - kapalı)");
  Serial.println("Keypad aktif - * tuşu ile şifre modu");
  Serial.println("Sistem zamanı millis() kullanılarak takip ediliyor");
  
  hosgeldinMesaji();
  delay(1000);
}

void loop() {

  keypadKontrol();
  
  if (sifreModuAktif && (millis() - sifreBaslamaZamani > SIFRE_TIMEOUT)) {
    sifreModuIptal();
  }
  
  hareketKontrol();
  
  if (hareketAlgilandi && (millis() - sonHareketZamani < ALARM_SURESI)) {
    guvenlikAlarm();
    return;
  }
  
  if (!sifreModuAktif && (millis() - sonZamanGuncelleme > 2000)) {
    zamanGoster();
    sonZamanGuncelleme = millis();
  }
  
  if (!sifreModuAktif) {
    rfidKontrol();
  }
}

void hosgeldinMesaji() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("HOSGELDINIZ");
  lcd.setCursor(0, 1);
  lcd.print("*:Sifre Kart:OK");
}
void keypadKontrol() {
  char tusBas = keypad.getKey();
  
  if (tusBas) {
    Serial.print("Tus basildi: ");
    Serial.println(tusBas);
    
    tone(buzzer, 800, 50);
    delay(50);
    noTone(buzzer);
    
    if (tusBas == '*' && !sifreModuAktif) {
      sifreModuBaslat();
    }
    else if (sifreModuAktif) {
      if (tusBas == '#') {
        sifreKontrolEt();
      }
      else if (tusBas == '*') {
        sifreModuIptal();
      }
      else if ((tusBas >= '0' && tusBas <= '9') && girilenSifre.length() < 8) {
        girilenSifre += tusBas;
        sifreyiGoster();
      }
    }
  }
}

void sifreModuBaslat() {
  sifreModuAktif = true;
  girilenSifre = "";
  sifreBaslamaZamani = millis();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SIFRE GIRINIZ:");
  lcd.setCursor(0, 1);
  lcd.print("____  #:OK *:CIK");
  
   Serial.println("Sifre modu aktif");
}

void sifreyiGoster() {
  lcd.setCursor(0, 1);
  lcd.print("                "); 
  lcd.setCursor(0, 1);
  
  for (int i = 0; i < girilenSifre.length(); i++) {
    lcd.print('*');
  }
  
  for (int i = girilenSifre.length(); i < 4; i++) {
    lcd.print('_');
  }
  
  lcd.print("  #:OK *:CIK");
}

void sifreKontrolEt() {
  if (girilenSifre == DOGRU_SIFRE) {
    Serial.println("SIFRE DOGRU - ERISIM IZNI");
    sifreModuAktif = false;
    yetkiliGiris();
    delay(1000);
    hosgeldinMesaji();
  } else {
    Serial.println("SIFRE YANLIS - ERISIM RED");
    sifreModuAktif = false;
    yetkisizGiris();
    delay(1000);
    hosgeldinMesaji();
  }
  girilenSifre = "";
}

void sifreModuIptal() {
  sifreModuAktif = false;
  girilenSifre = "";
  
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("IPTAL EDILDI");
  delay(1500);
  hosgeldinMesaji();
  
  Serial.println("Sifre modu iptal edildi");
}

void rfidKontrol() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("KART OKUNDU");
  lcd.setCursor(3, 1);
  lcd.print("KONTROL...");
  delay(1000);
  
  Serial.print("Okunan UID: ");
  for (int i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  if (kartKontrol()) {
    Serial.println("YETKILI KART - ERISIM IZNI");
    yetkiliGiris();
  } else {
    Serial.println("YETKISIZ KART - ERISIM RED");
    yetkisizGiris();
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
  hosgeldinMesaji();
}

void zamanGoster() {
  lcd.clear();
  

  unsigned long gecenSure = millis() - sistemBaslamaZamani;
  unsigned long saniye = gecenSure / 1000;
  unsigned long dakika = saniye / 60;
  unsigned long saat = dakika / 60;
  
  saniye = saniye % 60;
  dakika = dakika % 60;
  saat = saat % 24;
  
  lcd.setCursor(0, 0);
  lcd.print("CALISMA SURESI:");
  
  lcd.setCursor(0, 1);
  if (saat < 10) lcd.print("0");
  lcd.print(saat);
  lcd.print(":");
  if (dakika < 10) lcd.print("0");
  lcd.print(dakika);
  lcd.print(":");
  if (saniye < 10) lcd.print("0");
  lcd.print(saniye);
  
  if (servoAcik) {
    lcd.print(" ACIK");
  } else {
    lcd.print(" AKTIF");
  }
}

void zamanliLog(String mesaj) {
  unsigned long gecenSure = millis() - sistemBaslamaZamani;
  unsigned long saniye = gecenSure / 1000;
  unsigned long dakika = saniye / 60;
  unsigned long saat = dakika / 60;
  
  saniye = saniye % 60;
  dakika = dakika % 60;
  saat = saat % 24;
  
  Serial.print("[");
  if (saat < 10) Serial.print("0");
  Serial.print(saat);
  Serial.print(":");
  if (dakika < 10) Serial.print("0");
  Serial.print(dakika);
  Serial.print(":");
  if (saniye < 10) Serial.print("0");
  Serial.print(saniye);
  Serial.print("] ");
  Serial.println(mesaj);
}

void hareketKontrol() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  int16_t deltaAx = abs(ax - oncekiAx);
  int16_t deltaAy = abs(ay - oncekiAy);
  int16_t deltaAz = abs(az - oncekiAz);
  
  int16_t toplamHareket = deltaAx + deltaAy + deltaAz;
  
  if (toplamHareket > HAREKET_ESIGI) {
    if (!hareketAlgilandi) {
      zamanliLog("!!! SISTEM ZORLANIYIR - GUVENLIK ALARMI !!!");
      Serial.print("Hareket seviyesi: ");
      Serial.println(toplamHareket);
      hareketAlgilandi = true;
      sonHareketZamani = millis();
    }
  }
  
  oncekiAx = ax;
  oncekiAy = ay;
  oncekiAz = az;
  
  if (hareketAlgilandi && (millis() - sonHareketZamani > ALARM_SURESI)) {
    hareketAlgilandi = false;
    zamanliLog("Güvenlik alarmı sona erdi");
  }
}

void guvenlikAlarm() {
  digitalWrite(kirmiziLED, HIGH);
  digitalWrite(yesilLED, LOW);
  
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("!!! ALARM !!!");
  
  unsigned long gecenSure = millis() - sistemBaslamaZamani;
  unsigned long saniye = gecenSure / 1000;
  unsigned long dakika = saniye / 60;
  unsigned long saat = dakika / 60;
  
  saniye = saniye % 60;
  dakika = dakika % 60;
  saat = saat % 24;
  
  lcd.setCursor(0, 1);
  if (saat < 10) lcd.print("0");
  lcd.print(saat);
  lcd.print(":");
  if (dakika < 10) lcd.print("0");
  lcd.print(dakika);
  lcd.print(" ZORLANDI");
  
  for (int i = 0; i < 5; i++) {
    tone(buzzer, 800, 100);
    delay(150);
    tone(buzzer, 400, 100);
    delay(150);
  }
  
  digitalWrite(kirmiziLED, LOW);
  delay(200);
}

bool kartKontrol() {
  for (int i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != yetkiliKart[i]) {
      return false;
    }
  }
  return true;
}

void yetkiliGiris() {
  zamanliLog("YETKILI GIRIS - ERISIM IZNI VERILDI");
  
  digitalWrite(yesilLED, HIGH);
  digitalWrite(kirmiziLED, LOW);
  
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("HOSGELDINIZ!");
  
  unsigned long gecenSure = millis() - sistemBaslamaZamani;
  unsigned long saniye = gecenSure / 1000;
  unsigned long dakika = saniye / 60;
  unsigned long saat = dakika / 60;
  
  saniye = saniye % 60;
  dakika = dakika % 60;
  saat = saat % 24;
  
  lcd.setCursor(0, 1);
  if (saat < 10) lcd.print("0");
  lcd.print(saat);
  lcd.print(":");
  if (dakika < 10) lcd.print("0");
  lcd.print(dakika);
  lcd.print(" GIRIS OK");
  
  int notaSayisi = sizeof(melody) / sizeof(melody[0]);
  for (int i = 0; i < notaSayisi; i++) {
    int notaSuresi = 1000 / durations[i];
    tone(buzzer, melody[i], notaSuresi);
    delay(notaSuresi * 1.1);
    noTone(buzzer);
  }
  
  Serial.println("Servo motor aktif - 90 derece dönüyor...");
  servoMotor.write(servoAcikAci);
  servoAcik = true;
  
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("KAPI ACIK");
  lcd.setCursor(1, 1);
  lcd.print("SERVO: 90 DEG");
  
  delay(3000);
  
  Serial.println("Servo motor kapalı pozisyona dönüyor...");
  servoMotor.write(servoBaslangicAci);
  servoAcik = false;
  
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("KAPI KAPALI");
  lcd.setCursor(2, 1);
  lcd.print("SERVO: 0 DEG");
  
  digitalWrite(yesilLED, LOW);
  delay(2000);
}

void yetkisizGiris() {
  zamanliLog("YETKISIZ GIRIS DENEMESI - ERISIM REDDEDILDI");
  
  digitalWrite(kirmiziLED, HIGH);
  digitalWrite(yesilLED, LOW);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("YETKISIZ GIRIS!");
  
  unsigned long gecenSure = millis() - sistemBaslamaZamani;
  unsigned long saniye = gecenSure / 1000;
  unsigned long dakika = saniye / 60;
  unsigned long saat = dakika / 60;
  
  saniye = saniye % 60;
  dakika = dakika % 60;
  saat = saat % 24;
  
  lcd.setCursor(0, 1);
  if (saat < 10) lcd.print("0");
  lcd.print(saat);
  lcd.print(":");
  if (dakika < 10) lcd.print("0");
  lcd.print(dakika);
  lcd.print(" REDDEDILDI");
  
  for (int i = 0; i < 3; i++) {
    tone(buzzer, 300, 200);
    delay(400);
    noTone(buzzer);
    delay(200);
  }
  digitalWrite(kirmiziLED, LOW);
  delay(3000);
}