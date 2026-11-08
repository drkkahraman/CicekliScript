# MeteScript - Merhaba Dünya & Giriş Örneği
// Her ifade sonuna ';' koymak zorunludur!

mete.println("=== MeteScript 1.0 Başlatıldı ===");
mete.println("Hello World!");

// Kullanıcıdan girdi alma (input)
# İsim alalım:
isim = mete.input("Lütfen adınızı giriniz: ");
mete.println("Hoş geldin,", isim, "!");

// Sayı alıp karesini hesaplayalım:
sayi_metni = mete.input("Bir sayı giriniz: ");
sayi = mete.float(sayi_metni);
kare = sayi * sayi;

mete.println(sayi, "sayısının karesi:", kare);
