# Proje : İletişim

İletişim projesi, Tox ile P2P sistemini temel alan bir mesajlaşma platformudur. İletişim, merkeziyetsizliği esas alan ve arada herhangi sunucu olmadan (DHT ve TCP aktarıcı ara sunucularu haric) mesajlaşmayı sağlayan bir CLI yani programıdır.

### Öne Çıkan Özellikleri
- **Merkeziyetsiz**: Yönlendiriciler hariç tamamiyle P2P'dir. Yönlendiriciler ise sadece mesaj iletimini sağlar. Herhangi bir veri saklama veya okuma sağlayamazlar.

- **Mahremiyet ve Güvenlik(Şifreme)**: Tox'un gücünden gelen bu özellik, tüm mesajlarınızı Curve25519 ve xsalsa20 teknikleri kullanarak üst düzey şifreleme sağlar. Ayrıca mesajlarınız hem İletişim hem de ara sunucular asla mesajınızı göremezler. Bu da mahremiyeti üst düzeye taşır.

- **CLI Arayüzü**: Basit, anlaşılır ve modern CLI arayüzü vardır. İlerki sürümlerinde GUI arayüzü ile devam edilecektir.

### Kullanımı

&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Programı kullanmak için önce `toxmessenger.exe` dosyasını çalıştırın. Ardından açılan komut satırı ekranında ilk önce kayıt dosyanız var ise yükler, yok ise yeniden kayıt yapar. Sonrasında Bootstrap'lara bağlanır. Şuanlık 5 adet UDP ve TCP ara sunucuları vardır. Sunucular:
- 85.143.221.42:33445
- tox.initramfs.io:33445
- 144.217.167.73:33445
- tox.abilinski.com:33445
- tox.novg.net:33445

Ufak bir özet geçer ve kalıcı Tox Id'nizi size bildirir.

&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Son olarak komutları bildirir ve program kontrolu size bırakır. Komut kullanmak için ise, komut numarısını yazıp kullanabilirsiniz.

### Komutlar ve İşlevleri

#### 1 (Tox ID göster)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Tox Id'yı gösterir.

#### 2 (Arkadaş ekle)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;İlk önce arkadaş ekliceğiniz kullanıcının kalıcı Id'sini girin. Ardından arkadaş eklerken ne mesajı göndereceğinizi de yazmanız gerekir. Son olarak girilen Id'ye sahip kullanıcı çevrimiçi ise gönderil, çevrimdışı ise istek gönderilemez.

#### 3 (Mesaj gönderme)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Arkadaş eklenmiş olan Id'nin arkadaş numarını ister. Girilen arkadaş numarısına denk gelen Id'ye mesaj göndereceğiniz metni giriniz. En sonunda ise Id çevrimiçi ise gönderilir (İnternet bağlantısı, hızı etkiliyebilir.) Çevrimdışı ise mesaj gönderilemez.

#### 4 (Arkadaş listesi)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;İstek gönderilmiş Id'lerin listesi

#### 5 (İsim değiştir)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Girilen ismi, kullanıcı ismi olarak alır.

#### 6 (Profili manuel kaydet)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Profili manuel olarak kayıt eder.

#### 7 (Profil sil)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Mevcut profili siler ve programı kapatır. Program yeniden başlatıldığında yeni profil oluşur.

#### 8 (Çıkış)
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Programı kapatır.

### Derleme
&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Derlemeden önce, MSYS2'yi indirmemiz gerekir. Bunun için, [MSYS2](https://www.msys2.org) sitesine gidip, uygun olan sürümü indirlerim. Ve kuralım. Ardından mavi iconlu olan MSYS2 Mingw64 olan uygulamayı çalıştıralım. Gelen konsol ekranına;
```shell
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-make \
          mingw-w64-x86_64-pkg-config \
          mingw-w64-x86_64-libsodium \
          mingw-w64-x86_64-opus \
          mingw-w64-x86_64-libvpx \
          git \
          make
```
Paketlerini indirelim. Ardından C-toxcore'u derliyelim;
```shell
cd ~
git clone https://github.com/TokTok/c-toxcore.git
cd c-toxcore

# Submodule ayarlama
git submodule update --init --recursive

# Build klasörü
cd build

# CMake configure
cmake .. -G "MinGW Makefiles" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/mingw64

# Derle (4 çekirdek ile)
mingw32-make -j4

# Yükle
mingw32-make install
```

&ThinSpace;&ThinSpace;&ThinSpace;&ThinSpace;Sistemi derlemek için;
```shell
# Build klasörüne gitme
cd build

# Derleme dosyalarını ayarlama
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 4 çekirdek ile derleme
mingw32-make -j4
```
Son olarak proje klasöründe `libs` adlı klasörde, programın kullandığı .dll dosyaları bulunmaktadır. Kütüphanleri `toxmessenger.exe` dosyasının bulunduğu klasöre koyulması gerekir.

Ve artık program, derlenmiş oldu.

#### Ekran Görünüleri

![resim1](image1.png)
![resim2](image2.png)
![resim3](image3.png)
![resim4](image4.png)