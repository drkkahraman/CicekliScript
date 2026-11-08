# MeteScript - Hesap Makinesi Fonksiyonları

mete.deffunction hesapla(a, b, islem) {
    if (islem == "+") {
        return a + b;
    } elif (islem == "-") {
        return a - b;
    } elif (islem == "*") {
        return a * b;
    } elif (islem == "/") {
        if (b == 0) {
            return "Hata: Sıfıra bölünemez!";
        };
        return a / b;
    } elif (islem == "%") {
        return a % b;
    } else {
        return "Bilinmeyen işlem!";
    };
};

mete.println("10 + 5 =", hesapla(10, 5, "+"));
mete.println("10 - 5 =", hesapla(10, 5, "-"));
mete.println("10 * 5 =", hesapla(10, 5, "*"));
mete.println("10 / 5 =", hesapla(10, 5, "/"));
mete.println("10 % 3 =", hesapla(10, 3, "%"));
mete.println("10 / 0 =", hesapla(10, 0, "/"));
