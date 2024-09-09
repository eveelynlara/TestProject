#pragma once
#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"
#include <vector>
#include <string>

class Editor {
public:
    Editor();
    void run();

private:
    sf::RenderWindow window;
    EntityManager entityManager;
    
    sf::RectangleShape projectArea;
    sf::RectangleShape editArea;
    sf::RectangleShape sidebarArea;  // Nova área para a lista de entidades
    
    std::vector<sf::RectangleShape> entityThumbnails;
    std::vector<sf::RectangleShape> tileThumbnails;
    std::vector<sf::RectangleShape> placedTiles;
    std::vector<std::unique_ptr<Entity>> placedEntities;
    
    Entity* selectedEntity;
    sf::IntRect selectedTile;
    
    bool isFloatingWindowOpen;
    sf::Vector2f floatingWindowPosition;
    sf::Font font;

    int selectedTileIndex = -1;
    int selectedEntityIndex = -1;  // Índice da entidade selecionada na lista lateral

    // Grade
    int gridSize;
    std::vector<sf::Vertex> gridLines;
    
    void placeEntity(sf::Vector2i mousePos);
    void handleEvents();
    void update();
    void render();
    void loadEntities();
    void createEntityThumbnails();
    void createTileThumbnails();
    void handleMouseClick(sf::Vector2i mousePos);
    void placeTile(sf::Vector2i mousePos);
    void handleFloatingWindowClick(sf::Vector2f relativePos);
    void drawFloatingWindow();
    void saveScene(const std::string& filename);
    void createGrid();

    // Novas funções
    void renderSidebar();
    void handleKeyPress(sf::Keyboard::Key key);
    void selectEntity(int index);
    void showEntityDetails();
};