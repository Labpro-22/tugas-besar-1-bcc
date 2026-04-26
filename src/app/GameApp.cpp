#include "app/GameApp.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "core/TextFileRepository.hpp"
#include "core/engine/header/GameEngine.hpp"
#include "core/state/header/GameStateView.hpp"
#include "controllers/ComputerController.hpp"
#include "controllers/HumanController.hpp"
#include "models/Money.hpp"
#include "models/Player.hpp"
#include "tile/header/StreetTile.hpp"
#include "ui/AppScreen.hpp"
#include "ui/AssetManager.hpp"
#include "ui/GUIInput.hpp"
#include "ui/GUIView.hpp"
#include "ui/RaylibCompat.hpp"
#include "../views/gui/GuiMenuLayout.hpp"

int GameApp::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1440, 1024, "NIMONSPOLY");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    AssetManager::get().loadAll();

    GameEngine engine;
    TextFileRepository repo;
    engine.setRepository(&repo);
    engine.loadConfiguration("config");

    GUIView view;
    GUIInput input;
    GameStateView state(engine.getState());

    bool gameInitialized = false;
    std::vector<std::unique_ptr<Player>> ownedPlayers;
    std::vector<std::unique_ptr<PlayerController>> ownedControllers;
    std::vector<Player*> players;
    std::string pendingCommand;
    bool mouseClickHandled = false;

    auto renderFrame = [&]() {
        BeginDrawing();
        if (view.screen() == AppScreen::IN_GAME && gameInitialized) {
            state.refresh(engine.getState(), &engine.getBoard());
            view.showTransactionLog(engine.getTransactionLogger().getFullLog());
            view.setCurrentPrompt(&input.currentPrompt());
            view.showBoard(state);
        } else {
            view.renderCurrentScreen();
        }
        EndDrawing();
    };

    input.setRenderCallback(renderFrame);

    auto startNewGame = [&]() {
        ownedPlayers.clear();
        ownedControllers.clear();
        players.clear();
        pendingCommand.clear();
        gui::menu::finalizeSetupPlayers(view.setup());

        engine.loadConfiguration("config", view.setup().numTiles);

        ownedPlayers.reserve(static_cast<size_t>(view.setup().numPlayers));
        ownedControllers.reserve(static_cast<size_t>(view.setup().numPlayers));
        players.reserve(static_cast<size_t>(view.setup().numPlayers));

        for (int i = 0; i < view.setup().numPlayers; ++i) {
            std::string name =
                (i < static_cast<int>(view.setup().playerNames.size()) &&
                 !view.setup().playerNames[static_cast<size_t>(i)].empty())
                    ? view.setup().playerNames[static_cast<size_t>(i)]
                    : "P" + std::to_string(i + 1);

            if (i >= static_cast<int>(view.setup().playerNames.size())) {
                view.setup().playerNames.resize(static_cast<size_t>(i) + 1);
            }
            view.setup().playerNames[static_cast<size_t>(i)] = name;

            const bool isComputer =
                i < static_cast<int>(view.setup().isComputer.size()) &&
                view.setup().isComputer[static_cast<size_t>(i)];

            std::unique_ptr<PlayerController> controller;
            if (isComputer) {
                controller = std::make_unique<ComputerController>(name);
            } else {
                controller = std::make_unique<HumanController>(&input, &view, name);
            }

            ownedPlayers.push_back(std::make_unique<Player>(
                name, Money(engine.getConfig().getStartingMoney()), controller.get()));
            const int colorIdx = (i < static_cast<int>(view.setup().playerColors.size()))
                                     ? view.setup().playerColors[static_cast<size_t>(i)]
                                     : i % 4;
            ownedPlayers.back()->setColorIndex(colorIdx);
            ownedControllers.push_back(std::move(controller));
            players.push_back(ownedPlayers.back().get());
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(players.begin(), players.end(), g);

        engine.initialize(players, engine.getConfig().getMaxTurns());
        engine.start();
        gameInitialized = true;
        view.startGameTimer();
    };

    auto loadGameFromFile = [&](const std::string& path) {
        ownedPlayers.clear();
        ownedControllers.clear();
        players.clear();
        pendingCommand.clear();

        if (!repo.exists(path)) {
            return false;
        }

        const std::vector<std::string> names = repo.getPlayerNames(path);
        if (names.empty()) {
            return false;
        }

        ownedPlayers.reserve(names.size());
        ownedControllers.reserve(names.size());
        players.reserve(names.size());
        view.setup().numPlayers = static_cast<int>(names.size());
        view.setup().playerNames.clear();
        view.setup().playerNames.reserve(names.size());
        view.setup().isComputer.assign(names.size(), false);
        view.setup().playerColors.clear();
        view.setup().playerCharacters.clear();

        for (const std::string& name : names) {
            const std::string useName =
                name.empty() ? "P" + std::to_string(players.size() + 1) : name;
            auto controller = std::make_unique<HumanController>(&input, &view, useName);
            ownedPlayers.push_back(std::make_unique<Player>(
                useName, Money(engine.getConfig().getStartingMoney()), controller.get()));
            view.setup().playerNames.push_back(useName);
            view.setup().playerColors.push_back(static_cast<int>(players.size()) % 4);
            view.setup().playerCharacters.push_back(static_cast<int>(players.size()) % 4);
            ownedPlayers.back()->setColorIndex(view.setup().playerColors.back());
            ownedControllers.push_back(std::move(controller));
            players.push_back(ownedPlayers.back().get());
        }

        engine.initialize(players, engine.getConfig().getMaxTurns());
        engine.start();
        if (!engine.loadGame(path)) {
            return false;
        }

        gameInitialized = true;
        view.startGameTimer();
        return true;
    };

    while (!WindowShouldClose()) {
        if (view.screen() != AppScreen::IN_GAME) {
            const bool startSignal = view.handleMenuInput();
            if (startSignal && view.screen() == AppScreen::IN_GAME) {
                gameInitialized = false;
                if (!view.setup().loadFilePath.empty()) {
                    if (!loadGameFromFile(view.setup().loadFilePath)) {
                        view.setup().loadFilePath.clear();
                        view.setup() = SetupState{};
                        view.setScreen(AppScreen::LANDING);
                    }
                } else {
                    startNewGame();
                }
            }
        } else if (gameInitialized) {
            if (input.currentPrompt().type != GUIPromptType::NONE &&
                !input.currentPrompt().resolved) {
                input.updatePrompt();
            } else if (!mouseClickHandled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pendingCommand.empty()) {
                const Vector2 mouse = GetMousePosition();
                view.handleInGameClick(mouse.x, mouse.y, pendingCommand, state);
                mouseClickHandled = true;
            }
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                mouseClickHandled = false;
            }
        }

        if (view.screen() == AppScreen::IN_GAME && gameInitialized && engine.isRunning()) {
            Player* active = engine.getState().getActivePlayer();

            const std::string winCondition = engine.getGameOverReason();
            if (!winCondition.empty()) {
                engine.stop();
                view.showWinner(engine.getWinnerInfo(winCondition));
                view.setScreen(AppScreen::GAME_OVER);
                state.refresh(engine.getState(), &engine.getBoard());
                view.showBoard(state);
                renderFrame();
                continue;
            }

            if (!active) {
                engine.stop();
            } else {
                engine.ensureTurnPrepared(*active);
                if (active->isBankrupt()) {
                    engine.processCommand("SELESAI", *active);
                } else if (!pendingCommand.empty()) {
                if (pendingCommand == "DADU" && view.isDiceAnimating()) {
                    // Wait until the overlay animation finishes.
                } else if (pendingCommand == "ATUR_DADU") {
                    const auto [d1, d2] = input.getManualDice();
                    if (d1 > 0 && d2 > 0) {
                        engine.processCommand(
                            "atur dadu " + std::to_string(d1) + " " + std::to_string(d2),
                            *active);
                        view.setQueuedManualDice(d1, d2);
                        view.showSaveLoadStatus(
                            "Roll berikutnya diatur ke " + std::to_string(d1) + " + " + std::to_string(d2) + ".");
                    } else {
                        view.showSaveLoadStatus("Atur dadu dibatalkan.");
                    }
                    pendingCommand.clear();
                } else if (pendingCommand == "SIMPAN") {
                    std::string path = input.getString("Nama berkas simpanan");
                    if (path.empty()) {
                        path = "nimonspoli_save.txt";
                    }
                    if (!engine.getState().canSave()) {
                        view.showSaveLoadStatus("Permainan belum bisa disimpan.");
                    } else if (engine.saveGame(path)) {
                        view.showSaveLoadStatus("Permainan disimpan ke " + path + ".");
                    } else {
                        view.showSaveLoadStatus("Gagal menyimpan.");
                    }
                    pendingCommand.clear();
                } else if (pendingCommand == "DADU") {
                    engine.processCommand("lempar", *active);
                    view.showDiceResult(engine.getDice().getDie1(), engine.getDice().getDie2(), active->getUsername());
                    view.clearQueuedManualDice();
                    pendingCommand.clear();
                } else if (pendingCommand == "TEBUS") {
                    std::string code = input.getPropertyCodeInput("Kode properti untuk tebus");
                    if (!code.empty()) {
                        engine.processCommand("tebus " + code, *active);
                    }
                    pendingCommand.clear();
                } else if (pendingCommand == "BANGUN") {
                    bool hasCompleteGroup = false;
                    for (PropertyTile* prop : active->getProperties()) {
                        if (auto* street = dynamic_cast<StreetTile*>(prop)) {
                            if (street->isMonopolyComplete()) {
                                hasCompleteGroup = true;
                                break;
                            }
                        }
                    }
                    if (!hasCompleteGroup) {
                        engine.getTransactionLogger().log(
                            engine.getState().getCurrentTurn(),
                            active->getUsername(),
                            "BANGUN",
                            "Tidak ada color group lengkap. Tidak bisa bangun."
                        );
                        view.showSaveLoadStatus("Tidak ada color group lengkap. Tidak bisa bangun.");
                        pendingCommand.clear();
                        continue;
                    }

                    BuildMenuState menu = engine.getBoard().getBuildMenuState(*active);
                    if (menu.groups.empty()) {
                        engine.getTransactionLogger().log(
                            engine.getState().getCurrentTurn(),
                            active->getUsername(),
                            "BANGUN",
                            "Tidak ada properti yang memenuhi syarat untuk dibangun."
                        );
                        view.showSaveLoadStatus("Tidak ada properti yang memenuhi syarat untuk dibangun.");
                        pendingCommand.clear();
                        continue;
                    }

                    std::vector<std::string> groupOptions;
                    for (const ColorGroupBuildOption& group : menu.groups) {
                        std::string line = "[" + group.colorName + "] ";
                        for (size_t i = 0; i < group.tiles.size(); ++i) {
                            if (i > 0) line += " | ";
                            const TileBuildOption& tile = group.tiles[i];
                            line += tile.name + " (" + tile.code + "): " +
                                    std::to_string(tile.currentLevel) +
                                    (tile.currentLevel >= 4 ? " hotel" : " rumah") +
                                    " (Harga: M" + std::to_string(tile.buildCost) + ")";
                        }
                        groupOptions.push_back(line);
                    }
                    groupOptions.push_back("Uang kamu saat ini: " + active->getMoney().toString());
                    const int groupChoice = input.getMenuChoice(groupOptions);
                    if (groupChoice <= 0 || groupChoice > static_cast<int>(menu.groups.size())) {
                        pendingCommand.clear();
                        continue;
                    }

                    const ColorGroupBuildOption& selectedGroup = menu.groups[static_cast<size_t>(groupChoice - 1)];
                    if (selectedGroup.tiles.empty()) {
                        pendingCommand.clear();
                        continue;
                    }
                    std::vector<std::string> streetOptions;
                    for (const TileBuildOption& tile : selectedGroup.tiles) {
                        streetOptions.push_back(tile.name + " (" + tile.code + ")" +
                                              ": " + std::to_string(tile.currentLevel) +
                                              (tile.currentLevel >= 4 ? " hotel" : " rumah") +
                                              " <- dapat dibangun");
                    }

                    const int streetChoice = input.getMenuChoice(streetOptions);
                    if (streetChoice > 0 && streetChoice <= static_cast<int>(selectedGroup.tiles.size())) {
                        engine.processCommand("bangun " + selectedGroup.tiles[static_cast<size_t>(streetChoice - 1)].code, *active);
                    }
                    pendingCommand.clear();
                } else if (pendingCommand == "GADAI") {
                    std::string code = input.getPropertyCodeInput("Kode properti untuk gadai");
                    if (!code.empty()) {
                        engine.processCommand("gadai " + code, *active);
                    }
                    pendingCommand.clear();
                } else if (pendingCommand == "KARTU") {
                    const auto& skillCards = active->getSkillCards();
                    if (!skillCards.empty()) {
                        std::vector<CardInfo> cardInfos;
                        cardInfos.reserve(skillCards.size());
                        for (const SkillCard* card : skillCards) {
                            cardInfos.push_back(CardInfo{card->getName(), card->getDescription(), card->getType()});
                        }

                        const int choice = input.getSkillCardChoice(cardInfos);
                        if (choice > 0) {
                            engine.processCommand("gunakan kemampuan " + std::to_string(choice), *active);
                        }
                    } else {
                        view.showSaveLoadStatus("Tidak ada kartu kemampuan.");
                    }
                    pendingCommand.clear();
                } else {
                    engine.processCommand(pendingCommand, *active);
                    pendingCommand.clear();
                }
                } else if (dynamic_cast<ComputerController*>(active->getController()) != nullptr) {
                    const std::string command = active->getController()->chooseCommand(engine.getState().toView());
                    if (!command.empty()) {
                        engine.processCommand(command, *active);
                    }
                }
            }
        }

        renderFrame();
    }

    AssetManager::get().unloadAll();
    CloseWindow();
    return EXIT_SUCCESS;
}
