# Spec: Papan Dinamis

**Goal:** Board dapat dikonfigurasi dengan jumlah tile 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, atau 60 melalui folder config per ukuran.

**Status:** Approved

---

## 1. Struktur Config Folder

```
config/
├── misc.txt              # global: MAX_TURN, SALDO_AWAL
├── tax.txt               # global: PPH_FLAT, PPH_PERSENTASE, PBM_FLAT
├── special.txt           # global: GO_SALARY, JAIL_FINE
├── railroad.txt          # global: tabel sewa railroad
├── utility.txt           # global: tabel faktor utility
├── 20/
│   ├── property.txt      # properti khusus board 20
│   ├── aksi.txt          # aksi/spesial khusus board 20
│   └── board.txt         # urutan layout tile (hanya KODE)
├── 24/
│   ├── property.txt
│   ├── aksi.txt
│   └── board.txt
└── ... (28, 32, 36, 40, 44, 48, 52, 56, 60)
```

## 2. Format File

### property.txt (per ukuran)
Format sama dengan `config/property.txt` master:
```
ID KODE NAMA JENIS WARNA HARGA_LAHAN NILAI_GADAI UPG_RUMAH UPG_HT RENT_L0 RENT_L1 RENT_L2 RENT_L3 RENT_L4 RENT_L5
2 GRT GARUT STREET COKLAT 60 40 20 50 2 10 30 90 160 250
...
```

### aksi.txt (per ukuran)
```
ID KODE NAMA JENIS_PETAK WARNA
1 GO Petak_Mulai SPESIAL DEFAULT
3 DNU Dana_Umum KARTU DEFAULT
...
```

Jenis yang didukung: `go`, `jail`, `free_parking`, `go_to_jail`, `chance`, `community`, `festival`, `tax_pph`, `tax_pbm`.

### board.txt (per ukuran)
Hanya urutan KODE tile, satu per baris:
```
GO
GRT
DNU
TSK
PPH
...
```

## 3. Perubahan di Kode

### GameEngine
- `loadConfiguration(directory, numTiles)` — menerima ukuran board
- `buildDefaultBoard(configDirectory, numTiles)` — baca dari `config/{numTiles}/`
- Hapus hardcoded switch 40-tile

### Board
- Hapus `BOARD_SIZE` constexpr
- `getSize()` return `tiles.size()`

### Save/Load
- `TextFileRepository` simpan `numTiles` di header save file
- Saat load, reconstruct board dengan ukuran yang sama

### GUI
- `main.cpp` pass `setup_.numTiles` ke `engine.loadConfiguration()`

## 4. Validasi

- Jumlah baris `board.txt` harus = `numTiles`
- Setiap KODE di `board.txt` harus ada di `property.txt` atau `aksi.txt`
- Minimal ada tile `go`, `jail`, `free_parking`, `go_to_jail`

## 5. Backward Compatibility

- Folder `config/40/` berisi board standar
- Kalau folder `{numTiles}/` tidak ada → throw `InvalidBoardConfigException`
