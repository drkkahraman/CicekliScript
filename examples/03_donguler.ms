# MeteScript - Döngüler & Kontrol Akışı

mete.println("--- 1. Python Tarzı for..in Döngüsü ---");
meyveler = ["Elma", "Armut", "Muz", "Çilek"];
for (meyve in meyveler) {
    mete.println("Meyve:", meyve);
};

mete.println("\n--- 2. range() Fonksiyonu ile for Döngüsü ---");
for (i in mete.range(1, 6)) {
    mete.print(i, " ");
};
mete.println();

mete.println("\n--- 3. C Tarzı for(init; cond; step) Döngüsü ---");
toplam = 0;
for (j = 1; j <= 5; j = j + 1) {
    toplam += j;
};
mete.println("1'den 5'e toplam:", toplam);

mete.println("\n--- 4. While Döngüsü + break & continue ---");
sayac = 0;
while (sayac < 10) {
    sayac += 1;
    if (sayac == 3) {
        continue; // 3'ü atla
    };
    if (sayac == 7) {
        break;    // 7'de döngüyü kır
    };
    mete.println("Sayac:", sayac);
};
