# Switch derleme ve yama araçları

## NRO derleme

```sh
./tools/build_switch.sh clean all
```

## Oyun metni ve dublaj hazırlama

Yasal olarak çıkarılmış bir oyun kaynak klasörü üzerinde çalışır. Önce tüm
metinleri katalogla:

```sh
python3 tools/localization_pipeline.py extracted/romfs
```

İnternet erişimli OpenAI uyumlu bir çeviri servisiyle Türkçe üret:

```sh
python3 tools/localization_pipeline.py extracted/romfs \
  --translate \
  --endpoint http://127.0.0.1:11434 \
  --model qwen2.5:3b
```

Çıktılar:

- `build/catalog.json`: kaynak ve Türkçe eşleşmeleri.
- `build/turkish-romfs/`: çevrilmiş metin dosyaları.
- `build/voice-manifest.jsonl`: her replik için Türkçe dublaj ses dosyası yolu.

Ses üretimi, seçilen TTS servisinin ürettiği `.opus` dosyalarını manifestteki
kimliklerle eşleştirir. Bu araç ses dosyasını oyunun özel ses arşivine körlemesine
paketlemez; her oyun için ses bankası formatı ayrıca doğrulanmalıdır.

Şifreli NCA/NSP dosyalarını kırmaz veya değiştirmez. Araç, yasal ve çıkarılmış
mod kaynaklarıyla çalışır. Oyunların dosya biçimleri farklı olduğu için sonuç
doğrudan her oyunda açılmaz; ilgili oyunun arşivleme ve ses bankası adaptörü
gerekir.# Switch Derleme Araçları

Switch NRO derlemesi için devkitPro kurulumuna gerek kalmadan proje kökünde
`./tools/build_switch.sh clean all` çalıştırılabilir. Script resmi devkitPro
container'ını kullanır.

Üretilen NRO'yu SD kartta `/switch/SwitchTurkceOverlay.nro` konumuna,
UltraHand paketini de `/switch/.packages/SwitchTurkceOverlay/package.ini`
konumuna koyun. Çeviri verileri cihazda
`/switch/SwitchTurkceOverlay/dictionary.ini` dosyasında tutulur.