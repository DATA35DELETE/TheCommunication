#include <tox/tox.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Tox data dosyası
#define TOX_SAVE_FILE "tox_save.dat"

// Global değişkenler
static volatile int running = 1;
static Tox *global_tox = NULL;

// Bootstrap nodes (qTox'tan)
typedef struct {
    const char *ip;
    uint16_t port;
    const char *key;
} BootstrapNode;

BootstrapNode bootstrap_nodes[] = {
    {"85.143.221.42", 33445, "DA4E4ED4B697F2E9B000EEFE3A34B554ACD3F45F5C96EAEA2516DD7FF9AF7B43"},
    {"tox.initramfs.io", 33445, "3F0A45A268367C1BEA652F258C85F4A66DA76BCAA667A49E770BCC4917AB6A25"},
    {"144.217.167.73", 33445, "7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C"},
    {"tox.abilinski.com", 33445, "10C00EB250C3233E343E2AEBA07115A5C28920E9C8D29492F6D00B29049EDC7E"},
    {"tox.novg.net", 33445, "D527E5847F8330D628DAB1814F0A422F6DC9D0A300E6C357634EE2DA88C35463"}
};

// ANSI renk kodları
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// Tox state'i kaydet
int save_tox_state(Tox *tox) {
    size_t size = tox_get_savedata_size(tox);
    uint8_t *savedata = malloc(size);
    
    if (!savedata) {
        printf("%s[HATA]%s Bellek tahsis edilemedi!\n", RED, RESET);
        return 0;
    }
    
    tox_get_savedata(tox, savedata);
    
    FILE *f = fopen(TOX_SAVE_FILE, "wb");
    if (!f) {
        printf("%s[HATA]%s Dosya açılamadı: %s\n", RED, RESET, TOX_SAVE_FILE);
        free(savedata);
        return 0;
    }
    
    fwrite(savedata, size, 1, f);
    fclose(f);
    free(savedata);
    
    printf("%s[✓]%s Tox profili kaydedildi (%zu bytes)\n", GREEN, RESET, size);
    return 1;
}

// Tox state'i yükle
uint8_t* load_tox_state(size_t *size) {
    FILE *f = fopen(TOX_SAVE_FILE, "rb");
    if (!f) {
        printf("%s[BİLGİ]%s Kayıtlı profil bulunamadı, yeni profil oluşturuluyor...\n", CYAN, RESET);
        *size = 0;
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *data = malloc(*size);
    if (!data) {
        fclose(f);
        *size = 0;
        return NULL;
    }
    
    fread(data, *size, 1, f);
    fclose(f);
    
    printf("%s[✓]%s Tox profili yüklendi (%zu bytes)\n", GREEN, RESET, *size);
    return data;
}

// Callback: Arkadaş isteği
void friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length, void *user_data) {
    printf("\n%s[ARKADAŞ İSTEĞİ]%s Yeni istek geldi!\n", BOLD GREEN, RESET);
    printf("%sMesaj:%s %.*s\n", CYAN, RESET, (int)length, message);
    
    TOX_ERR_FRIEND_ADD err;
    uint32_t friend_num = tox_friend_add_norequest(tox, public_key, &err);
    
    if (err == TOX_ERR_FRIEND_ADD_OK) {
        printf("%s[✓]%s Arkadaş otomatik eklendi! Friend #%u\n", GREEN, RESET, friend_num);
        // Profili kaydet (arkadaş listesi değişti)
        save_tox_state(tox);
    } else {
        printf("%s[✗]%s Arkadaş eklenemedi: %d\n", RED, RESET, err);
    }
}

// Callback: Mesaj alma
void friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *user_data) {
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
    
    // Arkadaş ismini al
    size_t name_size = tox_friend_get_name_size(tox, friend_number, NULL);
    char *name = malloc(name_size + 1);
    if (name && name_size > 0) {
        tox_friend_get_name(tox, friend_number, (uint8_t *)name, NULL);
        name[name_size] = '\0';
        printf("\n%s[%s]%s %s%s:%s %.*s\n", 
               BLUE, time_str, RESET,
               MAGENTA, name, RESET,
               (int)length, message);
        free(name);
    } else {
        printf("\n%s[%s]%s %sArkadaş #%u:%s %.*s\n", 
               BLUE, time_str, RESET,
               MAGENTA, friend_number, RESET,
               (int)length, message);
        if (name) free(name);
    }
}

// Callback: Bağlantı durumu (arkadaş)
void friend_connection_status_cb(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status, void *user_data) {
    if (connection_status != TOX_CONNECTION_NONE) {
        const char *conn_type = (connection_status == TOX_CONNECTION_TCP) ? "TCP Relay" : "UDP Direct";
        printf("\n%s[BAĞLANTI]%s Arkadaş #%u çevrimiçi (%s%s%s)\n", 
               GREEN, RESET, friend_number, YELLOW, conn_type, RESET);
    } else {
        printf("\n%s[BAĞLANTI]%s Arkadaş #%u çevrimdışı\n", RED, RESET, friend_number);
    }
}

// Callback: DHT bağlantı durumu
void self_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data) {
    if (connection_status != TOX_CONNECTION_NONE) {
        printf("\n%s[DHT]%s Tox ağına bağlandı! ✓\n", GREEN, RESET);
    } else {
        printf("\n%s[DHT]%s Tox ağından ayrıldı\n", YELLOW, RESET);
    }
}

// Callback: İsim değişikliği
void friend_name_cb(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data) {
    printf("%s[BİLGİ]%s Arkadaş #%u ismini değiştirdi: %.*s\n", 
           CYAN, RESET, friend_number, (int)length, name);
}

// Hex <-> Binary dönüştürme
void hex_to_bin(const char *hex, uint8_t *bin) {
    for (size_t i = 0; i < strlen(hex) / 2; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bin[i]);
    }
}

void bin_to_hex(const uint8_t *bin, size_t length, char *hex) {
    for (size_t i = 0; i < length; i++) {
        sprintf(hex + 2 * i, "%02X", bin[i]);
    }
    hex[length * 2] = '\0';
}

// Bootstrap yap
void bootstrap_to_network(Tox *tox) {
    printf("\n%s[BOOTSTRAP]%s DHT ağına bağlanılıyor...\n", YELLOW, RESET);
    
    int udp_ok = 0, tcp_ok = 0;
    
    for (size_t i = 0; i < sizeof(bootstrap_nodes) / sizeof(BootstrapNode); i++) {
        uint8_t key_bin[TOX_PUBLIC_KEY_SIZE];
        hex_to_bin(bootstrap_nodes[i].key, key_bin);
        
        // UDP bootstrap
        TOX_ERR_BOOTSTRAP err_udp;
        tox_bootstrap(tox, bootstrap_nodes[i].ip, bootstrap_nodes[i].port, key_bin, &err_udp);
        if (err_udp == TOX_ERR_BOOTSTRAP_OK) {
            udp_ok++;
            printf("  %s[UDP]%s %s:%d ✓\n", CYAN, RESET, bootstrap_nodes[i].ip, bootstrap_nodes[i].port);
        }
        
        // TCP relay
        TOX_ERR_BOOTSTRAP err_tcp;
        tox_add_tcp_relay(tox, bootstrap_nodes[i].ip, bootstrap_nodes[i].port, key_bin, &err_tcp);
        if (err_tcp == TOX_ERR_BOOTSTRAP_OK) {
            tcp_ok++;
            printf("  %s[TCP]%s %s:%d ✓ (CGNAT bypass)\n", GREEN, RESET, bootstrap_nodes[i].ip, bootstrap_nodes[i].port);
        }
    }
    
    printf("\n%sÖzet:%s %d UDP + %d TCP relay aktif\n", BOLD, RESET, udp_ok, tcp_ok);
}

void show_tox_id(Tox *tox) {
    uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox, tox_id_bin);
    
    char tox_id_hex[TOX_ADDRESS_SIZE * 2 + 1];
    bin_to_hex(tox_id_bin, TOX_ADDRESS_SIZE, tox_id_hex);
    
    printf("\n%s╔═══════════════════════════════════════════════════════════════════════════╗%s\n", BOLD CYAN, RESET);
    printf("%s║                            SİZİN TOX ID'NİZ                               ║%s\n", BOLD CYAN, RESET);
    printf("%s║                        (STATIC - Kalıcı)                                  ║%s\n", BOLD CYAN, RESET);
    printf("%s╠═══════════════════════════════════════════════════════════════════════════╣%s\n", BOLD CYAN, RESET);
    printf("%s║%s %s%-76s%s %s║%s\n", BOLD CYAN, RESET, YELLOW, tox_id_hex, RESET, BOLD CYAN, RESET);
    printf("%s╚═══════════════════════════════════════════════════════════════════════════╝%s\n", BOLD CYAN, RESET);
    
    // Public key'i de göster (debug için)
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(tox, public_key);
    char pk_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    bin_to_hex(public_key, TOX_PUBLIC_KEY_SIZE, pk_hex);
    printf("%sPublic Key:%s %s\n", CYAN, RESET, pk_hex);
}
// Arkadaş ekle
void add_friend(Tox *tox, const char *tox_id_hex, const char *message) {
    if (strlen(tox_id_hex) != TOX_ADDRESS_SIZE * 2) {
        printf("%s[HATA]%s Geçersiz Tox ID uzunluğu! (76 karakter olmalı)\n", RED, RESET);
        return;
    }
    
    uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
    hex_to_bin(tox_id_hex, tox_id_bin);
    
    TOX_ERR_FRIEND_ADD err;
    uint32_t friend_num = tox_friend_add(tox, tox_id_bin, (const uint8_t *)message, strlen(message), &err);
    
    if (err == TOX_ERR_FRIEND_ADD_OK) {
        printf("%s[✓]%s Arkadaş isteği gönderildi! Friend #%u\n", GREEN, RESET, friend_num);
        // Profili kaydet
        save_tox_state(tox);
    } else {
        printf("%s[✗]%s Arkadaş eklenemedi: ", RED, RESET);
        switch (err) {
            case TOX_ERR_FRIEND_ADD_TOO_LONG: printf("Mesaj çok uzun\n"); break;
            case TOX_ERR_FRIEND_ADD_NO_MESSAGE: printf("Mesaj boş\n"); break;
            case TOX_ERR_FRIEND_ADD_OWN_KEY: printf("Kendi ID'niz!\n"); break;
            case TOX_ERR_FRIEND_ADD_ALREADY_SENT: printf("Zaten gönderilmiş\n"); break;
            case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM: printf("Geçersiz ID checksum\n"); break;
            case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM: printf("Nospam değiştirilmeli\n"); break;
            default: printf("Bilinmeyen hata (%d)\n", err);
        }
    }
}

// Mesaj gönder
void send_message(Tox *tox, uint32_t friend_number, const char *message) {
    TOX_ERR_FRIEND_SEND_MESSAGE err;
    tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL, 
                           (const uint8_t *)message, strlen(message), &err);
    
    if (err == TOX_ERR_FRIEND_SEND_MESSAGE_OK) {
        printf("%s[GÖNDERİLDİ]%s %s\n", GREEN, RESET, message);
    } else {
        printf("%s[HATA]%s Mesaj gönderilemedi: ", RED, RESET);
        switch (err) {
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED: printf("Arkadaş çevrimdışı\n"); break;
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND: printf("Arkadaş bulunamadı\n"); break;
            case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG: printf("Mesaj çok uzun\n"); break;
            default: printf("Bilinmeyen hata (%d)\n", err);
        }
    }
}

// Arkadaş listesi
void list_friends(Tox *tox) {
    size_t count = tox_self_get_friend_list_size(tox);
    
    if (count == 0) {
        printf("\n%sHenüz arkadaş yok%s\n", YELLOW, RESET);
        return;
    }
    
    printf("\n%s╔══════════════════════════════════════════════════════════════╗%s\n", BOLD CYAN, RESET);
    printf("%s║                    ARKADAŞ LİSTESİ                           ║%s\n", BOLD CYAN, RESET);
    printf("%s╠══════════════════════════════════════════════════════════════╣%s\n", BOLD CYAN, RESET);
    
    uint32_t *friends = malloc(count * sizeof(uint32_t));
    tox_self_get_friend_list(tox, friends);
    
    for (size_t i = 0; i < count; i++) {
        TOX_CONNECTION status = tox_friend_get_connection_status(tox, friends[i], NULL);
        
        size_t name_size = tox_friend_get_name_size(tox, friends[i], NULL);
        char *name = malloc(name_size + 1);
        
        if (name && name_size > 0) {
            tox_friend_get_name(tox, friends[i], (uint8_t *)name, NULL);
            name[name_size] = '\0';
        } else {
            if (name) free(name);
            name = strdup("(İsimsiz)");
        }
        
        const char *status_str;
        const char *color;
        if (status == TOX_CONNECTION_NONE) {
            status_str = "⚫ Çevrimdışı";
            color = RED;
        } else if (status == TOX_CONNECTION_TCP) {
            status_str = "🟡 Çevrimiçi (TCP)";
            color = YELLOW;
        } else {
            status_str = "🟢 Çevrimiçi (UDP)";
            color = GREEN;
        }
        
        printf("%s║%s #%-3u %-30s %s%s%s\n", 
               BOLD CYAN, RESET, friends[i], name, color, status_str, RESET);
        
        free(name);
    }
    
    printf("%s╚══════════════════════════════════════════════════════════════╝%s\n", BOLD CYAN, RESET);
    free(friends);
}

// İsim değiştir
void change_name(Tox *tox) {
    char name[TOX_MAX_NAME_LENGTH + 1];
    
    printf("\n%sYeni isim:%s ", BOLD, RESET);
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;
    
    if (strlen(name) == 0) {
        printf("%s[HATA]%s İsim boş olamaz!\n", RED, RESET);
        return;
    }
    
    TOX_ERR_SET_INFO err;
    tox_self_set_name(tox, (uint8_t *)name, strlen(name), &err);
    
    if (err == TOX_ERR_SET_INFO_OK) {
        printf("%s[✓]%s İsim değiştirildi: %s\n", GREEN, RESET, name);
        save_tox_state(tox);
    } else {
        printf("%s[✗]%s İsim değiştirilemedi: %d\n", RED, RESET, err);
    }
}

// Profil silme
void delete_profile() {
    printf("\n%s⚠️  UYARI: Profil silinecek! Tox ID ve tüm arkadaşlar kaybolacak!%s\n", BOLD RED, RESET);
    printf("Onaylamak için 'EVET' yazın: ");
    
    char confirm[10];
    fgets(confirm, sizeof(confirm), stdin);
    confirm[strcspn(confirm, "\n")] = 0;
    
    if (strcmp(confirm, "EVET") == 0) {
        if (remove(TOX_SAVE_FILE) == 0) {
            printf("%s[✓]%s Profil silindi. Program yeniden başlatılıyor...\n", GREEN, RESET);
            running = 0;
        } else {
            printf("%s[HATA]%s Profil silinemedi!\n", RED, RESET);
        }
    } else {
        printf("%s[İPTAL]%s Profil silme iptal edildi\n", YELLOW, RESET);
    }
}

// Tox iterate thread
void *tox_iterate_thread(void *arg) {
    Tox *tox = (Tox *)arg;
    
    while (running) {
        tox_iterate(tox, NULL);
        usleep(tox_iteration_interval(tox) * 1000);
    }
    
    return NULL;
}

// Otomatik kayıt thread (her 30 saniyede)
void *auto_save_thread(void *arg) {
    Tox *tox = (Tox *)arg;
    
    while (running) {
        sleep(30);
        if (running) {
            save_tox_state(tox);
        }
    }
    
    return NULL;
}

// Signal handler (Ctrl+C)
void signal_handler(int sig) {
    printf("\n\n%s[KAPATILIYOR]%s Profil kaydediliyor...\n", YELLOW, RESET);
    if (global_tox) {
        save_tox_state(global_tox);
    }
    running = 0;
}

// Menü
void show_menu() {
    printf("\n%s╔════════════════════════════════════╗%s\n", BOLD MAGENTA, RESET);
    printf("%s║           KOMUTLAR                 ║%s\n", BOLD MAGENTA, RESET);
    printf("%s╠════════════════════════════════════╣%s\n", BOLD MAGENTA, RESET);
    printf("%s║%s 1. Tox ID göster                   %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 2. Arkadaş ekle                    %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 3. Mesaj gönder                    %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 4. Arkadaş listesi                 %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 5. İsim değiştir                   %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 6. Profili manuel kaydet           %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 7. Profili sil (yeni başlat)       %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s║%s 8. Çıkış                           %s║%s\n", BOLD MAGENTA, RESET, BOLD MAGENTA, RESET);
    printf("%s╚════════════════════════════════════╝%s\n", BOLD MAGENTA, RESET);
    printf("%sSeçim:%s ", BOLD, RESET);
}

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // ANSI renkleri aktif et (Windows 10/11)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #endif

    printf("%s╔═══════════════════════════════════════════════════════════════╗%s\n", BOLD GREEN, RESET);
    printf("%s║     TOX P2P MESAJLAŞMA SİSTEMİ (STATIC ID)                    ║%s\n", BOLD GREEN, RESET);
    printf("%s║     CGNAT Bypass: DHT + UDP Hole Punching + TCP Relay         ║%s\n", BOLD GREEN, RESET);
    printf("%s╚═══════════════════════════════════════════════════════════════╝%s\n", BOLD GREEN, RESET);
    
    // Signal handler
    signal(SIGINT, signal_handler);
    
    // Önceki profili yükle
    size_t save_size = 0;
    uint8_t *save_data = load_tox_state(&save_size);
    
    // Tox ayarları
    struct Tox_Options *options = tox_options_new(NULL);
    tox_options_set_udp_enabled(options, true);
    tox_options_set_tcp_port(options, 0);
    tox_options_set_hole_punching_enabled(options, true);
    
    // Eğer kayıtlı profil varsa yükle
    if (save_data && save_size > 0) {
        tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
        tox_options_set_savedata_data(options, save_data, save_size);
    }
    
    // Tox başlat
    TOX_ERR_NEW err_new;
    Tox *tox = tox_new(options, &err_new);
    tox_options_free(options);
    
    if (save_data) {
        free(save_data);
    }
    
    if (err_new != TOX_ERR_NEW_OK) {
        printf("%s[FATAL]%s Tox başlatılamadı: %d\n", RED, RESET, err_new);
        return 1;
    }
    
    global_tox = tox;
    
    // Callback'ler
    tox_callback_friend_request(tox, friend_request_cb);
    tox_callback_friend_message(tox, friend_message_cb);
    tox_callback_friend_connection_status(tox, friend_connection_status_cb);
    tox_callback_self_connection_status(tox, self_connection_status_cb);
    tox_callback_friend_name(tox, friend_name_cb);
    
    // İsim ayarla (eğer yoksa)
    size_t name_size = tox_self_get_name_size(tox);
    if (name_size == 0) {
        const char *name = "ToxUser";
        tox_self_set_name(tox, (const uint8_t *)name, strlen(name), NULL);
        printf("%s[BİLGİ]%s Varsayılan isim ayarlandı: %s\n", CYAN, RESET, name);
    }
    
    // Bootstrap
    bootstrap_to_network(tox);
    show_tox_id(tox);
    
    // İlk kayıt
    save_tox_state(tox);
    
    // Thread'leri başlat
    pthread_t iterate_tid, autosave_tid;
    pthread_create(&iterate_tid, NULL, tox_iterate_thread, tox);
    pthread_create(&autosave_tid, NULL, auto_save_thread, tox);
    
    printf("\n%s[BİLGİ]%s Otomatik kayıt aktif (30 saniyede bir)\n", CYAN, RESET);
    printf("%s[BİLGİ]%s Ağ bağlantısı kuruluyor, lütfen bekleyin...\n", CYAN, RESET);
    
    // Ana döngü
    while (running) {
        show_menu();
        
        int choice;
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                show_tox_id(tox);
                break;
                
            case 2: {
                char friend_id[TOX_ADDRESS_SIZE * 2 + 1];
                char message[256];
                
                printf("\n%sArkadaş Tox ID:%s ", BOLD, RESET);
                scanf("%s", friend_id);
                while (getchar() != '\n');
                
                printf("%sMesaj:%s ", BOLD, RESET);
                fgets(message, sizeof(message), stdin);
                message[strcspn(message, "\n")] = 0;
                
                add_friend(tox, friend_id, message);
                break;
            }
            
            case 3: {
                uint32_t friend_num;
                char message[1024];
                
                printf("\n%sArkadaş numarası:%s ", BOLD, RESET);
                scanf("%u", &friend_num);
                while (getchar() != '\n');
                
                printf("%sMesaj:%s ", BOLD, RESET);
                fgets(message, sizeof(message), stdin);
                message[strcspn(message, "\n")] = 0;
                
                send_message(tox, friend_num, message);
                break;
            }
            
            case 4:
                list_friends(tox);
                break;
                
            case 5:
                change_name(tox);
                break;
                
            case 6:
                save_tox_state(tox);
                break;
                
            case 7:
                delete_profile();
                break;
                
            case 8:
                running = 0;
                break;
                
            default:
                printf("%s[HATA]%s Geçersiz seçim!\n", RED, RESET);
        }
    }
    
    // Temizlik
    printf("\n%s[KAPANIYOR]%s Thread'ler durduruluyor...\n", YELLOW, RESET);
    pthread_join(iterate_tid, NULL);
    pthread_join(autosave_tid, NULL);
    
    printf("%s[KAPANIYOR]%s Son kayıt yapılıyor...\n", YELLOW, RESET);
    save_tox_state(tox);
    
    printf("%s[KAPANIYOR]%s Tox kapatılıyor...\n", YELLOW, RESET);
    tox_kill(tox);
    
    printf("%s[✓]%s Başarıyla kapatıldı! Profil kaydedildi.\n", GREEN, RESET);
    printf("%sBir sonraki başlatmada aynı Tox ID kullanılacak!%s\n", BOLD CYAN, RESET);
    
    return 0;
}
