# BootSound

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

GUI, YamaNX projesindeki panel tabanli Switch arayuzlerinden ilham alir. ROMFS'teki
`font.ttf`, YamaNX yazari SertAy'in lisans kosullarina uygun olarak kullanilir.
