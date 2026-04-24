# Analisis Bonus — NIMONSPOLY

> Laporan ini menjelaskan seluruh fitur bonus yang dikerjakan dan direncanakan pada proyek **NIMONSPOLY**, termasuk bonus yang diusulkan oleh spesifikasi tugas besar dan kreasi mandiri tim pengembang. Setiap bagian disertai cuplikan kode dan penjelasan pendekatan teknis yang digunakan.

---

## 1. Bonus yang Diusulkan oleh Spesifikasi

### 1.1 GUI (Graphical User Interface)

**Status:** ✅ Sudah diimplementasi (SFML) — **refactor ke Raylib belum dikerjakan**.

**Pendekatan:**
Agar antarmuka grafis tidak menyebar ke seluruh basis kode, tim menggunakan **arsitektur berbasis interface** (Strategy Pattern untuk View & Input). `IGameView` dan `IGameInput` bertindak sebagai kontrak abstrak antara mesin permainan (`GameEngine`) dengan lapisan presentasi. Implementasi konkret saat ini menggunakan **SFML 3.x** (`GUIView` + `GUIInput`), namun karena seluruh logika game bergantung pada interface, refactor ke **Raylib** atau library grafis lain dapat dilakukan tanpa mengubah satu baris pun di `GameEngine`.

Struktur GUI terdiri dari beberapa screen yang dikelola oleh enum `AppScreen`: Landing, Setup (jumlah pemain, kustomisasi), Load Game, In-Game, dan Game Over. Selain itu, `AssetManager` (Singleton) bertugas memuat font dan tekstur secara terpusat agar tidak ada duplikasi resource di memori.

**Cuplikan Kode:**

```cpp
// include/ui/AppScreen.hpp
enum class AppScreen {
    LANDING,
    NEW_GAME_NUM_PLAYERS,
    NEW_GAME_CUST_PLAYER,
    NEW_GAME_CUST_MAP,
    LOAD_GAME,
    IN_GAME,
    GAME_OVER,
};
```

```cpp
// include/ui/IGameView.hpp (kontrak abstrak)
class IGameView {
public:
    virtual ~IGameView() = default;
    virtual void showBoard(const GameStateView& boardState) = 0;
    virtual void showDiceResult(int d1, int d2, const string& playerName) = 0;
    virtual void showBuyPrompt(const PropertyInfo& propertyInfo, Money playerMoney) = 0;
    // ... 20+ method virtual untuk seluruh aksi permainan
};
```

```cpp
// src/main.cpp — inisialisasi GUI (SFML)
#if NIMONSPOLY_ENABLE_SFML
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{1440u, 1024u}), "NIMONSPOLY");
    AssetManager::get().loadAll();
    GUIView view(window);
    GUIInput input(window);
```

**Rencana Refactor Raylib (Belum Dikerjakan):**
Arsitektur saat ini sudah mendukung pergantian library grafis karena `IGameView` dan `IGameInput` berupa interface. Namun, implementasi `RaylibView` dan `RaylibInput` **belum ada** di codebase. Refactor ini masih berupa rencana karena seluruh rendering GUI saat ini masih menggunakan SFML 3.x.

---

### 1.2 COM (Computer Player / AI)

**Status:** ✅ Sudah diimplementasi dengan strategi AI yang sophisticated.

**Pendekatan:**
Pengendali komputer diimplementasikan melalui `ComputerController` yang meng-extend `PlayerController` (Strategy Pattern). AI tidak bertindak random; ia memiliki **sistem penilaian (scoring)** untuk setiap keputusan:

- **Property Scoring:** Setiap tile diberi skor berdasarkan kode lokasi, tipe (Street/Railroad/Utility), color group, rasio sewa/harga, dan level bangunan. Contoh: Orange group (MAL, SMG, SBY) mendapat skor tertinggi karena frekuensi kunjungan tinggi.
- **Cash Reserve Management:** AI menjaga cadangan uang proporsional terhadap total kekayaan. Jika uang ≤ 300, reserve = 25–75; jika > 1200, reserve = max(200, cash/5). Ini mencegah AI bangkrut karena pembelian impulsif.
- **Auction Budgeting:** AI menghitung anggaran maksimal untuk lelang = `min(cash - reserve, cash × 45%)`. Jika cash ≤ 2000, budget dibatasi 420 agar tidak terlalu agresif di awal permainan.
- **Liquidation Optimization:** Saat likuidasi, AI memilih opsi dengan *waste* paling kecil (nilai likuidasi dikurangi kewajiban). Jika tidak ada yang pas, AI memilih opsi dengan nilai tertinggi sebagai fallback.
- **Turn Memory:** `ComputerController` menyimpan snapshot `GameStateView` dari turn sebelumnya untuk mencegah loop (misalnya: mengulang skill card atau bayar denda penjara berkali-kali dalam satu turn).

**Cuplikan Kode:**

```cpp
// include/controllers/ComputerController.hpp
class ComputerController final : public PlayerController {
public:
    string chooseCommand(const GameStateView& state) override;
    bool decideBuyProperty(const PropertyInfo& info, Money money) override;
    AuctionDecision decideAuction(int currentBid, Money money) override;
    int decideBuild(const BuildMenuState& state) override;
    int decideLiquidation(const LiquidationState& state) override;
private:
    int cashReserve(int cash) const;
    int propertyScore(const PropertyInfo& info) const;
    int propertyCodeScore(const string& code) const;
    int cardScore(const CardInfo& card) const;
    void refreshTurnMemory(const GameStateView& state);
};
```

```cpp
// src/controllers/ComputerController.cpp — property scoring
int ComputerController::propertyScore(const PropertyInfo& info) const {
    int score = propertyCodeScore(info.code);
    switch (info.type) {
        case TileType::STREET:   score += 15; break;
        case TileType::RAILROAD: score += 20; break;
        case TileType::UTILITY:  score += 5;  break;
        default: break;
    }
    switch (info.colorGroup) {
        case Color::ORANGE:      score += 30; break;
        case Color::RED:         score += 28; break;
        case Color::LIGHT_BLUE:  score += 24; break;
        case Color::YELLOW:      score += 22; break;
        case Color::PINK:        score += 18; break;
        case Color::GREEN:       score += 17; break;
        case Color::DARK_BLUE:   score += 15; break;
        case Color::BROWN:       score += 14; break;
        default: break;
    }
    const int price = info.purchasePrice.getAmount();
    const int bestRent = maxRent(info);
    if (bestRent > 0) {
        score += min(35, bestRent / 40);
        if (price > 0) score += min(25, bestRent * 10 / price);
    }
    score += info.buildingLevel * 20;
    if (info.status == PropertyStatus::MORTGAGED) score -= 35;
    return score;
}
```

```cpp
// Auction AI dengan budgeting cerdas
AuctionDecision ComputerController::decideAuction(int currentBid, Money money) {
    const int cash = money.getAmount();
    const int minimumBid = max(0, currentBid + 1);
    if (cash <= minimumBid) return AuctionDecision{AuctionAction::PASS, 0};

    int budget = cash - cashReserve(cash);
    budget = min(budget, cash * 45 / 100);
    if (cash <= 2000) budget = min(budget, 420);
    if (budget < minimumBid) return AuctionDecision{AuctionAction::PASS, 0};

    const int step = max(10, cash / 50);
    int bid = currentBid < 0 ? min(budget, max(50, step * 3))
                             : min(budget, currentBid + step);
    bid = max(bid, minimumBid);
    return AuctionDecision{AuctionAction::BID, bid};
}
```

---

### 1.3 Papan Dinamis

**Status:** ⏳ Belum diimplementasi — direncanakan pada milestone berikutnya.

**Pendekatan yang Akan Digunakan:**
Saat ini, papan permainan (`Board`) diinisialisasi secara statis dengan 40 tile sesuai spesifikasi. Fitur papan dinamis direncanakan dengan cara:

1. **Board Factory / Builder Pattern:** Membuat kelas `BoardFactory` yang membaca konfigurasi tile dari file eksternal (JSON/CSV/txt) dan menghasilkan `vector<unique_ptr<Tile>>` sesuai jumlah tile yang diinginkan.
2. **Procedural Layout Generator:** Jika jumlah tile bukan 40, generator akan menyesuaikan ukuran grid persegi (misalnya 36 tile = 6×6, 48 tile = 8×6) sambil mempertahankan proporsi tipe tile (Street, Railroad, Utility, Action, Special).
3. **Dynamic Rent Scaling:** Jika tile bertambah, rent table dan harga properti akan di-scale secara proporsional agar game balance tetap terjaga.

Struktur data sudah siap karena `Board` menyimpan tiles dalam `vector<unique_ptr<Tile>>`, sehingga jumlah tile tidak hardcoded di kelas `Board`.

**Cuplikan Kode (struktur yang sudah mendukung dinamis):**

```cpp
// include/models/board/header/Board.hpp
class Board {
public:
    void addTile(std::unique_ptr<Tile> tile);
    Tile* getTile(int index) const;
    Tile* getTileByCode(const std::string& code) const;
    int getSize() const;  // bukan constexpr 40, tapi hasil size()
private:
    std::vector<std::unique_ptr<Tile>> tiles;
};
```

```cpp
// include/ui/AppScreen.hpp — setup state sudah menyiapkan field numTiles
struct SetupState {
    int numTiles{40};  // siap untuk diubah saat papan dinamis diimplementasi
    // ...
};
```

---

### 1.4 Kreativitas (Design Patterns & Arsitektur)

**Status:** ✅ Diterapkan secara ekstensif di seluruh basis kode.

Tim secara sadar menerapkan berbagai pola desain agar kode mudah di-maintain, di-test, dan di-extend. Berikut tabel ringkasan:

| Pattern | Lokasi Implementasi | Tujuan |
|---------|---------------------|--------|
| **Strategy** | `PlayerController` → `HumanController` / `ComputerController` | Ganti perilaku pemain (manusia vs AI) tanpa ubah engine |
| **Command** | `Command` + `CommandRegistry` | Eksekusi perintah permainan terpusat dan extensible |
| **Builder** | `GameBuilder` | Konstruksi `GameEngine` step-by-step dengan dependency injection |
| **Repository** | `IGameRepository` → `TextFileRepository` | Abstraksi penyimpanan (file, DB, cloud) |
| **Observer** | `EventBus` + `IEventListener` | Decoupled event handling antar modul |
| **Singleton** | `AssetManager` | Satu instance pengelola resource grafis |
| **DTO** | `UiDtos.hpp` | Transfer data ringan antar model dan UI tanpa circular dependency |
| **Memento** | `FestivalEffectSnapshot` | Capture & restore state efek festival saat save/load |
| **Effect System** | `Effect` base class | Modular buff/debuff yang bisa di-chain |

**Cuplikan Kode:**

```cpp
// include/core/command/header/Command.hpp — Command Pattern
class Command {
public:
    using Handler = std::function<void()>;
    Command(std::string name = "", Handler handler = nullptr);
    void execute() const;
private:
    std::string name;
    Handler handler;
};
```

```cpp
// include/core/IGameRepository.hpp — Repository Pattern
class IGameRepository {
public:
    virtual ~IGameRepository() = default;
    virtual bool save(const GameState& state, const Board& board,
                      const TransactionLogger& logger,
                      const FestivalManager& festivals,
                      const std::string& id) = 0;
    virtual bool loadInto(GameState& state, Board& board,
                          TransactionLogger& logger,
                          FestivalManager& festivals,
                          const std::string& id) = 0;
};
```

```cpp
// include/manager/header/EventBus.hpp — Observer Pattern
class EventBus {
public:
    void subscribe(IEventListener* listener);
    void unsubscribe(IEventListener* listener);
    void publish(const GameEvent& event) const;
private:
    std::vector<IEventListener*> listeners;
};
```

---

## 2. Bonus Kreasi Mandiri

### 2.1 Dual UI Mode (CLI & GUI dalam Satu Codebase)

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
Tim mendesain sistem agar satu executable bisa berjalan dalam mode CLI (terminal) atau GUI (grafis) hanya dengan mengubah flag saat build:

```bash
cmake -S . -B build                    # CLI mode (default)
cmake -S . -B build -DNIMONSPOLY_ENABLE_SFML=ON  # GUI mode
```

Dalam `main.cpp`, seluruh logika di-compile conditionally menggunakan `#if NIMONSPOLY_ENABLE_SFML`. Namun, karena `GameEngine` hanya bergantung pada `IGameView` dan `IGameInput`, mode CLI dan GUI berbagi **100% logika permainan yang sama**. Hanya lapisan view dan input yang berbeda.

**Cuplikan Kode:**

```cpp
// src/main.cpp — dual mode entry point
#if NIMONSPOLY_ENABLE_SFML
    // GUI mode: SFML window, GUIView, GUIInput
    sf::RenderWindow window(...);
    GUIView view(window);
    GUIInput input(window);
    // ... game loop dengan rendering SFML
#else
    // CLI mode: terminal, CLIView, ConsoleInput
    ConsoleInput input;
    HumanController controller(&input);
    GameEngine engine;
    // ... game loop dengan cin/cout
#endif
```

---

### 2.2 ANSI Colored CLI Output

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
`CLIView` tidak sekadar mencetak teks biasa. Ia menggunakan **ANSI escape codes** untuk memberikan warna pada setiap tile sesuai color group-nya (Coklat, Biru Muda, Pink, Orange, Merah, Kuning, Hijau, Biru Tua). Selain itu, papan dicetak dalam format grid persegi yang mirip dengan visualisasi pada spesifikasi, lengkap dengan:
- Tag warna (`[CK]`, `[BM]`, `[OR]`, dst.)
- Indikator kepemilikan (`P1`, `P2`, ...)
- Indikator bangunan (`^`, `^^`, `^^^`, `*` untuk hotel)
- Posisi bidak pemain (`(1)`, `(2)`, ...)

**Cuplikan Kode:**

```cpp
// src/views/CLIView.cpp
static string ansiColor(Color c) {
    switch (c) {
        case Color::BROWN:      return "\033[38;5;130m";
        case Color::LIGHT_BLUE: return "\033[96m";
        case Color::PINK:       return "\033[95m";
        case Color::ORANGE:     return "\033[38;5;208m";
        case Color::RED:        return "\033[91m";
        case Color::YELLOW:     return "\033[93m";
        case Color::GREEN:      return "\033[92m";
        case Color::DARK_BLUE:  return "\033[34m";
        case Color::GRAY:       return "\033[90m";
        default:                return "\033[37m";
    }
}
```

```cpp
// Rendering papan dengan warna
void CLIView::showBoard(const GameStateView& state) {
    // ... build helper maps for player positions & properties
    auto colorLine1 = [&](int idx) -> string {
        const auto& t = getTile(idx);
        return ansiColor(t.color) + tileL1(t) + ANSI_RESET;
    };
    // Cetak border + 11 tile per baris (top, side, bottom)
}
```

---

### 2.3 Modular Effect System

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
Sistem efek didesain secara **modular dan chainable**. Kelas abstrak `Effect` mendefinisikan tiga hook utama:

1. `modifyOutgoingPayment` — mengurangi/menambah uang yang keluar (contoh: diskon pajak).
2. `modifyRent` — mengubah sewa yang diterima/dibayar (contoh: festival lipatgandakan sewa).
3. `blockPayment` — memblokir pembayaran sepenuhnya (contoh: shield card).

Setiap `Player` menyimpan `vector<unique_ptr<Effect>>`. Saat pembayaran terjadi, `Bank` atau `GameEngine` memanggil `player.applyOutgoingModifiers(amount)` yang secara otomatis menelusuri seluruh efek aktif.

**Cuplikan Kode:**

```cpp
// include/models/effects/Effect.hpp
class Effect {
public:
    virtual Money modifyOutgoingPayment(const Money& amount) const;
    virtual Money modifyRent(const Money& amount, PropertyTile* prop) const;
    virtual bool blockPayment() const;
    void tick();
    bool isExpired() const;
protected:
    int remainingTurns;
};
```

```cpp
// src/models/effects/ShieldEffect.cpp
bool ShieldEffect::blockPayment() const { return true; }
```

```cpp
// src/models/effects/FestivalEffect.cpp
Money FestivalRentEffect::modifyRent(const Money& amount, PropertyTile*) const {
    return Money(amount.getAmount() * multiplier);
}
```

```cpp
// src/core/Bank.cpp — chain efek saat transfer
void Bank::transferBetweenPlayers(Player& from, Player& to, const Money& amount, ...) {
    Money payableAmount = getPayableAmount(from, amount);
    // getPayableAmount memanggil from.applyOutgoingModifiers(amount)
    // yang menelusuri seluruh efek aktif (discount, shield, dll)
    from.deductMoney(payableAmount);
    to.addMoney(payableAmount);
}
```

---

### 2.4 Festival Manager dengan Multiplier Bertingkat

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
Sesuai spesifikasi, efek festival bisa di-stack hingga 3 kali (multiplier maksimal 8×). `FestivalManager` mengelola map dari `PropertyTile*` ke `FestivalPropertyEffect`. Setiap kali pemain mendarat di Festival Tile, sistem mengecek:

- Jika efek belum ada → buat baru dengan multiplier 2×, durasi 3 turn.
- Jika efek sudah ada dan `< 3 kali apply` → naikkan level (2× → 4× → 8×), reset durasi ke 3.
- Jika sudah 3 kali → hanya reset durasi, tidak naik multiplier lagi.

Selain itu, `tickPlayerEffects` dipanggil setiap turn untuk mengurangi durasi. Jika durasi habis, efek dihapus dari map.

**Cuplikan Kode:**

```cpp
// include/core/FestivalManager.hpp
class FestivalManager {
public:
    FestivalResult applyFestival(Player& player, PropertyTile& property);
    void tickPlayerEffects(Player& player);
    int getMultiplier(PropertyTile* property) const;
    int getDuration(PropertyTile* property) const;
private:
    class FestivalPropertyEffect { ... };
    static int multiplierFor(int timesApplied);
    std::map<PropertyTile*, FestivalPropertyEffect> activeEffects;
};
```

```cpp
// src/core/FestivalManager.cpp
int FestivalManager::multiplierFor(int timesApplied) {
    switch (timesApplied) {
        case 1:  return 2;
        case 2:  return 4;
        case 3:  return 8;
        default: return 1;
    }
}

FestivalResult FestivalManager::applyFestival(Player& player, PropertyTile& property) {
    auto it = activeEffects.find(&property);
    if (it != activeEffects.end()) {
        if (it->second.getTimesApplied() >= 3) {
            return FestivalResult(false, it->second.getMultiplier(), it->second.getTurnsRemaining());
        }
        it->second.setTimesApplied(it->second.getTimesApplied() + 1);
        it->second.setMultiplier(multiplierFor(it->second.getTimesApplied()));
        it->second.setTurnsRemaining(3);
        player.addEffect(new FestivalRentEffect(it->second.getMultiplier(), 3));
        return FestivalResult(true, it->second.getMultiplier(), 3);
    }
    // ... inisialisasi efek baru
}
```

---

### 2.5 Custom Exception Hierarchy

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
Seluruh error domain-specific diwakili oleh hierarki exception turunan `NimonopoliException`. Ini memudahkan penanganan error terstruktur dan memberikan pesan yang informatif ke pemain:

- `InsufficientFundsException` — uang tidak cukup untuk transaksi.
- `InvalidCommandException` — perintah tidak dikenali.
- `InvalidPropertyException` — kode properti tidak valid.
- `CardSlotFullException` — tangan kartu penuh (maks 3).
- `InvalidBidException` — bid lelang tidak valid.
- `SaveLoadException` — gagal baca/tulis file save.
- `InvalidBoardConfigException` — konfigurasi papan tidak valid.

**Cuplikan Kode:**

```cpp
// include/utils/Exceptions.hpp
class NimonopoliException : public exception {
public:
    explicit NimonopoliException(string message);
    const char* what() const noexcept override;
private:
    string message;
};

class InsufficientFundsException final : public NimonopoliException {
public:
    InsufficientFundsException(Money required, Money available);
    Money getRequired() const;
    Money getAvailable() const;
private:
    Money required;
    Money available;
};

class CardSlotFullException final : public NimonopoliException {
public:
    explicit CardSlotFullException(int maxSlots);
    int getMaxSlots() const;
private:
    int maxSlots;
};
```

---

### 2.6 DTO & Snapshot Pattern untuk Save/Load

**Status:** ✅ Sudah diimplementasi.

**Pendekatan:**
Untuk menghindari circular dependency antara model dan UI, tim membuat **DTO (Data Transfer Object)** di `UiDtos.hpp`. DTO ini berisi data ringan seperti `PropertyInfo`, `RentInfo`, `CardInfo`, `JailInfo`, dll. Selain itu, untuk save/load, `FestivalEffectSnapshot` menyimpan state efek festival agar bisa di-restore sempurna saat permainan dimuat kembali.

**Cuplikan Kode:**

```cpp
// include/ui/UiDtos.hpp — DTOs bebas dari heavy model includes
class PropertyInfo {
public:
    string code;
    string name;
    string ownerName;
    PropertyStatus status{PropertyStatus::BANK};
    int buildingLevel{0};
    Money purchasePrice{0};
};

class RentInfo {
public:
    string payerName;
    string ownerName;
    Money amount{0};
    PropertyInfo property;
};
```

```cpp
// include/core/FestivalEffectSnapshot.hpp
class FestivalEffectSnapshot {
public:
    FestivalEffectSnapshot(PropertyTile* property, Player* owner,
                           int multiplier, int turnsRemaining, int timesApplied);
    PropertyTile* getProperty() const;
    Player* getOwner() const;
    int getMultiplier() const;
    int getTurnsRemaining() const;
    int getTimesApplied() const;
private:
    PropertyTile* property;
    Player* owner;
    int multiplier;
    int turnsRemaining;
    int timesApplied;
};
```

---

## 3. Ringkasan Implementasi Bonus

| No | Fitur Bonus | Status | Keterangan Singkat |
|----|------------|--------|-------------------|
| 1 | GUI | ✅ | SFML, interface-based, siap refactor ke Raylib |
| 2 | COM | ✅ | AI sophisticated dengan scoring & memory |
| 3 | Papan Dinamis | ⏳ | Struktur sudah siap, menunggu implementasi BoardFactory |
| 4 | Kreativitas | ✅ | 10+ design pattern digunakan |
| 5 | Dual UI Mode | ✅ | CLI & GUI dalam satu codebase via CMake flag |
| 6 | ANSI Colored CLI | ✅ | Output terminal berwarna sesuai color group |
| 7 | Modular Effect System | ✅ | Chainable buff/debuff (Festival, Shield, Discount) |
| 8 | Festival Manager | ✅ | Multiplier 2×→4×→8× dengan durasi terpisah per properti |
| 9 | Exception Hierarchy | ✅ | 7 jenis exception domain-specific |
| 10 | DTO & Snapshot | ✅ | Decoupled UI + save/load state efek festival |

---

## 4. Penutup

Tim **BurntCheesecake** mendesain NIMONSPOLY dengan prinsip **Clean Architecture** dan **SOLID**. Setiap fitur bonus diimplementasikan bukan sekadar "ada", tetapi melalui perencanaan arsitektur yang matang agar kode tetap maintainable, extensible, dan testable. Fitur seperti GUI, COM, dan Effect System saling terintegrasi melalui interface sehingga perubahan di satu modul tidak merusak modul lain. Fitur yang belum dikerjakan (Papan Dinamis) juga sudah memiliki fondasi arsitektur yang kuat untuk diimplementasi di iterasi berikutnya.
