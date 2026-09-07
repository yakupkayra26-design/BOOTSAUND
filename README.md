# Switch Turkce Overlay

Nintendo Switch üzerinde çalışan, oyun adaptörlerinden gelen altyazıları
Türkçe göstermek için geliştirilen Atmosphere/UltraHand uyumlu bir çekirdek.
Ses tanıma ve çeviri işlemi hedef cihazın içinde kalır; PC, capture card veya
harici ağ servisi çalışma zamanı bağımlılığı değildir.

## Gerçekçi kapsam

Switch oyunları ortak bir altyazı veya ekran metni API'si sunmaz. Bu yüzden
tek bir NRO/sysmodule ile bütün oyunlarda ve bütün sürümlerde otomatik yakalama
garantisi teknik olarak mümkün değildir. Her oyun motoru veya sürümü için
metnin nereden okunacağını bilen bir adaptör gerekir. Bu repo, o adaptörlerin
bağlandığı ortak çekirdeği sağlar.

İlk sürümün çalışan sınırı:

- `inbox.txt` üzerinden gelen oyun altyazısını okur. Bu dosya ilk adaptör
	sözleşmesidir; sonraki sysmodule adaptörleri aynı veriyi IPC ile sağlayabilir.
- `dictionary.ini` içindeki cihaz içi çevirileri uygular.
- Son dört satırı libnx ekranında gösterir.
- UltraHand içinden `SwitchTurkceOverlay.nro` dosyasını başlatır.

Gerçek sistem overlay entegrasyonu ve oyun belleği adaptörleri sonraki
katmanların işidir. UltraHand burada overlay motoru değil, paketi başlatıp
kurulum komutlarını çalıştıran dağıtım katmanıdır.

## Dublajdan yapay zekâ ile altyazı

Switch üzerinde Whisper veya büyük dil modeli çalıştırmak gerçek zamanlı ve
bütün oyunları kapsayan bir çözüm için fazla ağırdır. Bu nedenle cihaz içi
çeviri motoru önce sözlük/önbellek ve oyun adaptörleriyle ilerler. Hafif bir
model ileride ayrıca port edilse bile sesin oyunlardan evrensel biçimde
okunması için yine oyun veya sürüm adaptörü gerekir.

Yabancı araştırma notları `docs/FOREIGN_RESEARCH.md` dosyasındadır.
Evrensel OCR örneği olarak incelenen OverlayTranslate Windows'a özeldir;
Switch'te aynı sonucu almak için önce oyunun görüntüsünü homebrew'e taşıyan
bir erişim katmanı gerekir.

## Oyun dosyalarından Türkçe yama ve dublaj

`tools/localization_pipeline.py`, çıkarılmış metin kaynaklarını tarar, internet
bağlantısı varsa OpenAI uyumlu bir AI endpoint'ine gönderir, yer tutucuları
koruyarak Türkçe dosyalar üretir ve Türkçe dublaj için ses manifesti hazırlar.
Üretilen seslerin oyunun özel ses bankasına paketlenmesi oyun adaptörü ister;
her oyunun ses arşivi aynı formatta değildir.

## İnternetten yetkili yama indirme

Uygulamanın yama kataloğu `patches/catalog.json` içinde tutulur. Katalogdaki
paketler HTTPS, oyun kimliği, sürüm ve SHA-256 ile doğrulanmadan kurulmaz.
Katalog biçimi `docs/PATCH_CATALOG.md` dosyasında açıklanmıştır. YamanX veya
başka bir çeviri ekibiyle entegrasyon için onların izinli dağıtım bağlantısı
ve lisans bilgisi gerekir; üçüncü taraf yamaları kopyalamıyoruz.

## Derleme

devkitPro devkitA64 ortamında:

```sh
dkp-pacman -S --needed switch-dev
make
```

Çıktı `SwitchTurkceOverlay.nro` olur. Bu çalışma alanında devkitPro yoksa
yerel derleme yapılamaz; aynı komut GitHub Actions üzerinde devkitPro container
ile çalışır.

## SD kart kurulumu

```text
sdmc:/switch/SwitchTurkceOverlay.nro
sdmc:/switch/SwitchTurkceOverlay/dictionary.ini
sdmc:/switch/SwitchTurkceOverlay/inbox.txt
sdmc:/switch/SwitchTurkceOverlay/config.ini
sdmc:/switch/.packages/SwitchTurkceOverlay/package.ini
```

`config.ini` ayarları:

```ini
online_ai=1
auto_patch=0
dubbing=0
```

`Y` tuşu online AI seçeneğini açıp kapatır. Ağ üzerinden yama indirme ve AI
endpoint bağlantısı için libnx tarafında HTTPS/SSL taşıma katmanı ile yetkili
katalog sunucusu gerekir; bu sürüm ayar durumunu gösterir, sahte indirme sonucu
üretmez.

UltraHand menüsünden `Switch Turkce Overlay` paketini açın. Test için
`inbox.txt` içine şu formatta satır eklenebilir:

```text
1000|Hello
1050|Good morning
```

Çeviri sözlüğü `kaynak=hedef` satırlarından oluşur. Ayrıntılı veri sözleşmesi
`docs/ADAPTER_PROTOCOL.md` dosyasındadır.

## Arşiv

Önceki BootSound çalışması kaynakları ve derleme çıktıları
`archive/BootSound` altında korunur. Yeni proje onun kod yolunu veya
önyükleme sesini kullanmaz.# BootSound

Nintendo Switch için Atmosphere üzerinde çalışan, açılışta seçilen MP3 dosyasını
çalan BootSound projesi. GUI ve arka plan servisi aynı kaynak dosyasından iki
ayrı hedef olarak derlenir.

## Gereksinimler

- Nintendo Switch homebrew geliştirme ortamı (devkitPro, devkitA64 ve libnx)
- Atmosphere CFW
- SD kartta `/switch/BootSound.nro` dosyasını çalıştırabilen Homebrew Menu

Bu depodaki kaynak, Switch SDK'sı olmadan masaüstünde çalıştırılamaz; `switch.h`
ve `audout` libnx tarafından sağlanır.

## Derleme

devkitPro ortamı etkin bir terminalde:

```sh
make
```

Çıktılar `BootSound.nro` ve `BootSoundSysmodule.nsp` olarak üretilir. `icon.jpg`
kullanılacaksa kök dizine eklenmelidir; aksi halde NRO adımına `--icon=icon.jpg`
parametresi kaldırılmalıdır.

## SD kart kurulumu

```text
sdmc:/switch/V2.01.nro
sdmc:/atmosphere/contents/4200000000000077/exefs.nsp
sdmc:/switch/.packages/BootSound/package.ini
sdmc:/atmosphere/contents/4200000000000077/flags/boot2
sdmc:/BOOTSOUND/
```

İlk çalıştırmada GUI `/BOOTSOUND/config.ini` dosyasını oluşturur. MP3 dosyalarını
bu klasöre koyun. GUI içinde yön tuşlarıyla dosya seçilir, `A` aktif açılış
sesi yapar, `X` Türkçe/İngilizce arasında geçiş yapar ve `+` çıkar.

Örnek yapılandırma:

```ini
language=TR
active_track=ornek.mp3
```

Servis, `boot2` bayrağı sayesinde Atmosphere tarafından yüklenir; yapılandırmayı
okur, MP3'ü `dr_mp3` ile PCM'e çevirir ve `audout` üzerinden çalar. Dosya yoksa
veya `active_track=none` ise sessizce sonlanır.

## UltraHand

`ultrahand/BootSound/package.ini` dosyasını SD kartta
`/switch/.packages/BootSound/package.ini` konumuna kopyalayın. UltraHand menüsünde
`BootSound` paketi görünür; `CROX` yapımcı adıyla uygulamayı başlatabilirsiniz.

Uygulama, Switch'in libnx konsol ekranini kullanir ve SDL/TTF gibi ek runtime
bagimliliklari olmadan acilir. Amac, emuMMC/Atmosphere baslangicinda secilen MP3
dosyasini caldirmaktir.
