#ifndef NMS_MEMORY_GUARD_H
#define NMS_MEMORY_GUARD_H

#include <iostream>
#include <unordered_map>
#include <cstddef>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <fstream>

class NMSMemoryGuard {
public:
    static NMSMemoryGuard& Ornek();

    void* BellekTahsis(size_t boyut, const char* dosya, int satir);
    void BellekSerbestBirak(void* adres);

    void BellekSizintilariRaporla();
    void PerformansRaporla();
    void BellekKullanimiRaporla();

    void TopluSerbestBirak();

    void HataAyiklamaModu(bool etkin);
    void BellekTemizlemeSuresiAyarla(int saniye);
    void TahsisBoyutSinirlama(size_t minBoyut, size_t maxBoyut);
    void KritikBellekEsiğiAyarla(size_t esik);
    void BellekRaporlariniDisariAktar(const std::string& format);
    
private:
    NMSMemoryGuard();
    ~NMSMemoryGuard();

    NMSMemoryGuard(const NMSMemoryGuard&) = delete;
    NMSMemoryGuard& operator=(const NMSMemoryGuard&) = delete;

    struct BellekBilgisi {
        size_t boyut;
        std::string konum;
        std::chrono::time_point<std::chrono::steady_clock> zamanDamgasi;
        bool butunlukBayragi;
        size_t erisimSayisi;
    };

    std::unordered_map<void*, BellekBilgisi> tahsisler;
    std::mutex kilit;
    bool hataAyiklama = false;
    int temizlemeSuresi = 0;
    size_t kritikBellekEsigi = 0;
    size_t minTahsisBoyutu = 0;
    size_t maxTahsisBoyutu = SIZE_MAX;

    void BellekTemizleyici();
    void ButunlukKontrolu(void* adres, BellekBilgisi& bilgi);
    void TahsisRaporla(void* adres, size_t boyut, const std::string& konum);
    void SerbestBirakRaporla(void* adres, const std::string& konum);
    void BellekEsikKontrolu(size_t toplamKullanilan);
    void RaporlariKaydet(const std::string& format);
};

#define NMS_YENI(boyut) NMSMemoryGuard::Ornek().BellekTahsis(boyut, __FILE__, __LINE__)
#define NMS_SIL(adres) NMSMemoryGuard::Ornek().BellekSerbestBirak(adres)

#endif // NMS_MEMORY_GUARD_H
