# Windows İçin CicekliScript Kurulum Rehberi 🪟

CicekliScript; C11 standartları, POSIX sistem çağrıları ve grafik arayüz (GUI) için **Cairo & X11** kütüphanelerini kullanır. Windows ortamında en yüksek performans, tam uyumluluk ve grafik pencerelerinin (GUI) doğrudan Windows masaüstünde açılabilmesi için **WSL 2 (Windows Subsystem for Linux)** kullanılması önerilir.

---

## 🌟 Yöntem 1: WSL 2 (Windows Subsystem for Linux) — Önerilen & En Kolay Yol

Windows 10 (Sürüm 2004 ve üzeri) veya Windows 11 kullanıyorsanız, WSL 2 yerleşik grafik desteği (**WSLg**) ile gelir. Bu sayede `cicekli.window()` ile açtığınız pencereler doğrudan Windows masaüstünde bağımsız bir uygulama gibi görüntülenir.

### 1. Adım: WSL'i Kurun

1. Windows Başlat menüsüne sağ tıklayın ve **Terminal (Yönetici)** veya **PowerShell (Yönetici)** seçeneğini açın.
2. Aşağıdaki komutu yazıp `Enter` tuşuna basın:

```powershell
wsl --install
```

> **Not:** Eğer sisteminizde WSL zaten yüklüyse doğrudan Ubuntu uygulamasını başlatabilirsiniz. İlk kurulumda bilgisayarınızı yeniden başlatmanız gerekebilir. Bilgisayar açıldığında Ubuntu konsolu sizden bir kullanıcı adı ve şifre oluşturmanızı isteyecektir.

---

### 2. Adım: Gerekli Paketleri Yükleyin

Ubuntu terminalini açın ve GCC derleyicisi, Make ve grafik kütüphanelerini yükleyin:

```bash
sudo apt update
sudo apt install -y build-essential libcairo2-dev libx11-dev git python3
```

---

### 3. Adım: CicekliScript'i İndirin ve Derleyin

Terminalde projeyi klonlayıp derleyin:

```bash
# Projeyi GitHub'dan indirin:
git clone https://github.com/drkkahraman/CicekliScript.git
cd CicekliScript

# Derleyin ve sisteme kurun:
make
make install
```

Kurulum bittiğinde terminali kapatıp yeniden açın ya da şu komutu çalıştırın:
```bash
source ~/.bashrc
```

---

### 4. Adım: Test Edin

Terminalde doğrudan REPL kabuğunu başlatabilirsiniz:
```bash
cicekliscript
```

Örnek bir betik dosyasını çalıştırmak için:
```bash
cicekliscript examples/01_merhaba.ccl
```

Grafik penceresi örneğini çalıştırmak için:
```bash
cicekliscript examples/07_gui_pencere.ccl
```
*(Windows masaüstünüzde yerel bir grafik penceresi açılacaktır).*

---

## 💻 VS Code ile Kusursuz Geliştirme Ortamı

Windows üzerinde kod yazarken sözdizimi renklendirmesi ve çalıştırma butonlarını kullanmak için:

1. Windows'ta **Visual Studio Code**'u açın.
2. Eklentiler (Extensions - `Ctrl + Shift + X`) sekmesinden **WSL** (Microsoft) eklentisini kurun.
3. Ubuntu terminalinizde CicekliScript klasöründeyken şu komutu yazın:
   ```bash
   code .
   ```
4. Proje içindeki özel VS Code eklentisini kurmak için:
   - VS Code sol menüsündeki **Extensions** sekmesini açın.
   - Sağ üst köşedeki **`...` (üç nokta)** simgesine tıklayın ve **"Install from VSIX..."** seçeneğini seçin.
   - Proje içindeki `vscode-extension/cicekliscript-1.0.0.vsix` dosyasını seçin.

Artık `.ccl` uzantılı dosyalarınızda tam sözdizimi renklendirmesi, dosya ikonları, snippet'lar ve sağ üstteki **Çalıştır (Play)** butonu aktif olacaktır.

---

## 🌐 Web Tabanlı Canlı Playground & IDE

CicekliScript ile birlikte gelen web arayüzünü çalıştırmak için:

```bash
python3 server.py
```
Komut çalıştıktan sonra tarayıcınızda [http://localhost:8080](http://localhost:8080) adresini açarak kod yazabilir, interaktif olarak çalıştırabilir ve grafik çizimlerini web tarayıcınızda canlı olarak görebilirsiniz.

---

## ⚙️ Yöntem 2: MSYS2 ile Kurulum (WSL İstemeyenler İçin)

Eğer sisteminize WSL kurmak istemiyorsanız:

1. [msys2.org](https://www.msys2.org/) adresinden MSYS2 kurulum dosyasını indirin ve kurun.
2. **MSYS2 UCRT64** terminalini açın ve şu komutları çalıştırın:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cairo mingw-w64-ucrt-x86_64-make pkg-config git
   ```
3. Projeyi klonlayıp derleyin:
   ```bash
   git clone https://github.com/drkkahraman/CicekliScript.git
   cd CicekliScript
   mingw32-make
   ```
> **Not:** MSYS2 altında GUI pencerelerinin görüntülenebilmesi için Windows tarafında bir X11 sunucusu (VcXsrv veya Xming gibi) çalışıyor olmalıdır. Bu nedenle WSL 2 çok daha zahmetsiz bir deneyim sunar.
