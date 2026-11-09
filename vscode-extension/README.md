# CicekliScript for Visual Studio Code

<p align="center">
  <img src="icons/ccl.png" width="128" height="128" alt="CicekliScript Logo" />
</p>

**CicekliScript** programlama dili için resmi Visual Studio Code eklentisi.

Python'ın esnekliği, C dilinin hızı ve zorunlu `;` disiplinini bir araya getiren CicekliScript için tam donanımlı geliştirme ortamı!

---

## ✨ Özellikler

- 🎨 **Gelişmiş Sözdizimi Renklendirme:** 
  - `cicekli.println`, `cicekli.print`, `cicekli.input`, `cicekli.deffunction`, `cicekli.window` (ve geriye uyumlu `mete.*`) tam desteği
  - Kontrol akışı: `if`, `elif`, `else`, `while`, `for in`, `return`, `break`, `continue`
  - Yerleşik matematik ve dosya fonksiyonları
  - Zorunlu noktalı virgül `;` vurgulaması
- 🏷️ **Özel Dosya Logosu:** `.ccl`, `.cicekli` ve `.ms` uzantılı dosyalarınız dosya gezgininde özel modern logoyla gösterilir.
- ▶️ **Tek Tıkla Çalıştırma:**
  - Editör başlığındaki **"▶"** (Play) butonu
  - `Ctrl + F5` kısayolu
  - Sağ tık menüsü: **"CicekliScript: Dosyayı Çalıştır"**
  - Dosya gezgininde sağ tık ile çalıştırma
- ⚡ **Zengin Kod Snippet'ları:**
  - `println` → `cicekli.println("Hello World");`
  - `input` → `a = cicekli.input("Metin: ");`
  - `deffunction` → `cicekli.deffunction fonk() { ... };`
  - `window` → Eksiksiz GUI pencere şablonu
- 📑 **Otomatik Girinti & Parantez Eşleme:** `{}` blokları ve string otomatik tamamlama.

---

## 🚀 Kullanım

1. Herhangi bir `.ccl` veya `.cicekli` uzantılı dosya açın (örneğin `ornek.ccl`).
2. Sağ üstteki **"▶"** butonuna basın veya `Ctrl+F5` tuşlayın.
3. Kodunuz VS Code Entegre Terminalinde doğrudan `cicekliscript` ile çalışır!
