# MeteScript 🚀

**MeteScript**, Python'ın pratikliği ve yüksek seviyeli sözdizimi ile C dilinin gücünü, blok yapısını (`{}`) ve hızını birleştiren; her cümlenin sonuna **`;` (noktalı virgül)** koymanın zorunlu olduğu modern, dinamik ve C tabanlı bir programlama dilidir!

---

## 🌟 Temel Özellikler

1. **C Tabanlı**: Yorumlayıcı tamamen sıfırdan saf C (C11) ile yazılmıştır, son derece hafif ve hızlıdır.
2. **Python Kolaylığı**: Dinamik tipler, listeler `[1, 2, 3]`, sözlükler/haritalar `{"anahtar": "deger"}`, dilimleme ve negatif indeksleme (`[-1]`).
3. **Zorunlu Noktalı Virgül (`;`)**: Tıpkı C'de olduğu gibi tüm ifadelerin sonuna `;` konulması zorunludur!
4. **`mete` Çekirdek Modülü**:
   - `a = mete.input("Cümle");` ile kullanıcıdan girdi alma
   - `mete.println("Hello World");` ile ekrana satır sonlu yazdırma
   - `mete.print(...)` ile ekrana satır sonu olmadan yazdırma
   - `mete.deffunction` ile isimli veya anonim fonksiyon tanımlama
5. **Esnek Döngü Desteği**:
   - Python tarzı: `for (eleman in liste) { ... };`
   - C tarzı: `for (i = 0; i < 10; i = i + 1) { ... };`
   - While döngüsü: `while (kosul) { ... };`
   - `break;` ve `continue;` kontrolleri
6. **Çift Yorum Desteği**:
   - `# Python tarzı yorum`
   - `// C tarzı tek satır yorum`
   - `/* C tarzı çok satırlı yorum */`
7. **Etkileşimli Kabuk (REPL)**: Terminalde `metescript` yazarak canlı deneme yapabilme.

---

## 🛠️ Kurulum ve Derleme

MeteScript doğrudan GCC ve Make ile tek komutla derlenebilir:

```bash
cd /home/admin/metescript
make
```

Derlenen ikili dosya `bin/metescript` altında ve `~/.local/bin/metescript` yolunda hazır haldedir.

---

## 💻 Kullanım Şekilleri

### 1. Etkileşimli REPL Kabuğu
```bash
metescript
```
```
╔═══════════════════════════════════════════════════╗
║            MeteScript v1.0.0 REPL                 ║
║  Python kolaylığı + C gücü + Zorunlu ';'         ║
║  Çıkmak için: exit() veya mete.exit();            ║
╚═══════════════════════════════════════════════════╝

mete>>> mete.println("Merhaba Dünya!");
Merhaba Dünya!
mete>>> a = 10; b = 20;
mete>>> mete.println("Toplam:", a + b);
Toplam: 30
mete>>> exit();
```

### 2. Dosyadan Betik Çalıştırma
```bash
metescript script.ms
```

### 3. Tek Satır Komut Çalıştırma (`-c`)
```bash
metescript -c 'mete.println("Hello World");'
```

---

## 📖 Sözdizimi Rehberi

### 1. Girdi ve Çıktı (I/O)
```javascript
// Ekrana yazdırma
mete.println("Hello World");

// Kullanıcıdan girdi alma (input)
a = mete.input("Cümle giriniz: ");
mete.println("Girdiğiniz cümle:", a);
```

### 2. Fonksiyon Tanımlama (`mete.deffunction`)

#### İsimli Fonksiyon:
```javascript
mete.deffunction topla(a, b) {
    return a + b;
};

sonuc = topla(15, 25);
mete.println("Sonuç:", sonuc);
```

#### Değişkene Atanan Anonim Fonksiyon:
```javascript
kare = mete.deffunction(x) {
    return x * x;
};

mete.println("5'in karesi:", kare(5));
```

#### Anonim Olarak Doğrudan Çalıştırma:
```javascript
mete.deffunction() {
    mete.println("Bu blok hemen çalışır!");
}();
```

### 3. Koşul İfadeleri (`if`, `elif`, `else`)
```javascript
puan = 85;

if (puan >= 90) {
    mete.println("Harf notu: AA");
} elif (puan >= 70) {
    mete.println("Harf notu: BB");
} else {
    mete.println("Harf notu: CC");
};
```

### 4. Döngüler

#### Python Tarzı `for..in`:
```javascript
meyveler = ["Elma", "Armut", "Muz"];
for (m in meyveler) {
    mete.println("Meyve:", m);
};

// range() ile kullanım:
for (i in mete.range(5)) {
    mete.println("İndeks:", i);
};
```

#### C Tarzı `for(init; cond; step)`:
```javascript
for (i = 0; i < 5; i = i + 1) {
    mete.println("Adım:", i);
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
    mete.println("Sayaç:", sayac);
};
```

### 5. Veri Tipleri

- **Sayılar**: `10`, `3.14`, `-42`
- **Metinler**: `"Merhaba"`, `'MeteScript'`, `\n`, `\t` kaçış dizileri, metin birleştirme `+`
- **Mantıksal**: `true`, `false`, `True`, `False`
- **Boş Değer**: `null`, `None`, `nil`
- **Dinamik Listeler**: `[1, 2, "üç", [4, 5]]`
  - Eleman ekleme: `mete.append(liste, eleman);`
  - Eleman çıkarma: `mete.pop(liste);`
  - Uzunluk: `mete.len(liste);`
  - Negatif indeksleme: `liste[-1]`
- **Sözlükler (Maps)**: `{"ad": "Mete", "yas": 25}`
  - Nokta erişimi: `kullanici.ad`
  - İndeks erişimi: `kullanici["yas"]`

---

## 📂 Örnek Dosyalar

Tüm örnekler `examples/` klasöründe yer almaktadır:
- `examples/01_merhaba.ms` - Girdi/çıktı ve değişkenler
- `examples/02_fonksiyonlar.ms` - `mete.deffunction` varyasyonları
- `examples/03_donguler.ms` - `for in`, `for(;;)` ve `while` döngüleri
- `examples/04_fibonacci.ms` - Özyinelemeli (recursive) algoritma ve zaman ölçümü
- `examples/05_listeler_ve_sozlukler.ms` - Dinamik listeler ve haritalar
- `examples/06_hesap_makinesi.ms` - Fonksiyonel hesap makinesi
