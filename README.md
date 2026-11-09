<p align="center">
  <img src="vscode-extension/icons/logo.png" width="128" height="128" alt="CicekliScript ccl logo">
</p>

<h1 align="center">CicekliScript 🚀</h1>

<p align="center">
  <strong>Python Kolaylığı + C Performansı + Zorunlu Noktalı Virgül (<code>;</code>)</strong>
</p>

**CicekliScript**, Python'ın pratikliği ve yüksek seviyeli sözdizimi ile C dilinin gücünü, blok yapısını (`{}`) ve hızını birleştiren; her cümlenin sonuna **`;` (noktalı virgül)** koymanın zorunlu olduğu modern, dinamik ve C tabanlı bir programlama dilidir!

---

## 📚 Dokümantasyon Sayfaları

- 🪟 **[Windows Kurulum Rehberi](docs/WINDOWS_KURULUM.md)**: Windows (WSL 2 / MSYS2) adım adım kurulum kılavuzu.
- 📖 **[Dil ve Sözdizimi Kılavuzu](docs/DIL_REHBERI.md)**: Dil kuralları, veri yapıları ve kullanım detayları.
- 📚 **[API & Standart Kütüphane Referansı](docs/API_REFERANSI.md)**: Tüm yerleşik fonksiyonlar, modüller (`math`, `file`, `cicekli`) ve nesne metotları.
- 🌐 **[İnteraktif Web Dokümantasyonu](docs/index.html)**: Tarayıcı üzerinden zengin arayüzlü kılavuz.

---

## 🌟 Temel Özellikler

1. **C Tabanlı**: Yorumlayıcı tamamen sıfırdan saf C (C11) ile yazılmıştır, son derece hafif ve yüksek performanslıdır.
2. **Python Kolaylığı**: Dinamik tipler, listeler `[1, 2, 3]`, sözlükler `{"anahtar": "deger"}`, dilimleme ve negatif indeksleme (`[-1]`).
3. **Zorunlu Noktalı Virgül (`;`)**: Tıpkı C'de olduğu gibi tüm ifadelerin ve blokların sonuna `;` konulması zorunludur!
4. **Zengin Çekirdek Kütüphane (`cicekli.*`)**:
   - `cicekli.input("Mesaj");` ile kullanıcıdan girdi alma
   - `cicekli.println(...)` ve `cicekli.print(...)` ile ekrana formatlı yazdırma
   - `cicekli.deffunction` veya `def` ile fonksiyon ve anonim closure tanımlama
5. **Dahili Matematik ve Dosya Modülleri**:
   - `math.*` (`sqrt`, `pow`, `sin`, `cos`, `random`, `round`, vb.)
   - `file.*` (`read`, `write`, `append`, `exists`, `delete`)
6. **Donanım Hızlandırmalı GUI Motoru**:
   - `cicekli.window(width, height, title)` ile Cairo ve X11 tabanlı çift arabellekli (double buffering) 2D grafik pencereleri ve fare/klavye olayları.
7. **Esnek Döngü Desteği**:
   - Python tarzı: `for (eleman in liste) { ... };`
   - C tarzı: `for (i = 0; i < 10; i = i + 1) { ... };`
   - While döngüsü: `while (kosul) { ... };`
   - `break;` ve `continue;` kontrolleri
8. **Çift Yorum Desteği**: `# Yorum`, `// Tek satır yorum` ve `/* Çok satırlı blok */`.
9. **Etkileşimli Kabuk (REPL)**: Canlı denemeler için zengin terminal ortamı.
10. **Resmi VS Code Eklentisi**: `.ccl` dosyaları için sözdizimi renklendirmesi, ikonlar ve tek tıkla çalıştırma.

---

## 🛠️ Kurulum ve Derleme

### 🐧 Linux (Ubuntu / Debian / Arch / Fedora)

Gerekli derleyici ve grafik kütüphanelerini kurun:

```bash
# Ubuntu / Debian:
sudo apt update && sudo apt install -y build-essential libcairo2-dev libx11-dev git

# Arch Linux:
sudo pacman -S base-devel cairo libx11 git

# Fedora:
sudo dnf install gcc make cairo-devel libX11-devel git
```

Projeyi klonlayıp derleyin:

```bash
git clone https://github.com/drkkahraman/CicekliScript.git
cd CicekliScript
make
make install
```

---

### 🪟 Windows (WSL 2 — Önerilen)

Windows 10/11 kullanıcıları, Windows Terminal veya PowerShell'i **Yönetici olarak** açıp:

```powershell
wsl --install
```

Ardından Ubuntu konsolunu açıp Linux adımlarını uygulayabilir. Windows 10/11'deki **WSLg** desteği sayesinde `cicekli.window()` ile açılan grafik pencereleri doğrudan Windows masaüstünüzde çalışır!

> Detaylı Windows rehberi için: **[docs/WINDOWS_KURULUM.md](docs/WINDOWS_KURULUM.md)**

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
cicekliscript -c 'cicekli.println("Karekök 144 =", math.sqrt(144));'
```

---

## 📖 Sözdizimi Rehberi

### 1. Giriş ve Çıkış (I/O)
```javascript
// Ekrana yazdırma
cicekli.println("Hello World");

// Kullanıcıdan girdi alma (input)
a = cicekli.input("Cümle giriniz: ");
cicekli.println("Girdiğiniz cümle:", a);
```

### 2. Fonksiyon Tanımlama (`def` / `cicekli.deffunction`)

#### İsimli Fonksiyon:
```javascript
def topla(a, b) {
    return a + b;
};

sonuc = topla(15, 25);
cicekli.println("Sonuç:", sonuc);
```

#### Değişkene Atanan Anonim Fonksiyon & Closure:
```javascript
kare = cicekli.deffunction(x) {
    return x * x;
};

cicekli.println("5'in karesi:", kare(5));
```

### 3. Koşul İfadeleri (`if`, `elif`, `else`)
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

### 4. Döngüler

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

#### C Tarzı `for (init; cond; step)`:
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

### 5. Veri Tipleri ve Metotlar

- **Listeler**: `[1, 2, "üç"]`
  - Metotlar: `.append()`, `.pop()`, `.len()`, `.sort()`, `.reverse()`, `.join()`, `.contains()`, `.slice()`
  - Negatif indeks: `liste[-1]`
- **Sözlükler (Maps)**: `{"ad": "Çiçek", "yas": 25}`
  - Metotlar: `.keys()`, `.values()`, `.has_key()`, `.len()`
  - Erişim: `kullanici.ad` veya `kullanici["ad"]`
- **Metinler (Strings)**:
  - Metotlar: `.len()`, `.split()`, `.trim()`, `.replace()`, `.contains()`, `.to_upper()`, `.to_lower()`, `.starts_with()`, `.ends_with()`, `.slice()`

### 6. Matematik ve Dosya Modülleri
```javascript
// Matematik:
kok = math.sqrt(64);             // 8
rastgele = math.random(1, 100);   // 1 ile 100 arası rastgele tamsayı

// Dosya İşlemleri:
file.write("test.txt", "CicekliScript Dosya Testi\n");
file.append("test.txt", "Yeni satır\n");
metin = file.read("test.txt");
cicekli.println("Okunan:", metin);
```

### 7. Grafiksel Arayüz (GUI) Motoru
```javascript
// 400x300 boyutunda pencere aç:
window = cicekli.window(400, 300, "Uygulama Penceresi");

// Arka planı boya:
window.color("#1e1e2e");

// Metin ekle: window.text(metin, renk, boyut, x, y);
window.text("CicekliScript GUI", "white", 16, 20, 35);

// Geometrik şekiller:
window.rect(10, 50, 40, 20, "blue");
window.circle(70, 60, 15, "green");
window.line(10, 100, 200, 100, "yellow", 2);

// Pencereyi açık tutan olay döngüsü:
window.mainloop();
```

---

## 🎨 VS Code Eklentisi

CicekliScript için özel olarak geliştirilmiş resmi bir VS Code eklentisi mevcuttur:

- **Konum**: `vscode-extension/cicekliscript-1.0.0.vsix`
- **Özellikler**:
  - `.ccl` ve `.cicekli` dosyaları için zengin sözdizimi renklendirmesi
  - Özel CicekliScript dosya ikonları
  - Fonksiyon, döngü, GUI ve temel yapılar için hızlı snippet'lar
  - Sağ üst köşede tek tıkla **Çalıştır (Run)** butonu

### Kurulum:
VS Code içerisinden `Ctrl+Shift+X` (Eklentiler) -> Sağ üstteki `...` (Üç nokta) -> **Install from VSIX...** seçeneğini tıklayıp `vscode-extension/cicekliscript-1.0.0.vsix` dosyasını seçmeniz yeterlidir.

---

## 🌐 Web Tabanlı Canlı Playground & Sunucu

İnteraktif web geliştirme ortamını ve canlı grafik önizleme tuvalini yerelinizde çalıştırmak için:

```bash
python3 server.py
```
Ardından tarayıcınızdan **[http://localhost:8080](http://localhost:8080)** adresine gidebilirsiniz.

---

## 📂 Örnek Dosyalar

Tüm örnekler `examples/` klasöründe yer almaktadır:
- `examples/01_merhaba.ccl` - Girdi/çıktı ve değişkenler
- `examples/02_fonksiyonlar.ccl` - Fonksiyon varyasyonları ve anonim fonksiyonlar
- `examples/03_donguler.ccl` - `for in`, `for(;;)` ve `while` döngüleri
- `examples/04_fibonacci.ccl` - Özyinelemeli (recursive) algoritma ve zaman ölçümü
- `examples/05_listeler_ve_sozlukler.ccl` - Dinamik listeler ve sözlükler
- `examples/06_hesap_makinesi.ccl` - Fonksiyonel hesap makinesi
- `examples/07_gui_pencere.ccl` - GUI temel pencere ve şekil çizimi
- `examples/08_gui_gelismis.ccl` - İnteraktif GUI animasyon ve çizim örneği
