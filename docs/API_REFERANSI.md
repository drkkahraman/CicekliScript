# CicekliScript API & Standart Kütüphane Referansı 📚

CicekliScript, zengin bir yerleşik standart kütüphaneye ve nesne metotlarına sahiptir. Fonksiyonlar hem doğrudan genel kapsamda (global), hem de `cicekli.*` (veya geriye dönük uyumluluk için `mete.*`) isim alanı altında çağrılabilir.

---

## 📋 İçindekiler
1. [Giriş / Çıkış (I/O) ve Temel Fonksiyonlar](#1-giriş--çıkış-io-ve-temel-fonksiyonlar)
2. [Tip Dönüşümleri ve Kontroller](#2-tip-dönüşümleri-ve-kontroller)
3. [Zaman ve Sistem Fonksiyonları](#3-zaman-ve-sistem-fonksiyonları)
4. [Matematik Modülü (`math.*`)](#4-matematik-modülü-math)
5. [Dosya İşlemleri Modülü (`file.*`)](#5-dosya-işlemleri-modülü-file)
6. [Metin (String) Metotları](#6-metin-string-metotları)
7. [Liste (Array / List) Metotları](#7-liste-array--list-metotları)
8. [Sözlük (Map / Dictionary) Metotları](#8-sözlük-map--dictionary-metotları)
9. [Grafiksel Arayüz Modülü (`cicekli.window`)](#9-grafiksel-arayüz-modülü-cicekliwindow)

---

## 1. Giriş / Çıkış (I/O) ve Temel Fonksiyonlar

### `cicekli.println(...)` / `println(...)`
Verilen argümanları aralarında boşluk bırakarak ekrana yazdırır ve satır sonu (`\n`) ekler.
```javascript
cicekli.println("Merhaba", 2026, [1, 2]);
```

### `cicekli.print(...)` / `print(...)`
Verilen argümanları ekrana yazdırır ancak satır sonu eklemez.
```javascript
cicekli.print("Yükleniyor... ");
cicekli.println("Tamamlandı!");
```

### `cicekli.input(prompt)` / `input(prompt)`
Kullanıcıdan konsoldan bir satır metin girdisi alır. İsteğe bağlı olarak bir istem mesajı alabilir.
```javascript
ad = cicekli.input("Adınız: ");
cicekli.println("Hoş geldin,", ad);
```

### `cicekli.len(obj)` / `len(obj)` / `obj.len()`
Bir metnin karakter sayısını, listenin eleman sayısını veya sözlüğün anahtar sayısını döndürür.
```javascript
boyut = cicekli.len([10, 20, 30]); // 3
uzunluk = "Cicekli".len();         // 7
```

### `cicekli.range(stop)` veya `cicekli.range(start, stop, [step])`
Belirtilen aralıkta tamsayılar içeren bir liste üretir.
```javascript
for (i in cicekli.range(5)) {
    cicekli.println(i); // 0, 1, 2, 3, 4
};

ciftler = cicekli.range(0, 10, 2); // [0, 2, 4, 6, 8]
```

### `args` / `cicekli.args`
Komut satırından betiğe aktarılan bağımsız değişkenlerin listesidir.

---

## 2. Tip Dönüşümleri ve Kontroller

### `cicekli.type(val)` / `type(val)`
Değerin tipini metin olarak döndürür: `"null"`, `"bool"`, `"number"`, `"string"`, `"list"`, `"map"`, `"function"`, `"native"`, `"window"`.
```javascript
cicekli.println(type(42)); // "number"
cicekli.println(type([])); // "list"
```

### `cicekli.str(val)` / `str(val)`
Herhangi bir değeri metne (`string`) dönüştürür.
```javascript
s = str(123.45); // "123.45"
```

### `cicekli.int(val)` / `int(val)`
Metin veya kayan noktalı sayıyı tamsayıya (`integer`) dönüştürür.
```javascript
n = int("456"); // 456
```

### `cicekli.float(val)` / `float(val)`
Metin veya tamsayıyı kayan noktalı sayıya (`float`) dönüştürür.
```javascript
f = float("3.1415"); // 3.1415
```

---

## 3. Zaman ve Sistem Fonksiyonları

### `cicekli.time()` / `time()`
Unix başlangıcından (Epoch) bu yana geçen süreyi saniye cinsinden kayan noktalı sayı olarak döndürür. Süre ölçümleri için idealdir.
```javascript
baslangic = time();
// işlemler...
bitis = time();
cicekli.println("Geçen süre:", bitis - baslangic, "sn");
```

### `cicekli.sleep(saniye)` / `sleep(saniye)`
Programın çalışmasını belirtilen saniye kadar duraklatır (ondalıklı değerler desteklenir: `0.5`, `0.016`).
```javascript
sleep(1.5); // 1.5 saniye bekle
```

### `cicekli.exit([kod])` / `exit([kod])`
Programı belirtilen çıkış koduyla (varsayılan: 0) sonlandırır.
```javascript
exit(0);
```

---

## 4. Matematik Modülü (`math.*`)

`math` modülü altındaki tüm fonksiyonlar hem `math.<fn>` hem de `cicekli.<fn>` şeklinde çağrılabilir:

| Fonksiyon | Açıklama | Örnek |
|---|---|---|
| `math.sqrt(x)` | x'in karekökünü alır | `math.sqrt(16)` -> `4` |
| `math.pow(taban, us)` | Üs alma işlemi ($taban^{us}$) | `math.pow(2, 8)` -> `256` |
| `math.abs(x)` | Mutlak değer | `math.abs(-42)` -> `42` |
| `math.round(x)` | En yakın tamsayıya yuvarlar | `math.round(3.6)` -> `4` |
| `math.floor(x)` | Aşağı yuvarlar | `math.floor(3.9)` -> `3` |
| `math.ceil(x)` | Yukarı yuvarlar | `math.ceil(3.1)` -> `4` |
| `math.sin(r)` | Radyan cinsinden sinüs | `math.sin(3.14159 / 2)` -> `1` |
| `math.cos(r)` | Radyan cinsinden kosinüs | `math.cos(0)` -> `1` |
| `math.tan(r)` | Radyan cinsinden tanjant | `math.tan(0)` -> `0` |
| `math.log(x)` | Doğal logaritma (ln) | `math.log(2.71828)` -> `1` |
| `math.min(...)` | Verilen argümanların en küçüğü | `math.min(10, 5, 20)` -> `5` |
| `math.max(...)` | Verilen argümanların en büyüğü | `math.max(10, 5, 20)` -> `20` |
| `math.random([min, max])` | Rastgele sayı üretir | `math.random()` (0.0-1.0) veya `math.random(1, 100)` |

---

## 5. Dosya İşlemleri Modülü (`file.*`)

Dosya I/O fonksiyonları hem `file.<fn>` hem de `cicekli.<fn>` ile kullanılabilir:

```javascript
// 1. Dosya Yazma (Yeni dosya oluşturur veya üzerine yazar)
file.write("test.txt", "Merhaba CicekliScript!\n");

// 2. Dosyaya Ekleme Yapma (Append)
file.append("test.txt", "İkinci satır eklendi.\n");

// 3. Dosyanın Varlığını Kontrol Etme
if (file.exists("test.txt")) {
    cicekli.println("Dosya mevcut!");
};

// 4. Dosya Okuma (İçeriği string olarak döner)
icerik = file.read("test.txt");
cicekli.println("Okunan içerik:\n" + icerik);

// 5. Dosya Silme
file.delete("test.txt");
```

---

## 6. Metin (String) Metotları

Metin nesneleri üzerinde doğrudan çağrılabilecek zengin metotlar:

- `str.len()`: Metnin uzunluğu.
- `str.split([ayrac])`: Metni belirtilen ayraçtan (varsayılan: boşluk) parçalayarak liste döndürür.
  ```javascript
  kelimeler = "elma,armut,muz".split(","); // ["elma", "armut", "muz"]
  ```
- `str.trim()`: Metnin başındaki ve sonundaki boşlukları temizler.
- `str.replace(eski, yeni)`: Metindeki tüm `eski` alt metinleri `yeni` ile değiştirir.
- `str.contains(alt_metin)`: Metin alt metni içeriyorsa `true` döner.
- `str.to_upper()` / `str.upper()`: Metni büyük harfe dönüştürür.
- `str.to_lower()` / `str.lower()`: Metni küçük harfe dönüştürür.
- `str.starts_with(oneki)`: Metin verilen ön ekle başlıyorsa `true` döner.
- `str.ends_with(soneki)`: Metin verilen son ekle bitiyorsa `true` döner.
- `str.slice(baslangic, [bitis])`: Metinden dilim alır (negatif indeksler desteklenir).

---

## 7. Liste (Array / List) Metotları

Dinamik listeler heterojen elemanlar barındırabilir ve Python tarzı negatif indekslemeyi destekler:

- `liste.append(eleman)`: Listenin sonuna yeni bir eleman ekler.
- `liste.pop([indeks])`: Son elemanı (veya verilen indeksteki elemanı) listeden çıkarır ve değerini döner.
- `liste.len()`: Listenin eleman sayısı.
- `liste.sort()`: Sayısal veya alfabetik olarak listeyi yerinde sıralar.
- `liste.reverse()`: Listeyi tersine çevirir.
- `liste.join([ayrac])`: Liste elemanlarını ayraç ile birleştirip tek bir metin yapar.
  ```javascript
  cumle = ["Python", "C", "CicekliScript"].join(" + "); // "Python + C + CicekliScript"
  ```
- `liste.contains(eleman)`: Eleman listede varsa `true` döner.
- `liste.slice(baslangic, [bitis])`: Listeden alt liste dilimi alır.
- **Negatif İndeksleme**: `liste[-1]` son elemanı, `liste[-2]` sondan bir öncekini verir.

---

## 8. Sözlük (Map / Dictionary) Metotları

Anahtar-değer çiftlerini saklar. Değerlere hem nokta (`sozluk.anahtar`) hem de köşeli parantez (`sozluk["anahtar"]`) ile erişilebilir:

- `sozluk.keys()`: Tüm anahtarların listesini döndürür.
- `sozluk.values()`: Tüm değerlerin listesini döndürür.
- `sozluk.has_key(anahtar)`: Verilen anahtar sözlükte tanımlıysa `true` döner.
- `sozluk.len()`: Sözlükteki toplam anahtar sayısı.

```javascript
kullanici = {
    "isim": "Mete",
    "puan": 95,
    "aktif": true
};

cicekli.println("Anahtarlar:", kullanici.keys());
cicekli.println("Puan:", kullanici.puan);
```

---

## 9. Grafiksel Arayüz Modülü (`cicekli.window`)

CicekliScript, doğrudan çift arabelleğe (double buffering) ve Cairo vektör çizim motoruna sahip pencereler sunar.

### Pencere Başlatma
```javascript
win = cicekli.window(genislik, yukseklik, [baslik]);
```

### Çizim ve Kontrol Metotları

- `win.color([renk])` veya `win.clear([renk])`:
  Arka planı belirtilen renge boyar.
  Desteklenen renk formatları:
  - İsimler: `"red"`, `"blue"`, `"green"`, `"black"`, `"white"`, `"yellow"`, `"purple"`, `"cyan"`, `"orange"`, `"gray"`
  - Hex kodu: `"#1e1e2e"`, `"#ffaa00"`
  - RGB sayıları: `win.color(0.2, 0.4, 0.8)` (0.0 ile 1.0 arası)
- `win.rect(x, y, genislik, yukseklik, [renk])`:
  Dolu dikdörtgen çizer.
- `win.circle(merkez_x, merkez_y, yaricap, [renk])`:
  Dolu daire çizer.
- `win.line(x1, y1, x2, y2, [renk], [cizgi_kalinligi])`:
  İki nokta arasına çizgi çizer.
- `win.text(metin, [renk], [boyut], [x], [y])`:
  Belirtilen konuma yazı yazar.
- `win.update()` veya `win.show()`:
  Çizilen kareyi ekrana yansıtır (double buffer swap).
- `win.sleep(saniye)`:
  Pencere olaylarını dinlerken belirtilen saniye kadar bekler.
- `win.is_open()`:
  Pencere açık mı kontrol eder (`true`/`false`).
- `win.close()`:
  Pencereyi kapatır.
- `win.mainloop()` veya `win.run()`:
  Pencereyi kullanıcı kapatana kadar açık tutan engelsiz olay döngüsünü başlatır.

### Pencere Özellikleri ve Fare Kontrolü

- `win.width`: Pencere genişliği.
- `win.height`: Pencere yüksekliği.
- `win.mouse_x`: Fare imlecinin anlık X koordinatı.
- `win.mouse_y`: Fare imlecinin anlık Y koordinatı.
- `win.is_mouse_down()`: Fare sol tuşuna basılı tutuluyor mu (`true`/`false`).
