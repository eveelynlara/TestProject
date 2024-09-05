#pragma once
#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"

class Editor {
public:
    Editor();
    void run();

private:
    sf::RenderWindow window;
    EntityManager entityManager;
    
    sf::RectangleShape projectArea;
    sf::RectangleShape editArea;
    
    std::vector<sf::RectangleShape> entityThumbnails;
    std::vector<sf::RectangleShape> tileThumbnails;
    
    Entity* selectedEntity;
    sf::IntRect selectedTile;
    
    void handleEvents();
    void update();
    void render();
    void loadEntities();
    void createEntityThumbnails();
    void createTileThumbnails();
    void handleMouseClick(sf::Vector2i mousePos);
    void placeTile(sf::Vector2i mousePos);
};