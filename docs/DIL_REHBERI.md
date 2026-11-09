# CicekliScript Dil ve Sözdizimi Kılavuzu 📖

CicekliScript, Python'ın pratikliği ve yüksek seviyeli veri yapıları ile C dilinin blok yapısını (`{}`) ve performansını bir araya getiren bir programlama dilidir.

En temel kural: **Her cümlenin ve blok yapısının sonunda `;` (noktalı virgül) bulunması zorunludur!**

---

## 1. Yorum Satırları

CicekliScript hem Python hem de C tarzı yorumları destekler:

```javascript
# Bu bir Python tarzı tek satırlık yorumdur

// Bu bir C tarzı tek satırlık yorumdur

/*
   Bu ise
   C tarzı çok satırlı bir yorum bloğudur.
*/
```

---

## 2. Değişkenler ve Atamalar

Değişken tanımlamak için önceden tip belirtmeye gerek yoktur. Tipler dinamiktir.

```javascript
sayi = 42;
ondalik = 3.14;
mesaj = "Merhaba Dünya";
aktif = true;
bosluk = null;
```

Bileşik atama operatörleri desteklenir:
```javascript
x = 10;
x += 5;   # x = 15;
x -= 3;   # x = 12;
x *= 2;   # x = 24;
x /= 4;   # x = 6;
```

---

## 3. Zorunlu Noktalı Virgül (`;`) Kuralı

CicekliScript'te noktalı virgül bir seçenek değil, C dilinde olduğu gibi **zorunludur**:

- Değişken atamalarının sonunda: `a = 10;`
- Fonksiyon çağrılarının sonunda: `cicekli.println("test");`
- `if-else` bloklarının kapanış parantezinden sonra: `if (...) { ... };`
- `for` ve `while` bloklarının sonunda: `while (...) { ... };`
- Fonksiyon tanımlarının sonunda: `def topla() { ... };`

---

## 4. Koşul İfadeleri (`if`, `elif`, `else`)

```javascript
not = 82;

if (not >= 90) {
    cicekli.println("Derece: AA");
} elif (not >= 80) {
    cicekli.println("Derece: BA");
} elif (not >= 70) {
    cicekli.println("Derece: BB");
} else {
    cicekli.println("Kaldı");
};
```

Mantıksal operatörler:
- Ve: `&&` veya `and`
- Veya: `||` veya `or`
- Değil: `!` veya `not`

Karşılaştırma operatörleri:
- `==`, `!=`, `<`, `<=`, `>`, `>=`

---

## 5. Döngüler

### Python Tarzı `for..in`
Bir liste veya aralık (range) üzerinde gezinmek için kullanılır:

```javascript
meyveler = ["Elma", "Muz", "Çilek"];

for (meyve in meyveler) {
    cicekli.println("Meyve:", meyve);
};
```

### C Tarzı `for (init; cond; step)`
Sayaç temelli klasik döngü:

```javascript
for (i = 0; i < 5; i = i + 1) {
    cicekli.println("Sayı:", i);
};
```

### `while` Döngüsü
Belirli bir koşul doğru olduğu sürece çalışır:

```javascript
sayac = 0;
while (sayac < 10) {
    sayac += 1;
    if (sayac == 3) {
        continue; # 3'ü atla
    };
    if (sayac == 8) {
        break;    # 8'de döngüden çık
    };
    cicekli.println("Sayaç:", sayac);
};
```

---

## 6. Fonksiyonlar

Fonksiyonlar hem `def` anahtar kelimesiyle hem de `cicekli.deffunction` ile tanımlanabilir.

### İsimli Fonksiyon:
```javascript
def faktoriyel(n) {
    if (n <= 1) {
        return 1;
    };
    return n * faktoriyel(n - 1);
};

cicekli.println("5! =", faktoriyel(5));
```

### Değişkene Atanan Anonim Fonksiyon (First-Class Citizens):
```javascript
kareAl = cicekli.deffunction(x) {
    return x * x;
};

cicekli.println("6'nın karesi:", kareAl(6));
```

### Kapsam ve Kapanışlar (Closures):
Fonksiyonlar tanımlandıkları ortamdaki dış değişkenleri korur:

```javascript
def sayacUretici(baslangic) {
    deger = baslangic;
    return cicekli.deffunction() {
        deger += 1;
        return deger;
    };
};

sayac = sayacUretici(10);
cicekli.println(sayac()); # 11
cicekli.println(sayac()); # 12
```

---

## 7. Veri Yapıları: Listeler ve Haritalar

### Dinamik Listeler
```javascript
liste = [10, "metin", true, [1, 2]];

# Eleman ekleme ve çıkarma
liste.append(99);
son = liste.pop(); # 99

# Negatif indeksleme
cicekli.println("Son eleman:", liste[-1]);
cicekli.println("Sondan ikinci:", liste[-2]);

# Dilimleme (Slicing)
alt_liste = liste.slice(0, 2);
```

### Sözlükler (Maps)
```javascript
kisi = {
    "ad": "Deniz",
    "yas": 30,
    "sehir": "İstanbul"
};

# Nokta ile erişim
cicekli.println("Ad:", kisi.ad);

# Köşeli parantezle erişim
cicekli.println("Yaş:", kisi["yas"]);

# Yeni alan ekleme / güncelleme
kisi.meslek = "Yazılımcı";
```

---

## 8. Hata Ayıklama ve REPL İpuçları

Komut satırından tek bir kod parçasını çalıştırmak için `-c` bayrağını kullanabilirsiniz:
```bash
cicekliscript -c 'cicekli.println("10 * 20 =", 10 * 20);'
```

Canlı REPL ortamında hızlıca çıkmak için `exit()` veya `cicekli.exit();` yazabilirsiniz.
