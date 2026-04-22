#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#if NIMONSPOLY_ENABLE_SFML
#include "ui/GUIInput.hpp"
#include "ui/GUIView.hpp"

#include <SFML/Graphics.hpp>

#include <optional>
#else
#include "controllers/HumanController.hpp"
#include "core/engine/header/GameEngine.hpp"
#include "models/Player.hpp"
#include "ui/ConsoleInput.hpp"
#endif

int main() {
#if NIMONSPOLY_ENABLE_SFML
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{1024u, 768u}), "NIMONSPOLY");
    window.setFramerateLimit(60);

    GUIView view(window);
    GUIInput input(window);

    // Temporary GUI smoke test loop. GameEngine integration comes later.
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color(30, 30, 30));
        window.display();
    }

    return EXIT_SUCCESS;
#else
    ConsoleInput input;
    HumanController controller(&input);
    GameEngine engine;
    engine.loadConfiguration("config");

    const int playerCount = input.getNumberInRange("Jumlah pemain", 2, 4);
    std::vector<std::unique_ptr<Player>> ownedPlayers;
    std::vector<Player*> players;
    ownedPlayers.reserve(playerCount);
    players.reserve(playerCount);

    for (int i = 0; i < playerCount; ++i) {
        const std::string name = input.getString("Nama pemain " + std::to_string(i + 1));
        ownedPlayers.push_back(std::make_unique<Player>(name.empty() ? "P" + std::to_string(i + 1) : name,
                                                        Money(engine.getConfig().getStartingMoney()),
                                                        &controller));
        players.push_back(ownedPlayers.back().get());
    }

    engine.initialize(players, engine.getConfig().getMaxTurns());
    engine.start();
    std::cout << "NIMONSPOLI dimulai. Command utama: CETAK PAPAN, LEMPAR DADU, ATUR DADU x y, "
                 "CETAK PROPERTI, CETAK AKTA KODE, GADAI KODE, TEBUS KODE, BANGUN KODE, "
                 "GUNAKAN KEMAMPUAN nomor, SELESAI.\n";

    while (engine.isRunning()) {
        if (engine.getState().getCurrentTurn() > engine.getState().getMaxTurn()) {
            engine.stop();
            break;
        }

        int activeCount = 0;
        Player* winner = nullptr;
        for (Player* player : players) {
            if (player && !player->isBankrupt()) {
                ++activeCount;
                winner = player;
            }
        }
        if (activeCount <= 1) {
            if (winner) {
                std::cout << "Pemenang: " << winner->getUsername() << "\n";
            }
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

    return EXIT_SUCCESS;
#endif
}
