# MeteScript - Fibonacci ve Zaman Ölçümü

mete.deffunction fib(n) {
    if (n <= 1) {
        return n;
    };
    return fib(n - 1) + fib(n - 2);
};

mete.println("Fibonacci Serisi Hesaplanıyor...");

baslangic = mete.time();

for (i in mete.range(0, 16)) {
    mete.println("fib(" + mete.str(i) + ") =", fib(i));
};

bitis = mete.time();
gecen_sure = bitis - baslangic;
mete.println("Toplam geçen süre (saniye):", gecen_sure);
