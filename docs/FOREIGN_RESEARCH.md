# Yabancı Proje Araştırması

Bu proje aşağıdaki açık kaynak Switch projelerinin mimarisini inceledi; kodları
kopyalamaz ve lisans yükümlülüklerini projeye taşımadan türev parça eklemez.

## Tesla Menu ve libtesla

- `WerWolv/Tesla-Menu`: Switch overlay menüsü.
- `WerWolv/libtesla`: Tesla overlay yazmak için destek kütüphanesi.
- Lisans: GPL-2.0.

Bunlar ekranda arayüz göstermek için kullanılır; oyun sesini yakalayan,
OCR yapan veya otomatik çeviri yapan bir motor değildir. Bu nedenle mevcut
libnx konsol NRO'sunu tek başına Tesla overlay diye adlandırmıyoruz. Gerçek
oyun içi görünüm için sonraki adım libtesla tabanlı NRO ve ayrı sysmodule IPC'si
olmalıdır.

## OverlayTranslate

- `uezer/OverlayTranslate`: Windows üzerinde ekran bölgesi yakalayıp OCR yapan
   ve çeviriyi aynı bölgenin üstüne çizen uygulama.
- PaddleOCR veya uzak OCR kullanır; birden fazla çeviri motoru vardır.
- Windows ekran yakalama, WPF pencere overlay'i ve .NET servislerine bağlıdır.

Bu, kullanıcının bahsettiği "bütün oyunlarda" davranışın nasıl sağlandığını
açıklayan en yakın örnektir: oyun belleğine değil ekrana bakar. Ancak aynı kod
Switch'e taşınamaz. Switch homebrew tarafında oyun görüntüsünü her oyundan
alan genel bir framebuffer/API bulunmadığı için OCR motorundan önce görüntünün
elde edilmesi problemi çözülmelidir. Tesla overlay yalnızca kendi çizdiği
arayüzü gösterir; oyunun arka plan karesini otomatik olarak OCR'a vermez.

## CaptureSight

- `zaksabeast/CaptureSight`: Pokémon oyunları için Tesla overlay.
- Lisans: GPL-3.0.

Bu proje genel oyun desteği sağlamaz; belirli oyunların belleğini ve sürüm
verilerini bilen adaptörlerle çalışır. Bizim oyun adaptörü tasarımımızın
dayanağı budur: oyun kimliği, sürüm kontrolü, güvenli okuma ve altyazı olayını
ortak çekirdeğe aktarma.

## Sonuç

Yabancı projelerde Switch üzerinde çalışan evrensel ses -> konuşma -> Türkçe
çeviri motoru bulunmadı. Ortak Switch API'si oyun sesini veya GPU görüntüsünü
her oyundan okumuyor. Bu nedenle gerçekçi Switch-only yol şöyledir:

1. Tesla/libtesla ile görünür overlay.
2. Görüntü yakalama yolu varsa Switch'e uyarlanmış OCR.
3. Görüntü yolu yoksa Atmosphere sysmodule ile oyun/sürüm adaptörlerini yükleme.
4. Bellekten metin okuyabilen oyunlarda doğrudan altyazı çıkarma.
5. Sadece adaptörün desteklediği oyunlarda cihaz içi sözlük veya hafif çeviri.
6. Ses tabanlı destek için ayrıca oyun ses servisine özel, sürüm kontrollü
   araştırma; bu genel bir garanti değildir.

Bu sınırlar değiştirilmeden "bütün oyunlar ve bütün sürümler" sözü teknik
olarak doğru olmaz.