# MeteScript - Gelişmiş GUI Uygulaması

// 450x350 boyutunda penceremizi açalım
window = mete.window(450, 350, "MeteScript GUI Uygulaması");

// Koyu tema arka plan
window.color("#1e1e2e");

// Başlık ve metinler ekleyelim
window.text("MeteScript GUI Demo", "#89b4fa", 24, 80, 30);
window.text("Python Kolaylığı + C Gücü + ;", "#cdd6f4", 14, 110, 75);

// Şekiller çizelim
// Dikdörtgen: window.rect(x, y, genislik, yukseklik, renk);
window.rect(40, 120, 160, 90, "#313244");
window.text("Kutu 1", "#a6adc8", 14, 90, 155);

window.rect(240, 120, 160, 90, "#45475a");
window.text("Kutu 2", "#a6adc8", 14, 290, 155);

// Daireler: window.circle(x, y, yaricap, renk);
window.circle(100, 270, 30, "#f38ba8");
window.circle(225, 270, 30, "#a6e3a1");
window.circle(350, 270, 30, "#f9e2af");

// Çizgiler: window.line(x1, y1, x2, y2, renk, kalinlik);
window.line(40, 320, 410, 320, "#585b70", 2);

mete.println("Pencere açıldı! Çıkmak için pencereyi kapatın.");

// Pencereyi açık tutan döngü
window.mainloop();
