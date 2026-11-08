# MeteScript - Liste ve Harita (Sözlük) Veri Yapıları

// Dinamik Liste
liste = [10, "MeteScript", true, [1, 2]];
mete.println("Orijinal liste:", liste);

mete.append(liste, 99.5);
mete.println("Eleman eklendikten sonra:", liste);

ilk_eleman = liste[0];
son_eleman = liste[-1];
mete.println("İlk eleman:", ilk_eleman);
mete.println("Son eleman (negatif indeks):", son_eleman);

// Sözlük / Nesne (Map)
kullanici = {
    "kullanici_adi": "mete_dev",
    "seviye": 99,
    "aktif": true
};

mete.println("\nKullanıcı:", kullanici);
mete.println("Kullanıcı adı (nokta ile erişim):", kullanici.kullanici_adi);
mete.println("Seviye (indeks ile erişim):", kullanici["seviye"]);

// Değer güncelleme
kullanici.seviye = 100;
mete.println("Güncellenmiş seviye:", kullanici.seviye);
