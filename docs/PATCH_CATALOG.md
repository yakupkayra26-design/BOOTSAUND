# Yetkili Yama Kataloğu

Uygulama yalnızca yayıncısından izin alınmış veya açık lisansla dağıtılabilen
Türkçe yama paketlerini katalogdan indirebilir. Yama içeriği depoya gömülmez.
Katalog `patches/catalog.json` konumundadır.

Her kayıt `id`, `title_id`, `version`, `title`, HTTPS `url`, SHA-256 `sha256`
ve `license` alanlarını taşır. Örnek:

```json
{
  "id": "example-game-tr-1.0.0",
  "title_id": "0100000000000000",
  "version": "1.0.0",
  "title": "Example Game Turkce Yama",
  "url": "https://example.org/patch.zip",
  "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
  "license": "Permission granted by publisher",
  "source": "https://example.org/permission"
}
```

Uygulama indirmeden önce oyun başlığını, sürümünü ve SHA-256 değerini kontrol
eder. Hash uyuşmazsa paket kurulmaz.

YamanX gibi bir ekip ile entegrasyon için onların açık izni, dağıtım URL'si ve
lisans koşulları gerekir. İsim veya dosya kopyalamak yerine katalog yalnızca
yetkili yayına işaret eder.