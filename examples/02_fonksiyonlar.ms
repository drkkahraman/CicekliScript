# MeteScript - Fonksiyon Tanımlama Örnekleri

// 1. İsimli Fonksiyon (mete.deffunction)
mete.deffunction topla(a, b) {
    return a + b;
};

mete.println("3 + 5 =", topla(3, 5));

// 2. Değişkene Atanan Anonim Fonksiyon
carp = mete.deffunction(x, y) {
    return x * y;
};

mete.println("4 * 6 =", carp(4, 6));

// 3. Parametresiz ve Hemen Çalıştırılan Fonksiyon
mete.deffunction() {
    mete.println("Hemen çalışan anonim fonksiyon bloğu!");
}();

// 4. Varsayılan def sözdizimi de desteklenir:
def selamla(ad) {
    mete.println("Selamlar sana,", ad);
};

selamla("Mete");
