# Adaptör Protokolü

Oyun adaptörü altyazıyı şu dosyaya ekler:

```text
sdmc:/switch/SwitchTurkceOverlay/inbox.txt
```

Her satır UTF-8 metindir ve önerilen biçim şöyledir:

```text
<oyun zaman damgası>|<kaynak altyazı>
```

Zaman damgası yalnızca teşhis içindir; çekirdek `|` karakterinden sonraki
metni çeviri anahtarı olarak kullanır. Ayraç yoksa satırın tamamı kaynak metin
kabul edilir. Çekirdek dosyayı her yenilemede yeniden okur ve son dört satırı
gösterir.

Çeviri önbelleği:

```text
sdmc:/switch/SwitchTurkceOverlay/dictionary.ini
```

Format:

```ini
Hello=Merhaba
Good morning=Gunaydin
```

## Adaptör sorumlulukları

- Oyunun kendi metin kaynağını bulmak. OCR, Switch üzerinde ortak ekran yakalama
  API'si olmadığı için yalnızca desteklenen bir görüntü yolu varsa kullanılabilir.
- Oyun ve sürüm kimliğini doğrulamak.
- Satırları dosyaya yazarken kısmi satır bırakmamak.
- Oyundan çıkınca eski satırları temizlemek.
- Dış ağ servisi veya PC gerektirmeden cihaz içindeki çeviri önbelleğini kullanmak.
- Oyunun çevrimiçi özelliklerini veya bütünlük kontrollerini ihlal etmeyecek
  şekilde yalnızca izin verilen homebrew/test ortamında çalışmak.

## Neden evrensel değildir?

Bir oyun metni bellekte UTF-8 tutabilir, başka bir oyun özel sıkıştırma kullanır,
bir diğeri yalnızca GPU'ya çizilmiş görüntü bırakır. Bu üç durumda aynı okuma
kodu çalışmaz. Sürüm güncellemeleri adresleri ve veri yapılarını da değiştirebilir.
Bu nedenle çekirdek sürüm bağımsız bir arayüz sunar; oyun desteği adaptör
paketleriyle ayrı ayrı doğrulanır.