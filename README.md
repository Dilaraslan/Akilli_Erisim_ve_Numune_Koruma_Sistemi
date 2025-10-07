Akıllı Erişim ve Numune Koruma Sistemi

Bu proje, **Arduino tabanlı bir güvenlik ve erişim kontrol sistemi**dir.  
Sistem; **RFID kart okuma**, **şifre girişi (keypad)**, **hareket algılama (MPU6050)**, **servo motor kontrollü kapı mekanizması** ve **alarm sistemi** içerir.  
LCD ekran üzerinden kullanıcı bilgilendirmesi yapılır.

---

Kullanılan Donanımlar

| Bileşen | Açıklama |
|----------|-----------|
| **Arduino UNO** | Ana kontrol kartı |
| **MFRC522** | RFID kart okuyucu modül |
| **MPU6050** | Hareket (ivme + jiroskop) sensörü |
| **LCD 16x2 (I2C)** | Durum ekranı |
| **Keypad (4x3)** | Şifre girişi için tuş takımı |
| **Servo Motor** | Kapı açma/kapama mekanizması |
| **Buzzer & LED** | Alarm ve durum göstergeleri |

---

Çalışma Prensibi

1. **Sistem açıldığında** LCD’de “Hazır” mesajı gösterilir.  
2. Kullanıcı:
   - Yetkili **RFID kartı** okutabilir **veya**
   - `*` tuşu ile **şifre giriş moduna** geçebilir.
3. Şifre doğru girilirse veya yetkili kart tanınırsa:
   - **Servo motor** 90° dönerek kapıyı açar.
   - LCD’de “Giriş OK” mesajı görünür.
   - Kısa bir melodi çalar.
4. **Yetkisiz giriş veya yanlış şifre** durumunda:
   - Kırmızı LED ve buzzer ile **alarm** devreye girer.
5. **MPU6050 sensörü**, fiziksel zorlamaları algılar.
   - Şiddetli hareket algılanırsa sistem **“!!! ALARM !!!”** durumuna geçer.
6. Ekranda çalışma süresi sürekli olarak görüntülenir.

---

Güvenlik Özellikleri

- **RFID doğrulama:** Kart UID kontrolü  
- **Şifre girişi (keypad):** Kullanıcı tanımlı PIN  
- **Zaman aşımı:** Şifre girilmezse 15 saniye sonra iptal  
- **Titreşim algılama:** Fiziksel zorlamaya karşı alarm
