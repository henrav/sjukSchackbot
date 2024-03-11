//
// Created by Henrik Ravnborg on 2024-03-09.
//
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#ifndef UNTITLED7_GENERATEBOARD_H
#define UNTITLED7_GENERATEBOARD_H


class generateBoard {
public:
    void run() {
        sf::RenderWindow window(sf::VideoMode(800, 800), "Chess Game");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            window.clear();
            drawBoard(window);
            window.display();
        }
    }
private:
    void drawBoard(sf::RenderWindow &window) {
        int tileSize = 100; // Assuming an 800x800 window, this makes each tile 100x100 pixels.
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
                tile.setPosition(x * tileSize, y * tileSize);
                tile.setFillColor((x + y) % 2 == 0 ? sf::Color::White : sf::Color::Black);
                window.draw(tile);
            }
        }
    }

};


#endif //UNTITLED7_GENERATEBOARD_H
