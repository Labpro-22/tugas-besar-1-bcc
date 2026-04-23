#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

#if NIMONSPOLY_ENABLE_SFML
#include "core/state/header/GameStateView.hpp"
#include "ui/AppScreen.hpp"
#include "ui/AssetManager.hpp"
#include "ui/GUIInput.hpp"
#include "ui/GUIView.hpp"

#include <SFML/Graphics.hpp>

#include <optional>
#else
#include "controllers/HumanController.hpp"
#include "core/TextFileRepository.hpp"
#include "core/engine/header/GameEngine.hpp"
#include "models/Money.hpp"
#include "models/Player.hpp"
#include "ui/ConsoleInput.hpp"
#endif

int main() {
#if NIMONSPOLY_ENABLE_SFML
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{1440u, 1024u}), "NIMONSPOLY");
    window.setFramerateLimit(60);

    AssetManager::get().loadAll();

    GUIView view(window);
    GUIInput input(window);
    GameStateView demoState;

    input.setRenderCallback([&]() {
        if (view.screen() == AppScreen::IN_GAME)
            view.showBoard(demoState);
        else
            view.renderCurrentScreen();
    });

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }

            if (view.screen() != AppScreen::IN_GAME) {
                view.handleMenuEvent(*event);
            } else {
                input.handleEvent(*event);
            }
        }

        if (!window.isOpen()) break;

        if (view.screen() == AppScreen::IN_GAME)
            view.showBoard(demoState);
        else
            view.renderCurrentScreen();
    }

    return EXIT_SUCCESS;
#else
    ConsoleInput input;
    HumanController controller(&input);
    GameEngine engine;
    TextFileRepository repo;
    engine.setRepository(&repo);
    engine.loadConfiguration("config");

    std::cout << "1. Permainan baru\n2. Muat simpanan\n";
    const int menu = input.getNumberInRange("Pilihan", 1, 2);

    std::vector<std::unique_ptr<Player>> ownedPlayers;
    std::vector<Player*> players;
    std::string loadPath;

    if (menu == 2) {
        loadPath = input.getString("Nama berkas simpanan");
        if (loadPath.empty()) {
            loadPath = "nimonspoli_save.txt";
        }
        if (!repo.exists(loadPath)) {
            std::cout << "Berkas tidak ditemukan.\n";
            return EXIT_FAILURE;
        }
        const std::vector<std::string> names = repo.getPlayerNames(loadPath);
        if (names.empty()) {
            std::cout << "Simpanan tidak valid.\n";
            return EXIT_FAILURE;
        }
        ownedPlayers.reserve(names.size());
        players.reserve(names.size());
        for (const std::string& name : names) {
            const std::string useName = name.empty() ? "P" + std::to_string(players.size() + 1) : name;
            ownedPlayers.push_back(std::make_unique<Player>(useName, Money(engine.getConfig().getStartingMoney()),
                &controller));
            players.push_back(ownedPlayers.back().get());
        }
        engine.initialize(players, engine.getConfig().getMaxTurns());
        engine.start();
        if (!engine.loadGame(loadPath)) {
            std::cout << "Gagal memuat simpanan.\n";
            return EXIT_FAILURE;
        }
    } else {
        const int playerCount = input.getNumberInRange("Jumlah pemain", 2, 4);
        ownedPlayers.reserve(playerCount);
        players.reserve(playerCount);

        for (int i = 0; i < playerCount; ++i) {
            const std::string name = input.getString("Nama pemain " + std::to_string(i + 1));
            ownedPlayers.push_back(std::make_unique<Player>(name.empty() ? "P" + std::to_string(i + 1) : name,
                                                            Money(engine.getConfig().getStartingMoney()),
                                                            &controller));
            players.push_back(ownedPlayers.back().get());
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(players.begin(), players.end(), g);

        engine.initialize(players, engine.getConfig().getMaxTurns());
        engine.start();
    }

    while (engine.isRunning()) {
        if (engine.getState().getCurrentTurn() > engine.getState().getMaxTurn()) {
            engine.stop();
            break;
        }

        int activeCount = 0;
        for (Player* player : players) {
            if (player && !player->isBankrupt()) {
                ++activeCount;
            }
        }
        if (activeCount <= 1) {
            engine.stop();
            break;
        }

        Player* active = engine.getState().getActivePlayer();
        if (!active) {
            engine.stop();
            break;
        }
        if (active->isBankrupt()) {
            engine.processCommand("SELESAI", *active);
            continue;
        }

        std::cout << "\nTurn " << engine.getState().getCurrentTurn() << "/"
                  << engine.getState().getMaxTurn() << " - " << active->getUsername()
                  << " (" << active->getMoney().toString() << ")\n";
        const std::string command = input.getCommand();
        if (command == "quit" || command == "QUIT" || command == "keluar") {
            engine.stop();
            break;
        }
        engine.processCommand(command, *active);
    }

    std::vector<Player*> survivors;
    for (Player* p : players) {
        if (p && !p->isBankrupt()) {
            survivors.push_back(p);
        }
    }
    std::sort(survivors.begin(), survivors.end(), [](Player* a, Player* b) {
        if (!a || !b) {
            return a != nullptr;
        }
        const Money wa = a->getTotalWealth();
        const Money wb = b->getTotalWealth();
        if (wa.getAmount() != wb.getAmount()) {
            return wa.getAmount() > wb.getAmount();
        }
        return a->getUsername() < b->getUsername();
    });

    if (survivors.size() == 1) {
        std::cout << "Pemenang: " << survivors.front()->getUsername() << "\n";
    } else if (!survivors.empty()) {
        std::cout << "Permainan berakhir. Peringkat:\n";
        int r = 1;
        for (Player* p : survivors) {
            if (!p) continue;
            std::cout << r++ << ". " << p->getUsername() << " — " << p->getTotalWealth().toString() << "\n";
        }
    }

    return EXIT_SUCCESS;
#endif
}
