#pragma once
#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"
#include <tinyxml2.h>
#include <vector>
#include <string>
#include <map>

struct FileNode {
    std::string name;
    bool isDirectory;
    bool isOpen;
    std::vector<FileNode> children;
};

class Editor {
    sf::Vector2f currentGridSize;
    bool showGrid;

public:
    Editor();
    void run();
    void exportScene(const std::string& filename);

private:
    sf::RenderWindow window;
    EntityManager entityManager;
    
    sf::RectangleShape projectArea;
    sf::RectangleShape editArea;
    sf::RectangleShape sidebarArea;
    sf::Sprite entityPreview;

    std::vector<sf::RectangleShape> tileThumbnails;
    std::vector<sf::RectangleShape> placedTiles;
    std::vector<std::unique_ptr<Entity>> placedEntities;

    sf::Font menuFont;
    std::vector<sf::Text> menuItems;
    bool isMenuOpen;
    std::string saveFilePath;
    
    Entity* selectedEntity;
    int selectedTileIndex = -1;
    
    bool isFloatingWindowOpen;
    sf::Vector2f floatingWindowPosition;
    sf::Font font;

    int selectedEntityIndex = -1;

    // Grade
    int gridSize;
    std::vector<sf::Vertex> gridLines;
    
    // Estrutura de arquivos
    FileNode rootNode;
    int selectedNodeIndex;
    int currentNodeIndex = 0;

    void updatePlacedEntitySpriteFrame(Entity* entity, int tileIndex);
    void placeEntity(sf::Vector2i mousePos);
    void handleEvents();
    void update();
    void render();
    void loadEntities();
    void createTileThumbnails();
    void handleMouseClick(sf::Vector2i mousePos);
    void placeTile(sf::Vector2i mousePos);
    void handleFloatingWindowClick(sf::Vector2f relativePos);
    void drawFloatingWindow();
    void saveScene(const std::string& filename);
    void updateGridSize();
    void toggleGrid();
    void drawGrid();
    void createGrid();

    // Funções modificadas ou novas
    void renderSidebar();
    void handleKeyPress(sf::Keyboard::Key key);
    void selectEntity(const std::string& path);
    void showEntityDetails();
    void loadFileStructure(const std::string& path, FileNode& node);
    std::string getClickedEntityPath(float x, float y, float& yOffset, int& outIndex);
    std::string getClickedEntityPathRecursive(const FileNode& node, int depth, float x, float y, float& yOffset, int& currentIndex);
    std::string getFullPath(const FileNode& node);
    void toggleNodeOpen(FileNode& node);
    void toggleSelectedNode();
    void toggleSelectedNodeRecursive(FileNode& node, int& currentIndex);
    void renderFileNode(const FileNode& node, int depth, float& yOffset, int& currentIndex);
    void navigateEntities(int direction);
    void selectEntityAtIndex(int index);
    void collectEntityPaths(const FileNode& node, std::vector<std::string>& paths);
    std::string selectedEntityPath;
    void updateEntityPreview(sf::Vector2i mousePos);
    void renderInvisibleEntity(sf::RenderWindow &window, const Entity *entity);
    void addCustomDataVariable(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *customData,
                               const std::string &type, const std::string &name, const std::string &value);
    void createMenu();
    void handleMenu();
    void showSaveFileDialog();
    
    // Nova função adicionada
    void renderPlacedEntities();
};