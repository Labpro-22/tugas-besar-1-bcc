#include "ui/CLIView.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "core/state/header/GameStateView.hpp"
#include "utils/Enums.hpp"


static const string ANSI_RESET = "\033[0m";
static const string ANSI_BOLD  = "\033[1m";

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

static string colorCode(Color c) {
    switch (c) {
        case Color::BROWN:      return "CK";
        case Color::LIGHT_BLUE: return "BM";
        case Color::PINK:       return "PK";
        case Color::ORANGE:     return "OR";
        case Color::RED:        return "MR";
        case Color::YELLOW:     return "KN";
        case Color::GREEN:      return "HJ";
        case Color::DARK_BLUE:  return "BT";
        case Color::GRAY:       return "AB";
        default:                return "DF";
    }
}

static string colorName(Color c) {
    switch (c) {
        case Color::BROWN:      return "COKLAT";
        case Color::LIGHT_BLUE: return "BIRU MUDA";
        case Color::PINK:       return "PINK";
        case Color::ORANGE:     return "ORANGE";
        case Color::RED:        return "MERAH";
        case Color::YELLOW:     return "KUNING";
        case Color::GREEN:      return "HIJAU";
        case Color::DARK_BLUE:  return "BIRU TUA";
        case Color::GRAY:       return "ABU-ABU";
        default:                return "DEFAULT";
    }
}

static string buildingStr(int level) {
    if (level <= 0) return "";
    if (level >= 5) return "*";
    return string(static_cast<size_t>(level), '^');
}

static string pad(const string& s, int width, char fill = ' ') {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, static_cast<size_t>(width));
    return s + string(static_cast<size_t>(width - static_cast<int>(s.size())), fill);
}

static string center(const string& s, int width) {
    int total = width - static_cast<int>(s.size());
    if (total <= 0) return s.substr(0, static_cast<size_t>(width));
    int left = total / 2;
    int right = total - left;
    return string(static_cast<size_t>(left), ' ') + s + string(static_cast<size_t>(right), ' ');
}

static string moneyFmt(int amount) {
    return "M" + to_string(amount);
}

static string moneyFmt(const Money& m) {
    return moneyFmt(m.getAmount());
}

// Returns the 10-char visible content of line 1 (color tag + code)
static string tileL1(const TileView& t) {
    return "[" + colorCode(t.color) + "] " + pad(t.code, 3) + "  ";
}

// Returns the 10-char visible content of line 2 (owner/building/tokens)
static string tileL2(
    const TileView& t,
    const PropertyView* prop,
    const map<string, int>& playerNum,            // username → 1-based index
    const vector<pair<int, PlayerStatus>>& here   // (1-based index, status)
) {
    constexpr int W = 10;

    // Jail tile: show IN / V breakdown
    if (t.type == TileType::JAIL) {
        string jailed, visiting;
        for (const auto& [idx, status] : here) {
            if (status == PlayerStatus::JAILED) {
                if (!jailed.empty()) jailed += ",";
                jailed += to_string(idx);
            } else {
                if (!visiting.empty()) visiting += ",";
                visiting += to_string(idx);
            }
        }
        string s;
        if (!jailed.empty())  s += "IN:" + jailed;
        if (!jailed.empty() && !visiting.empty()) s += " ";
        if (!visiting.empty()) s += "V:" + visiting;
        return pad(s, W);
    }

    // Ownership info (max 6 chars: "P1 ^^^")
    string info;
    if (prop && prop->status != PropertyStatus::BANK && !prop->ownerName.empty()) {
        auto it = playerNum.find(prop->ownerName);
        if (it != playerNum.end()) {
            info = "P" + to_string(it->second);
            string bld = buildingStr(prop->buildingLevel);
            if (!bld.empty()) info += " " + bld;
            if (prop->status == PropertyStatus::MORTGAGED) info += "[M]";
        }
    }

    // Player tokens (each "(N)")
    string tokens;
    for (const auto& [idx, status] : here) {
        tokens += "(" + to_string(idx) + ")";
    }

    if (info.empty() && tokens.empty()) return string(W, ' ');

    // Lay out: info left-padded, tokens right-aligned within 10 chars
    string combined = pad(info, max(1, W - static_cast<int>(tokens.size()))) + tokens;
    return pad(combined, W);
}

void CLIView::showBoard(const GameStateView& state) {
    // Build helper maps
    map<string, int> playerNum;
    for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
        playerNum[state.players[i].username] = i + 1;
    }

    map<string, const PropertyView*> propByCode;
    for (const auto& p : state.properties) {
        propByCode[p.code] = &p;
    }

    // tile index → list of (1-based player number, PlayerStatus)
    map<int, vector<pair<int, PlayerStatus>>> playersAt;
    for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
        const auto& pv = state.players[i];
        if (pv.status != PlayerStatus::BANKRUPT) {
            playersAt[pv.position].emplace_back(i + 1, pv.status);
        }
    }

    // Safe tile/prop accessors
    const int TOTAL_TILES = static_cast<int>(state.tiles.size());
    static const TileView EMPTY_TILE{0, "---", "---", TileType::GO, Color::DEFAULT};
    auto getTile = [&](int idx) -> const TileView& {
        if (idx < 0 || idx >= TOTAL_TILES) return EMPTY_TILE;
        return state.tiles[static_cast<size_t>(idx)];
    };
    auto getProp = [&](const string& code) -> const PropertyView* {
        auto it = propByCode.find(code);
        return it != propByCode.end() ? it->second : nullptr;
    };
    auto getHere = [&](int idx) -> const vector<pair<int, PlayerStatus>>& {
        static const vector<pair<int, PlayerStatus>> EMPTY;
        auto it = playersAt.find(idx);
        return it != playersAt.end() ? it->second : EMPTY;
    };

    // Helpers to produce colored line 1 with ANSI and plain line 2
    auto colorLine1 = [&](int idx) -> string {
        const auto& t = getTile(idx);
        return ansiColor(t.color) + tileL1(t) + ANSI_RESET;
    };
    auto plainLine2 = [&](int idx) -> string {
        const auto& t = getTile(idx);
        return tileL2(t, getProp(t.code), playerNum, getHere(idx));
    };

    // Full 11-tile border
    auto borderFull = [&]() -> string {
        string s;
        for (int i = 0; i < 11; ++i) s += "+----------";
        s += "+";
        return s;
    };
    // Side-only border: left tile + 98 spaces + right tile
    auto borderSide = [&]() -> string {
        return "+----------+" + string(98, ' ') + "+----------+";
    };

    string turnStr = "TURN " + to_string(state.currentTurn) + " / " + to_string(state.maxTurn);
    string cl[18];
    cl[0]  = string(98, ' ');
    cl[1]  = string(98, ' ');
    cl[2]  = center("==================================", 98);
    cl[3]  = center("||          NIMONSPOLI          ||", 98);
    cl[4]  = center("==================================", 98);
    cl[5]  = string(98, ' ');
    cl[6]  = center(turnStr, 98);
    cl[7]  = string(98, ' ');
    cl[8]  = center("----------------------------------", 98);
    cl[9]  = center("LEGENDA KEPEMILIKAN & STATUS", 98);
    cl[10] = center("P1-P4 : Properti milik Pemain 1-4", 98);
    cl[11] = center("^     : Rumah Level 1", 98);
    cl[12] = center("^^    : Rumah Level 2", 98);
    cl[13] = center("^^^   : Rumah Level 3", 98);
    cl[14] = center("* : Hotel (Maksimal)", 98);
    cl[15] = center("(1)-(4): Bidak (IN=Tahanan, V=Mampir)", 98);
    cl[16] = center("----------------------------------", 98);
    cl[17] = string(98, ' ');

    // Top border
    out_ << borderFull() << "\n";

    // Top row (tiles 20..30)
    {
        string r1 = "|", r2 = "|";
        for (int i = 20; i <= 30; ++i) {
            r1 += colorLine1(i) + "|";
            r2 += plainLine2(i) + "|";
        }
        out_ << r1 << "\n" << r2 << "\n";
    }

    // Separator between top row and side rows
    out_ << borderFull() << "\n";

    // Side rows (9 rows: left tile[19..11], right tile[31..39])
    for (int row = 0; row < 9; ++row) {
        int li = 19 - row;
        int ri = 31 + row;
        string lL1 = colorLine1(li), lL2 = plainLine2(li);
        string rL1 = colorLine1(ri), rL2 = plainLine2(ri);

        out_ << "|" << lL1 << "|" << cl[row * 2]     << "|" << rL1 << "|\n";
        out_ << "|" << lL2 << "|" << cl[row * 2 + 1] << "|" << rL2 << "|\n";

        // Last side row uses the full border (shared with bottom row top)
        if (row < 8) out_ << borderSide() << "\n";
        else         out_ << borderFull() << "\n";
    }

    // Bottom row (tiles 10..0, left→right)
    {
        string r1 = "|", r2 = "|";
        for (int i = 10; i >= 0; --i) {
            r1 += colorLine1(i) + "|";
            r2 += plainLine2(i) + "|";
        }
        out_ << r1 << "\n" << r2 << "\n";
    }
    out_ << borderFull() << "\n\n";
}


CLIView::CLIView(std::ostream& out) : out_(out) {}


void CLIView::showMainMenu() {
    out_ << "\n";
    out_ << "====================================\n";
    out_ << "          NIMONSPOLI                \n";
    out_ << "====================================\n";
    out_ << "1. New Game\n";
    out_ << "2. Load Game\n";
    out_ << "====================================\n";
    out_ << "Pilih opsi: ";
}

void CLIView::showPlayerOrder(const vector<string>& orderedNames) {
    out_ << "\nUrutan giliran pemain:\n";
    for (int i = 0; i < static_cast<int>(orderedNames.size()); ++i) {
        out_ << "  " << (i + 1) << ". " << orderedNames[i] << "\n";
    }
    out_ << "\n";
}


void CLIView::showTurnInfo(const string& playerName, int turnNum, int maxTurn) {
    out_ << "\n=== Giliran: " << playerName
         << " | Turn " << turnNum << "/" << maxTurn << " ===\n";
}


void CLIView::showDiceResult(int d1, int d2, const string& playerName) {
    out_ << "\nMengocok dadu...\n";
    out_ << "Hasil: " << d1 << " + " << d2 << " = " << (d1 + d2) << "\n";
    out_ << "Memajukan Bidak " << playerName << " sebanyak " << (d1 + d2) << " petak...\n";
}

void CLIView::showPlayerLanding(const string& playerName, const string& tileName) {
    out_ << "Bidak " << playerName << " mendarat di: " << tileName << ".\n";
}

void CLIView::showDoubleBonusTurn(const string& playerName, int doubleCount) {
    if (doubleCount >= 3) {
        out_ << "\n[!] " << playerName << " mendapat double untuk ke-3 kalinya! Langsung ke Penjara!\n";
    } else {
        out_ << "\nDouble ke-" << doubleCount << "! " << playerName << " mendapat giliran tambahan.\n";
    }
}


void CLIView::showPropertyCard(const PropertyInfo& info) {
    string typeName;
    switch (info.type) {
        case TileType::RAILROAD: typeName = "STASIUN"; break;
        case TileType::UTILITY:  typeName = "UTILITAS"; break;
        default:                 typeName = "LAHAN";
    }

    out_ << "\n+================================+\n";
    out_ << "|        AKTA KEPEMILIKAN        |\n";
    out_ << "| [" << pad(colorName(info.colorGroup), 8) << "] "
         << pad(info.name + " (" + info.code + ")", 18) << " |\n";
    out_ << "+================================+\n";
    out_ << "| Harga Beli        : " << pad(moneyFmt(info.purchasePrice), 10) << " |\n";
    out_ << "| Nilai Gadai       : " << pad(moneyFmt(info.mortgageValue), 10) << " |\n";

    if (info.type == TileType::STREET) {
        out_ << "+--------------------------------+\n";
        const char* labels[] = {
            "Sewa (unimproved)",
            "Sewa (1 rumah)   ",
            "Sewa (2 rumah)   ",
            "Sewa (3 rumah)   ",
            "Sewa (4 rumah)   ",
            "Sewa (hotel)     "
        };
        for (int i = 0; i < 6; ++i) {
            int rent = info.rentTable[static_cast<size_t>(i)];
            if (i == 0 && info.festivalMultiplier > 1)
                rent *= info.festivalMultiplier;
            out_ << "| " << labels[i] << " : " << pad(moneyFmt(rent), 8) << " |\n";
        }
        out_ << "+--------------------------------+\n";
        out_ << "| Harga Rumah       : " << pad(moneyFmt(info.buildCostPerHouse), 10) << " |\n";
        out_ << "| Harga Hotel       : " << pad(moneyFmt(info.buildCostHotel), 10)    << " |\n";
    }

    out_ << "+================================+\n";

    string statusStr;
    if (info.status == PropertyStatus::BANK) {
        statusStr = "BANK";
    } else if (info.status == PropertyStatus::MORTGAGED) {
        statusStr = "MORTGAGED (" + info.ownerName + ")";
    } else {
        statusStr = "OWNED (" + info.ownerName + ")";
    }
    out_ << "| Status : " << pad(statusStr, 22) << " |\n";

    if (info.festivalMultiplier > 1) {
        out_ << "| Festival x" << info.festivalMultiplier
             << " (sisa " << info.festivalDurationLeft << " giliran)         |\n";
    }
    out_ << "+================================+\n\n";
}

void CLIView::showPlayerProperties(const vector<PropertyInfo>& list) {
    if (list.empty()) {
        out_ << "Kamu belum memiliki properti apapun.\n";
        return;
    }

    out_ << "\n=== Properti Milikmu ===\n";

    // Group by color
    map<Color, vector<const PropertyInfo*>> grouped;
    for (const auto& p : list) grouped[p.colorGroup].push_back(&p);

    int total = 0;
    for (const auto& [color, props] : grouped) {
        out_ << "\n[" << colorName(color) << "]\n";
        for (const auto* p : props) {
            string statusFlag = (p->status == PropertyStatus::MORTGAGED) ? " [M]" : "";
            string bld = (p->buildingLevel > 0) ? ("  " + buildingStr(p->buildingLevel)) : "";
            out_ << "  - " << p->name << " (" << p->code << ")"
                 << bld << "  " << moneyFmt(p->purchasePrice) << "  "
                 << (p->status == PropertyStatus::MORTGAGED ? "MORTGAGED" : "OWNED")
                 << statusFlag << "\n";
            total += p->purchasePrice.getAmount();
            if (p->buildingLevel > 0 && p->type == TileType::STREET) {
                if (p->buildingLevel < 5) {
                    total += p->buildCostPerHouse * p->buildingLevel;
                } else {
                    total += p->buildCostPerHouse * 4 + p->buildCostHotel;
                }
            }
        }
    }
    out_ << "\nTotal kekayaan properti: " << moneyFmt(total) << "\n\n";
}


void CLIView::showBuyPrompt(const PropertyInfo& info, Money playerMoney) {
    showPropertyCard(info);
    out_ << "Uang kamu saat ini: " << moneyFmt(playerMoney) << "\n";
    out_ << "Apakah kamu ingin membeli properti ini seharga " << moneyFmt(info.purchasePrice) << "? (y/n): ";
}

void CLIView::showRentPayment(const RentInfo& rentInfo) {
    if (rentInfo.property.status == PropertyStatus::MORTGAGED) {
        out_ << "Kamu mendarat di " << rentInfo.property.name
             << " (" << rentInfo.property.code << "), milik " << rentInfo.ownerName << ".\n";
        out_ << "Properti ini sedang digadaikan [M]. Tidak ada sewa yang dikenakan.\n";
        return;
    }

    out_ << "\nKamu mendarat di " << rentInfo.property.name
         << " (" << rentInfo.property.code << "), milik " << rentInfo.ownerName << "!\n";
    out_ << "Kondisi     : " << buildingStr(rentInfo.buildingLevel);
    if (rentInfo.buildingLevel == 0) out_ << "tanah kosong";
    out_ << "\n";
    out_ << "Sewa        : " << moneyFmt(rentInfo.amount);
    if (rentInfo.festivalActive) out_ << " (festival aktif)";
    out_ << "\n";
    out_ << "Uang kamu   : " << moneyFmt(rentInfo.payerBefore)
         << " -> " << moneyFmt(rentInfo.payerAfter) << "\n";
    out_ << "Uang " << rentInfo.ownerName << " : " << moneyFmt(rentInfo.ownerBefore)
         << " -> " << moneyFmt(rentInfo.ownerAfter) << "\n\n";
}

void CLIView::showTaxPrompt(const TaxInfo& taxInfo) {
    if (taxInfo.taxType == TileType::TAX_PBM) {
        out_ << "\nKamu mendarat di Pajak Barang Mewah (PBM)!\n";
        out_ << "Pajak sebesar " << moneyFmt(taxInfo.flatAmount) << " langsung dipotong.\n";
        return;
    }

    out_ << "\nKamu mendarat di Pajak Penghasilan (PPH)!\n";
    out_ << "Pilih opsi pembayaran pajak:\n";
    out_ << "1. Bayar flat " << moneyFmt(taxInfo.flatAmount) << "\n";
    out_ << "2. Bayar " << taxInfo.percentageRate << "% dari total kekayaan\n";
    out_ << "(Pilih sebelum menghitung kekayaan!)\n";
    out_ << "Pilihan (1/2): ";
}

void CLIView::showAuctionState(const AuctionState& state) {
    out_ << "\nProperti " << state.property.name << " (" << state.property.code << ") sedang dilelang!\n";
    if (state.currentBid > 0) {
        out_ << "Penawaran tertinggi: " << moneyFmt(state.currentBid)
             << " (" << state.highestBidderName << ")\n";
    } else {
        out_ << "Belum ada penawaran.\n";
    }
    out_ << "\nGiliran: " << state.currentBidderName << "\n";
    out_ << "Aksi (PASS / BID <jumlah>): ";
}

void CLIView::showAuctionWinner(const AuctionSummary& summary) {
    out_ << "\nLelang selesai!\n";
    if (summary.noWinner || summary.winnerName.empty()) {
        out_ << "Tidak ada pemenang. Properti dikembalikan ke Bank.\n";
    } else {
        out_ << "Pemenang: " << summary.winnerName << "\n";
        out_ << "Harga akhir: " << moneyFmt(summary.finalBid) << "\n";
        out_ << "Properti " << summary.property.name << " (" << summary.property.code
             << ") kini dimiliki " << summary.winnerName << ".\n";
    }
    out_ << "\n";
}

void CLIView::showFestivalPrompt(const vector<PropertyInfo>& ownedProperties) {
    out_ << "\nKamu mendarat di petak Festival!\n";
    if (ownedProperties.empty()) {
        out_ << "Kamu tidak memiliki properti. Festival tidak berlaku.\n";
        return;
    }
    out_ << "Daftar properti milikmu:\n";
    for (const auto& p : ownedProperties) {
        string festInfo;
        if (p.festivalMultiplier > 1) {
            festInfo = " [festival x" + to_string(p.festivalMultiplier)
                     + ", sisa " + to_string(p.festivalDurationLeft) + " giliran]";
        }
        out_ << "  - " << p.code << " (" << p.name << ")" << festInfo << "\n";
    }
    out_ << "Masukkan kode properti: ";
}

void CLIView::showFestivalReinforced(const FestivalEffectInfo& info) {
    out_ << "\nEfek diperkuat!\n";
    out_ << "Sewa sebelumnya: " << moneyFmt(info.previousMultiplier) << "x\n";
    out_ << "Sewa sekarang  : " << moneyFmt(info.newMultiplier) << "x\n";
    out_ << "Durasi di-reset menjadi: " << info.durationTurns << " giliran\n\n";
}

void CLIView::showFestivalAtMax(const FestivalEffectInfo& info) {
    out_ << "\nEfek sudah maksimum (harga sewa sudah digandakan tiga kali).\n";
    out_ << "Durasi di-reset menjadi: " << info.durationTurns << " giliran\n\n";
}

void CLIView::showJailEntry(const JailEntryInfo& info) {
    out_ << "\n";
    switch (info.reason) {
        case JailEntryReason::GO_TO_JAIL_TILE:
            out_ << info.playerName << " mendarat di Petak Pergi ke Penjara!\n";
            break;
        case JailEntryReason::TRIPLE_DOUBLE:
            out_ << info.playerName << " melanggar batas kecepatan (triple double)!\n";
            break;
        case JailEntryReason::CHANCE_CARD:
            out_ << info.playerName << " harus masuk Penjara (kartu kesempatan)!\n";
            break;
    }
    out_ << "Bidak " << info.playerName << " dipindahkan ke Petak Penjara.\n\n";
}

void CLIView::showJailStatus(const JailInfo& info) {
    out_ << "\n" << info.playerName << " sedang dalam penjara.\n";
    out_ << "Giliran di penjara: " << info.turnsInJail
         << " | Sisa percobaan keluar: " << info.escapeAttemptsLeft << "\n";
    out_ << "Pilihan:\n";
    out_ << "1. Bayar denda " << moneyFmt(info.fine) << " lalu lempar dadu\n";
    if (info.hasEscapeCard) out_ << "2. Gunakan kartu Bebas dari Penjara\n";
    out_ << "3. Coba lempar dadu (harus double)\n";
    if (info.escapeAttemptsLeft <= 1) {
        out_ << "[!] Ini giliran terakhir — wajib bayar denda jika tidak double.\n";
    }
}

void CLIView::showBankruptcy(const BankruptcyInfo& info) {
    out_ << "\n" << info.playerName << " dinyatakan BANGKRUT!\n";
    if (info.creditorIsBank) {
        out_ << "Kreditor: Bank\n";
        out_ << "Uang sisa " << moneyFmt(info.transferredCash) << " diserahkan ke Bank.\n";
        out_ << "Seluruh properti dikembalikan ke status BANK.\n";
    } else {
        out_ << "Kreditor: " << info.creditorName << "\n";
        out_ << "Pengalihan aset ke " << info.creditorName << ":\n";
        out_ << "  - Uang tunai sisa : " << moneyFmt(info.transferredCash) << "\n";
        for (const auto& p : info.transferredAssets) {
            string mortStr = (p.status == PropertyStatus::MORTGAGED) ? " MORTGAGED [M]" : "";
            out_ << "  - " << p.name << " (" << p.code << ")" << mortStr << "\n";
        }
        out_ << info.creditorName << " menerima semua aset " << info.playerName << ".\n";
    }
    out_ << info.playerName << " telah keluar dari permainan.\n\n";
}

void CLIView::showLiquidationPanel(const LiquidationState& state) {
    out_ << "\n=== Panel Likuidasi ===\n";
    out_ << "Uang kamu saat ini: " << moneyFmt(state.currentBalance)
         << "  |  Kewajiban: " << moneyFmt(state.obligation) << "\n\n";

    out_ << "[Jual ke Bank]\n";
    int n = 1;
    for (const auto& opt : state.options) {
        if (opt.type == LiquidationType::SELL) {
            out_ << n++ << ". " << opt.description << "\n";
        }
    }

    out_ << "\n[Gadaikan]\n";
    for (const auto& opt : state.options) {
        if (opt.type == LiquidationType::MORTGAGE) {
            out_ << n++ << ". " << opt.description << "\n";
        }
    }
    out_ << "\nPilih aksi (0 jika sudah cukup): ";
}

void CLIView::showLiquidationResult(bool canCover, Money finalBalance) {
    if (canCover) {
        out_ << "Dana likuidasi dapat menutup kewajiban.\n";
        out_ << "Uang kamu sekarang: " << moneyFmt(finalBalance) << "\n\n";
    } else {
        out_ << "Likuidasi tidak cukup untuk menutup kewajiban.\n\n";
    }
}

void CLIView::showBuildMenu(const BuildMenuState& state) {
    if (state.groups.empty()) {
        out_ << "Tidak ada color group yang memenuhi syarat untuk dibangun.\n";
        out_ << "Kamu harus memiliki seluruh petak dalam satu color group terlebih dahulu.\n";
        return;
    }

    out_ << "\n=== Color Group yang Memenuhi Syarat ===\n";
    out_ << "Uang kamu saat ini: " << moneyFmt(state.playerMoney) << "\n\n";
    int g = 1;
    for (const auto& grp : state.groups) {
        out_ << g++ << ". [" << grp.colorName << "]\n";
        for (const auto& t : grp.tiles) {
            string bld = (t.currentLevel == 5) ? "Hotel" :
                         (t.currentLevel == 0) ? "0 rumah" :
                         (to_string(t.currentLevel) + " rumah");
            string canStr = t.canBuild ? " <- dapat dibangun" : "";
            out_ << "   - " << t.name << " (" << t.code << ") : " << bld
                 << " (Harga: " << moneyFmt(t.buildCost) << ")" << canStr << "\n";
        }
    }
    out_ << "\nPilih nomor color group (0 untuk batal): ";
}

void CLIView::showMortgageMenu(const MortgageMenuState& state) {
    if (state.options.empty()) {
        out_ << "Tidak ada properti yang dapat digadaikan saat ini.\n";
        return;
    }

    out_ << "\n=== Properti yang Dapat Digadaikan ===\n";
    int n = 1;
    for (const auto& opt : state.options) {
        string warning = opt.requiresBuildingSale ? " [*bangunan harus dijual dulu]" : "";
        out_ << n++ << ". " << opt.name << " (" << opt.code << ")"
             << "  [" << colorName(opt.color) << "]"
             << "  Nilai Gadai: " << moneyFmt(opt.mortgageValue)
             << warning << "\n";
    }
    out_ << "\nPilih nomor properti (0 untuk batal): ";
}

void CLIView::showRedeemMenu(const RedeemMenuState& state) {
    if (state.options.empty()) {
        out_ << "Tidak ada properti yang sedang digadaikan.\n";
        return;
    }

    out_ << "\n=== Properti yang Sedang Digadaikan ===\n";
    out_ << "Uang kamu saat ini: " << moneyFmt(state.playerMoney) << "\n";
    int n = 1;
    for (const auto& opt : state.options) {
        string afford = opt.canAfford ? "" : " [uang tidak cukup]";
        out_ << n++ << ". " << opt.name << " (" << opt.code << ")"
             << "  [" << colorName(opt.color) << "]  [M]"
             << "  Harga Tebus: " << moneyFmt(opt.redeemCost)
             << afford << "\n";
    }
    out_ << "\nPilih nomor properti (0 untuk batal): ";
}

void CLIView::showCardDrawn(const CardInfo& cardInfo) {
    out_ << "\nMengambil kartu...\n";
    out_ << "Kartu: \"" << cardInfo.description << "\"\n\n";
}

void CLIView::showSkillCardHand(const vector<CardInfo>& cards) {
    out_ << "\nDaftar Kartu Kemampuan Spesial Anda:\n";
    int n = 1;
    for (const auto& c : cards) {
        string extra;
        if (c.type == "DiscountCard") {
            extra = " - Diskon " + to_string(c.value) + "%";
            if (c.durationLeft > 0) extra += " (sisa " + to_string(c.durationLeft) + " giliran)";
        } else if (c.type == "MoveCard") {
            extra = " - Maju " + to_string(c.value) + " petak";
        }
        out_ << n++ << ". " << c.name << extra << "\n";
    }
    out_ << "0. Batal\n";
}

void CLIView::showCardEffect(const CardEffectInfo& info) {
    if (!info.detail.empty()) {
        out_ << "[" << info.cardType << "] " << info.playerName << ": " << info.detail << "\n";
    }
}

void CLIView::showDropCardPrompt(const vector<CardInfo>& cards) {
    out_ << "\nPERINGATAN: Kamu sudah memiliki 3 kartu di tangan (Maksimal 3)!\n";
    out_ << "Kamu diwajibkan membuang 1 kartu.\n\n";
    out_ << "Daftar Kartu Kemampuan Anda:\n";
    int n = 1;
    for (const auto& c : cards) {
        out_ << n++ << ". " << c.name << " - " << c.description << "\n";
    }
    out_ << "Pilih nomor kartu yang ingin dibuang (1-" << cards.size() << "): ";
}

void CLIView::showTransactionLog(const vector<LogEntry>& entries) {
    if (entries.empty()) {
        out_ << "=== Log kosong ===\n";
        return;
    }
    out_ << "\n=== Log Transaksi ===\n";
    for (const auto& e : entries) {
        out_ << "[Turn " << e.turn << "] "
             << pad(e.username, 8) << " | "
             << pad(e.actionType, 8) << " | "
             << e.detail << "\n";
    }
    out_ << "\n";
}

void CLIView::showWinner(const WinnerInfo& winInfo) {
    out_ << "\n====================================\n";
    out_ << "       PERMAINAN SELESAI!           \n";
    out_ << "====================================\n";
    if (!winInfo.winCondition.empty()) {
        out_ << "Kondisi: " << winInfo.winCondition << "\n\n";
    }

    if (!winInfo.players.empty()) {
        out_ << "Rekap pemain:\n";
        for (const auto& p : winInfo.players) {
            out_ << "\n" << p.username << "\n";
            out_ << "  Uang     : " << moneyFmt(p.money) << "\n";
            out_ << "  Properti : " << p.propertyCount << "\n";
            out_ << "  Kartu    : " << p.cardCount << "\n";
            if (p.isBankrupt) out_ << "  (BANGKRUT)\n";
        }
        out_ << "\n";
    }

    if (winInfo.winners.size() == 1) {
        out_ << "Pemenang: " << winInfo.winners[0] << "\n";
    } else {
        out_ << "Pemenang: ";
        for (size_t i = 0; i < winInfo.winners.size(); ++i) {
            if (i > 0) out_ << ", ";
            out_ << winInfo.winners[i];
        }
        out_ << " (SERI)\n";
    }
    out_ << "====================================\n\n";
}

void CLIView::showSaveLoadStatus(const string& message) {
    out_ << message << "\n";
}

void CLIView::showMessage(const string& message) {
    out_ << message << "\n";
}
