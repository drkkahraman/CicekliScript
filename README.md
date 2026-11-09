# CicekliScript 🚀

**CicekliScript**, Python'ın pratikliği ve yüksek seviyeli sözdizimi ile C dilinin gücünü, blok yapısını (`{}`) ve hızını birleştiren; her cümlenin sonuna **`;` (noktalı virgül)** koymanın zorunlu olduğu modern, dinamik ve C tabanlı bir programlama dilidir!

---

## 🌟 Temel Özellikler

1. **C Tabanlı**: Yorumlayıcı tamamen sıfırdan saf C (C11) ile yazılmıştır, son derece hafif ve hızlıdır.
2. **Python Kolaylığı**: Dinamik tipler, listeler `[1, 2, 3]`, sözlükler/haritalar `{"anahtar": "deger"}`, dilimleme ve negatif indeksleme (`[-1]`).
3. **Zorunlu Noktalı Virgül (`;`)**: Tıpkı C'de olduğu gibi tüm ifadelerin sonuna `;` konulması zorunludur!
4. **`cicekli` Çekirdek Modülü** (Geriye uyumluluk için `mete` de desteklenir):
   - `a = cicekli.input("Cümle");` ile kullanıcıdan girdi alma
   - `cicekli.println("Hello World");` ile ekrana satır sonlu yazdırma
   - `cicekli.print(...)` ile ekrana satır sonu olmadan yazdırma
   - `cicekli.deffunction` veya `def` ile isimli veya anonim fonksiyon tanımlama
5. **Esnek Döngü Desteği**:
   - Python tarzı: `for (eleman in liste) { ... };`
   - C tarzı: `for (i = 0; i < 10; i = i + 1) { ... };`
   - While döngüsü: `while (kosul) { ... };`
   - `break;` ve `continue;` kontrolleri
6. **Çift Yorum Desteği**:
   - `# Python tarzı yorum`
   - `// C tarzı tek satır yorum`
   - `/* C tarzı çok satırlı yorum */`
7. **Etkileşimli Kabuk (REPL)**: Terminalde `cicekliscript` yazarak canlı deneme yapabilme.

---

## 🛠️ Kurulum ve Derleme

CicekliScript doğrudan GCC ve Make ile tek komutla derlenebilir:

```bash
make
make install
```

Derlenen ikili dosya `bin/cicekliscript` altında ve `~/.local/bin/cicekliscript` yolunda hazır haldedir.

---

## 💻 Kullanım Şekilleri

### 1. Etkileşimli REPL Kabuğu
```bash
cicekliscript
```
```
╔═══════════════════════════════════════════════════╗
║          CicekliScript v1.0.0 REPL                ║
║  Python kolaylığı + C gücü + Zorunlu ';'         ║
║  Çıkmak için: exit() veya cicekli.exit();        ║
╚═══════════════════════════════════════════════════╝

cicekli>>> cicekli.println("Merhaba Dünya!");
Merhaba Dünya!
cicekli>>> a = 10; b = 20;
cicekli>>> cicekli.println("Toplam:", a + b);
Toplam: 30
cicekli>>> exit();
```

### 2. Dosyadan Betik Çalıştırma
```bash
cicekliscript script.ccl
```

### 3. Tek Satır Komut Çalıştırma (`-c`)
```bash
cicekliscript -c 'cicekli.println("Hello World");'
```

---

## 📖 Sözdizimi Rehberi

### 1. Girdi ve Çıktı (I/O)
```javascript
// Ekrana yazdırma
cicekli.println("Hello World");

// Kullanıcıdan girdi alma (input)
a = cicekli.input("Cümle giriniz: ");
cicekli.println("Girdiğiniz cümle:", a);
```

### 2. Fonksiyon Tanımlama (`cicekli.deffunction` / `def`)

#### İsimli Fonksiyon:
```javascript
cicekli.deffunction topla(a, b) {
    return a + b;
};

sonuc = topla(15, 25);
cicekli.println("Sonuç:", sonuc);
```

#### Değişkene Atanan Anonim Fonksiyon:
```javascript
kare = cicekli.deffunction(x) {
    return x * x;
};

cicekli.println("5'in karesi:", kare(5));
```

#### Anonim Olarak Doğrudan Çalıştırma:
```javascript
cicekli.deffunction() {
    cicekli.println("Bu blok hemen çalışır!");
}();
```

### 3. GUI (Pencere ve Grafik) Desteği

CicekliScript ile doğrudan donanım hızlandırmalı ve çift arabelleğe (double buffering) sahip GUI pencereleri oluşturabilirsiniz:

```javascript
// 400x300 boyutunda pencere aç:
window = cicekli.window(400, 300, "Uygulama Penceresi");

// Arka plan rengini ayarla:
window.color(); // veya window.color("red"); veya window.color("#1e1e2e");

// Metin ekle: window.text(metin, renk, boyut, x, y);
window.text("CicekliScript", "red", 16, 20, 35);

// Geometrik şekiller:
window.rect(10, 50, 40, 20, "blue");
window.circle(70, 60, 15, "green");

// Pencereyi açık tutan olay döngüsü:
window.mainloop();
```

### 4. Koşul İfadeleri (`if`, `elif`, `else`)
```javascript
puan = 85;

if (puan >= 90) {
    cicekli.println("Harf notu: AA");
} elif (puan >= 70) {
    cicekli.println("Harf notu: BB");
} else {
    cicekli.println("Harf notu: CC");
};
```

### 5. Döngüler

#### Python Tarzı `for..in`:
```javascript
meyveler = ["Elma", "Armut", "Muz"];
for (m in meyveler) {
    cicekli.println("Meyve:", m);
};

// range() ile kullanım:
for (i in cicekli.range(5)) {
    cicekli.println("İndeks:", i);
};
```

#### C Tarzı `for(init; cond; step)`:
```javascript
for (i = 0; i < 5; i = i + 1) {
    cicekli.println("Adım:", i);
};
```

#### `while` Döngüsü:
```javascript
sayac = 0;
while (sayac < 5) {
    sayac += 1;
    if (sayac == 2) {
        continue;
    };
    cicekli.println("Sayaç:", sayac);
};
```

### 6. Veri Tipleri

- **Sayılar**: `10`, `3.14`, `-42`
- **Metinler**: `"Merhaba"`, `'CicekliScript'`, `\n`, `\t` kaçış dizileri, metin birleştirme `+`
- **Mantıksal**: `true`, `false`, `True`, `False`
- **Boş Değer**: `null`, `None`, `nil`
- **Dinamik Listeler**: `[1, 2, "üç", [4, 5]]`
  - Eleman ekleme: `cicekli.append(liste, eleman);` veya `liste.append(eleman);`
  - Eleman çıkarma: `cicekli.pop(liste);` veya `liste.pop();`
  - Uzunluk: `cicekli.len(liste);` veya `liste.len();`
  - Negatif indeksleme: `liste[-1]`
- **Sözlükler (Maps)**: `{"ad": "Çiçek", "yas": 25}`
  - Nokta erişimi: `kullanici.ad`
  - İndeks erişimi: `kullanici["yas"]`

---

## 📂 Örnek Dosyalar

Tüm örnekler `examples/` klasöründe yer almaktadır:
- `examples/01_merhaba.ccl` - Girdi/çıktı ve değişkenler
- `examples/02_fonksiyonlar.ccl` - Fonksiyon varyasyonları
- `examples/03_donguler.ccl` - `for in`, `for(;;)` ve `while` döngüleri
- `examples/04_fibonacci.ccl` - Özyinelemeli (recursive) algoritma ve zaman ölçümü
- `examples/05_listeler_ve_sozlukler.ccl` - Dinamik listeler ve haritalar
- `examples/06_hesap_makinesi.ccl` - Fonksiyonel hesap makinesi
- `examples/07_gui_pencere.ccl` - GUI temel pencere örneği
- `examples/08_gui_gelismis.ccl` - İnteraktif GUI animasyon ve çizim örneği
