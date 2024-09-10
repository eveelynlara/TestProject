#pragma once
#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"
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
public:
    Editor();
    void run();

private:
    sf::RenderWindow window;
    EntityManager entityManager;
    
    sf::RectangleShape projectArea;
    sf::RectangleShape editArea;
    sf::RectangleShape sidebarArea;
    
    std::vector<sf::RectangleShape> tileThumbnails;
    std::vector<sf::RectangleShape> placedTiles;
    std::vector<std::unique_ptr<Entity>> placedEntities;
    
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
};