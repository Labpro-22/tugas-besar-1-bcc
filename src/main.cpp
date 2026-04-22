#include <cstdlib>

#if NIMONSPOLY_ENABLE_SFML
#include "core/state/header/GameStateView.hpp"
#include "ui/GUIInput.hpp"
#include "ui/GUIView.hpp"

#include <SFML/Graphics.hpp>

#include <optional>
#endif

int main() {
#if NIMONSPOLY_ENABLE_SFML
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{1024u, 768u}), "NIMONSPOLY");
    window.setFramerateLimit(60);

    GUIView view(window);
    GUIInput input(window);
    GameStateView demoState;

    // Temporary GUI smoke test loop. GameEngine integration comes later.
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        view.showBoard(demoState);
    }

    return EXIT_SUCCESS;
#else
    // Foundation build check: the executable should link even before game logic is implemented.
    return EXIT_SUCCESS;
#endif
}
