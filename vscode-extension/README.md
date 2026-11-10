# CicekliScript for Visual Studio Code

<p align="center">
  <img src="icons/ccl.png" width="128" height="128" alt="CicekliScript Logo" />
</p>

**CicekliScript** programlama dili için resmi Visual Studio Code eklentisi.

Python'ın esnekliği, C dilinin saf C11 hızı ve zorunlu `;` disiplinini bir araya getiren CicekliScript için tam donanımlı geliştirme ortamı!

---

## 🌟 Önemli Yenilik: Dahili Yerleşik Derleyici (Zero-Setup!)

**Artık bilgisayarınıza harici olarak GCC, Make veya CicekliScript kurmanıza gerek yok!**

Eklenti, **CicekliScript derleyicisini kendi içinde hazır ve derlenmiş olarak taşır.** Eklentiyi VS Code'a yüklediğiniz anda:
1. `.ccl` dosyanızı açıp sağ üstteki **"▶"** (Çalıştır) butonuna bastığınızda kodunuz eklenti içindeki yerleşik derleyici ile **anında çalışır**.
2. İsterseniz sağ tıklayıp **"CicekliScript: Dili Sisteme Kur (~/.local/bin/ccl)"** seçeneğini kullanarak eklenti içindeki derleyiciyi tek tıkla doğrudan işletim sisteminize kurabilir ve harici terminallerden de `ccl dosya.ccl` olarak kullanabilirsiniz!

---

## ✨ Özellikler

- 📦 **Dahili C11 Derleyicisi:** Kurulum gerektirmeden doğrudan eklenti içinden çalıştırma.
- 🎨 **Gelişmiş Sözdizimi Renklendirme:** 
  - `cicekli.println`, `cicekli.print`, `cicekli.input`, `cicekli.deffunction`, `cicekli.window`, `cicekli.cuda`, `cicekli.tensor` tam desteği
  - Kontrol akışı: `if`, `elif`, `else`, `while`, `for in`, `return`, `break`, `continue`, `def`
  - Yerleşik matematik (`math.*`), dosya (`doc.*`) fonksiyonları
  - Zorunlu noktalı virgül `;` vurgulaması
- 🏷️ **Özel Dosya Logosu:** `.ccl`, `.cicekli` ve `.ms` uzantılı dosyalarınız dosya gezgininde özel modern logoyla gösterilir.
- ▶️ **Tek Tıkla Çalıştırma:**
  - Editör başlığındaki **"▶"** (Play) butonu
  - `Ctrl + F5` kısayolu
  - Sağ tık menüsü: **"CicekliScript: Dosyayı Çalıştır"**
  - Seçili kodu çalıştırma (`cicekliscript.runSelection`)
- 💻 **Entegre REPL:**
  - Editör başlığındaki terminal simgesi ile tek tıkla etkileşimli kabuğu başlatma.
- ⚡ **Zengin Kod Snippet'ları:**
  - `println` → `cicekli.println("Hello World");`
  - `input` → `a = cicekli.input("Metin: ");`
  - `deffunction` → `cicekli.deffunction fonk() { ... };`
  - `window` → Eksiksiz GUI pencere şablonu
  - `tensor` → CUDA tensör ve matris çarpımı şablonu
- 📑 **Otomatik Girinti & Parantez Eşleme:** `{}` blokları ve string otomatik tamamlama.

---

## 🚀 Kurulum & Kullanım

1. VS Code'da **Extensions (Ctrl + Shift + X)** sekmesini açın.
2. Sağ üstteki `...` (üç nokta) simgesine tıklayıp **"Install from VSIX..."** seçin.
3. `cicekliscript-1.1.0.vsix` dosyasını seçin.
4. Herhangi bir `.ccl` dosyası açıp sağ üstteki **"▶"** butonuna basın!
