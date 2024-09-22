#include "NMSMemoryGuard.h"

NMSMemoryGuard& NMSMemoryGuard::Ornek() {
    static NMSMemoryGuard ornek;
    return ornek;
}

NMSMemoryGuard::NMSMemoryGuard() {
    if (temizlemeSuresi > 0) {
        std::thread(&NMSMemoryGuard::BellekTemizleyici, this).detach();
    }
}

void* NMSMemoryGuard::BellekTahsis(size_t boyut, const char* dosya, int satir) {
    std::lock_guard<std::mutex> kilitci(kilit);

    if (boyut < minTahsisBoyutu || boyut > maxTahsisBoyutu) {
        std::cerr << "Tahsis boyutu sınırlandırmanın dışında: " << boyut << " byte\n";
        return nullptr;
    }

    void* adres = malloc(boyut + sizeof(size_t));

    if (adres) {
        auto zamanDamgasi = std::chrono::steady_clock::now();
        tahsisler[adres] = {boyut, std::string(dosya) + ":" + std::to_string(satir), zamanDamgasi, true, 0};

        *((size_t*)adres) = 0xDEADBEEF;
        TahsisRaporla(adres, boyut, dosya + std::to_string(satir));

        BellekEsikKontrolu(boyut);
    }
    return (void*)((char*)adres + sizeof(size_t));
}

void NMSMemoryGuard::BellekSerbestBirak(void* adres) {
    if (!adres) return;
    
    adres = (void*)((char*)adres - sizeof(size_t));
    std::lock_guard<std::mutex> kilitci(kilit);

    auto it = tahsisler.find(adres);
    if (it != tahsisler.end()) {
        ButunlukKontrolu(adres, it->second);
        SerbestBirakRaporla(adres, it->second.konum);
        tahsisler.erase(it);
    }
    free(adres);
}

void NMSMemoryGuard::BellekSizintilariRaporla() {
    std::lock_guard<std::mutex> kilitci(kilit);
    if (!tahsisler.empty()) {
        std::cerr << "Bellek Sızıntıları Tespit Edildi:\n";
        for (const auto& girdi : tahsisler) {
            std::cerr << "Sızıntı: " << girdi.second.boyut << " byte " << girdi.first
                      << " adresinde (" << girdi.second.konum << ")\n";
        }
    } else {
        std::cout << "Bellek sızıntısı tespit edilmedi.\n";
    }
}

void NMSMemoryGuard::PerformansRaporla() {
    std::cout << "Performans Raporu:\n";
}

void NMSMemoryGuard::BellekKullanimiRaporla() {
    std::lock_guard<std::mutex> kilitci(kilit);
    size_t toplamTahsis = 0;
    for (const auto& girdi : tahsisler) {
        toplamTahsis += girdi.second.boyut;
    }
    std::cout << "Güncel Bellek Kullanımı: " << toplamTahsis << " byte\n";
}

void NMSMemoryGuard::TopluSerbestBirak() {
    std::lock_guard<std::mutex> kilitci(kilit);
    for (auto& girdi : tahsisler) {
        free(girdi.first);
    }
    tahsisler.clear();
}

void NMSMemoryGuard::HataAyiklamaModu(bool etkin) {
    hataAyiklama = etkin;
}

void NMSMemoryGuard::BellekTemizlemeSuresiAyarla(int saniye) {
    temizlemeSuresi = saniye;
}

void NMSMemoryGuard::TahsisBoyutSinirlama(size_t minBoyut, size_t maxBoyut) {
    minTahsisBoyutu = minBoyut;
    maxTahsisBoyutu = maxBoyut;
}

void NMSMemoryGuard::KritikBellekEsiğiAyarla(size_t esik) {
    kritikBellekEsigi = esik;
}

void NMSMemoryGuard::ButunlukKontrolu(void* adres, BellekBilgisi& bilgi) {
    size_t* bayrak = (size_t*)adres;
    if (*bayrak != 0xDEADBEEF) {
        std::cerr << "Bellek bozulması tespit edildi: " << adres << " (" << bilgi.konum << ")\n";
    }
}

void NMSMemoryGuard::BellekEsikKontrolu(size_t toplamKullanilan) {
    if (toplamKullanilan >= kritikBellekEsigi && kritikBellekEsigi > 0) {
        std::cerr << "Kritik bellek kullanım eşiği aşıldı: " << toplamKullanilan << " byte\n";
    }
}

void NMSMemoryGuard::BellekTemizleyici() {
    while (temizlemeSuresi > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(temizlemeSuresi));
        TopluSerbestBirak();
        std::cout << "Otomatik bellek temizliği yapıldı.\n";
    }
}

void NMSMemoryGuard::RaporlariKaydet(const std::string& format) {
    std::lock_guard<std::mutex> kilitci(kilit); // Bellek bilgilerine güvenli erişim için kilit

    if (format == "JSON") {
        std::ofstream dosya("bellek_raporu.json");
        dosya << "{\n\t\"bellek_raporu\": [\n";
        bool ilk = true;
        for (const auto& girdi : tahsisler) {
            if (!ilk) dosya << ",\n"; // JSON formatında virgül ayarlaması
            ilk = false;
            dosya << "\t\t{\n"
                  << "\t\t\t\"adres\": \"" << girdi.first << "\",\n"
                  << "\t\t\t\"boyut\": " << girdi.second.boyut << ",\n"
                  << "\t\t\t\"konum\": \"" << girdi.second.konum << "\",\n"
                  << "\t\t\t\"zaman\": \"" 
                  << std::chrono::duration_cast<std::chrono::seconds>(
                         girdi.second.zamanDamgasi.time_since_epoch()).count() 
                  << "\",\n"
                  << "\t\t\t\"erisimSayisi\": " << girdi.second.erisimSayisi << "\n"
                  << "\t\t}";
        }
        dosya << "\n\t]\n}\n";
        dosya.close();
        std::cout << "Bellek raporu JSON formatında kaydedildi: bellek_raporu.json\n";
    } else if (format == "CSV") {
        std::ofstream dosya("bellek_raporu.csv");
        dosya << "adres,boyut,konum,zaman,erisimSayisi\n";
        for (const auto& girdi : tahsisler) {
            dosya << girdi.first << ","
                  << girdi.second.boyut << ","
                  << girdi.second.konum << ","
                  << std::chrono::duration_cast<std::chrono::seconds>(
                         girdi.second.zamanDamgasi.time_since_epoch()).count() << ","
                  << girdi.second.erisimSayisi << "\n";
        }
        dosya.close();
        std::cout << "Bellek raporu CSV formatında kaydedildi: bellek_raporu.csv\n";
    } else {
        std::cerr << "Desteklenmeyen format: " << format << ". Lütfen JSON veya CSV seçin.\n";
    }
}

void NMSMemoryGuard::TahsisRaporla(void* adres, size_t boyut, const std::string& konum) {
    std::cout << "Bellek tahsis edildi: " << boyut << " byte (" << konum << ")\n";
}

void NMSMemoryGuard::SerbestBirakRaporla(void* adres, const std::string& konum) {
    std::cout << "Bellek serbest bırakıldı: " << adres << " (" << konum << ")\n";
}
