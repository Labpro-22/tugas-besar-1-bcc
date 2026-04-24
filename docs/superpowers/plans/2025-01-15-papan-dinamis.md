# Papan Dinamis Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implementasi bonus Papan Dinamis — board bisa dikonfigurasi 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, atau 60 tile.

**Architecture:** Config-driven board builder. Setiap ukuran punya folder `config/{N}/` berisi `property.txt`, `aksi.txt`, dan `board.txt`.

**Commit Strategy:** Per 3 config folders, commit singkat.

---

## Task 1: Configs 20-28

**Files:**
- Create: `config/20/property.txt`, `config/20/aksi.txt`, `config/20/board.txt`
- Create: `config/24/property.txt`, `config/24/aksi.txt`, `config/24/board.txt`
- Create: `config/28/property.txt`, `config/28/aksi.txt`, `config/28/board.txt`

- [ ] **Step 1:** Buat `config/20/` — 12 properti, 10 aksi, layout 20 tile
- [ ] **Step 2:** Buat `config/24/` — 14 properti, 11 aksi, layout 24 tile
- [ ] **Step 3:** Buat `config/28/` — 16 properti, 12 aksi, layout 28 tile
- [ ] **Step 4:** Commit

```bash
git add config/20/ config/24/ config/28/
git commit -m "feat: board configs 20-28"
```

---

## Task 2: Configs 32-40

**Files:**
- Create: `config/32/`, `config/36/`, `config/40/`

- [ ] **Step 1:** Buat `config/32/` — 18 properti, layout 32 tile
- [ ] **Step 2:** Buat `config/36/` — 20 properti, layout 36 tile
- [ ] **Step 3:** Buat `config/40/` — 28 properti, layout 40 tile (standar)
- [ ] **Step 4:** Commit

```bash
git add config/32/ config/36/ config/40/
git commit -m "feat: board configs 32-40"
```

---

## Task 3: Configs 44-52

**Files:**
- Create: `config/44/`, `config/48/`, `config/52/`

- [ ] **Step 1-3:** Buat folders dengan properti extended
- [ ] **Step 4:** Commit

```bash
git add config/44/ config/48/ config/52/
git commit -m "feat: board configs 44-52"
```

---

## Task 4: Configs 56-60

**Files:**
- Create: `config/56/`, `config/60/`

- [ ] **Step 1-2:** Buat folders
- [ ] **Step 3:** Commit

```bash
git add config/56/ config/60/
git commit -m "feat: board configs 56-60"
```

---

## Task 5: Refactor GameEngine

**Files:**
- Modify: `include/core/engine/header/GameEngine.hpp`
- Modify: `src/core/engine/GameEngine.cpp`

- [ ] **Step 1:** Tambah `numTiles` parameter ke `loadConfiguration` dan `buildDefaultBoard`
- [ ] **Step 2:** Implement `buildDefaultBoard` config-driven (parse board.txt, property.txt, aksi.txt)
- [ ] **Step 3:** Hapus hardcoded switch 40-tile lama
- [ ] **Step 4:** Commit

```bash
git add include/core/engine/header/GameEngine.hpp src/core/engine/GameEngine.cpp
git commit -m "feat: dynamic board builder"
```

---

## Task 6: Remove BOARD_SIZE

**Files:**
- Modify: `include/models/board/header/Board.hpp`

- [ ] **Step 1:** Hapus `constexpr int BOARD_SIZE = 40`
- [ ] **Step 2:** Fix references ke `Board::BOARD_SIZE` di codebase
- [ ] **Step 3:** Commit

```bash
git add include/models/board/header/Board.hpp
git commit -m "refactor: remove board size const"
```

---

## Task 7: Wire GUI

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1:** Pass `setup_.numTiles` ke `engine.loadConfiguration`
- [ ] **Step 2:** Commit

```bash
git add src/main.cpp
git commit -m "feat: wire board size selector"
```

---

## Task 8: Save/Load numTiles

**Files:**
- Modify: `include/core/TextFileRepository.hpp`
- Modify: `src/core/TextFileRepository.cpp`

- [ ] **Step 1:** Tambah `numTiles` ke format save
- [ ] **Step 2:** Parse `numTiles` saat load, reconstruct board
- [ ] **Step 3:** Commit

```bash
git add include/core/TextFileRepository.hpp src/core/TextFileRepository.cpp
git commit -m "feat: save board size"
```

---

## Task 9: Build & Test

- [ ] **Step 1:** Build: `cmake --build build`
- [ ] **Step 2:** Test run dengan berbagai ukuran board
- [ ] **Step 3:** Commit

```bash
git add .
git commit -m "test: verify dynamic board"
```
