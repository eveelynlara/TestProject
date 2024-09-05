#include "Editor.hpp"
#include <iostream>
#include <filesystem>

Editor::Editor() 
    : window(sf::VideoMode(1280, 720), "Entity Editor")
    , isFloatingWindowOpen(false)
{
    projectArea.setSize(sf::Vector2f(200, 720));
    projectArea.setFillColor(sf::Color(200, 200, 200));
    
    editArea.setSize(sf::Vector2f(1080, 720));
    editArea.setPosition(200, 0);
    editArea.setFillColor(sf::Color::White);
    
    loadEntities();
    createEntityThumbnails();

    // Use um caminho relativo ou absoluto para uma fonte que você sabe que existe
    if (!font.loadFromFile("resources/Arial.ttf")) {
        std::cerr << "Erro ao carregar a fonte. Usando fonte padrão do sistema." << std::endl;
        // Use uma fonte de fallback ou continue sem texto
    }
}

void Editor::run() {
    while (window.isOpen()) {
        handleEvents();
        update();
        render();
    }
}

void Editor::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();
        else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                handleMouseClick(sf::Mouse::getPosition(window));
            }
        }
    }
}

void Editor::update() {
    // Update logic here
}

void Editor::render() {
    window.clear(sf::Color::White);
    
    window.draw(projectArea);
    window.draw(editArea);
    
    for (const auto& thumbnail : entityThumbnails) {
        window.draw(thumbnail);
    }
    
    // Draw placed tiles in edit area
    
    if (isFloatingWindowOpen) {
        drawFloatingWindow();
    }
    
    window.display();
}

void Editor::drawFloatingWindow() {
    const float windowWidth = 300.f;
    const float windowHeight = 400.f;
    const float padding = 10.f;
    const float titleHeight = 30.f;
    const float tileSize = 64.f;
    const float spacing = 2.f;

    sf::RectangleShape windowShape(sf::Vector2f(windowWidth, windowHeight));
    windowShape.setPosition(floatingWindowPosition);
    windowShape.setFillColor(sf::Color(240, 240, 240));
    windowShape.setOutlineColor(sf::Color::Black);
    windowShape.setOutlineThickness(1);
    window.draw(windowShape);
    
    sf::Text title("Select Tile", font, 16);
    title.setPosition(floatingWindowPosition + sf::Vector2f(padding, padding));
    window.draw(title);

    // Add close button
    sf::RectangleShape closeButton(sf::Vector2f(20, 20));
    closeButton.setPosition(floatingWindowPosition + sf::Vector2f(windowWidth - 30, 5));
    closeButton.setFillColor(sf::Color::Red);
    window.draw(closeButton);
    
    sf::RectangleShape contentArea(sf::Vector2f(windowWidth - 2 * padding, windowHeight - titleHeight - 2 * padding));
    contentArea.setPosition(floatingWindowPosition + sf::Vector2f(padding, titleHeight + padding));
    contentArea.setFillColor(sf::Color::White);
    window.draw(contentArea);

    sf::View oldView = window.getView();
    sf::View contentView(sf::FloatRect(0, 0, contentArea.getSize().x, contentArea.getSize().y));
    contentView.setViewport(sf::FloatRect(
        (floatingWindowPosition.x + padding) / window.getSize().x,
        (floatingWindowPosition.y + titleHeight + padding) / window.getSize().y,
        contentArea.getSize().x / window.getSize().x,
        contentArea.getSize().y / window.getSize().y
    ));
    window.setView(contentView);

    float x = 0, y = 0;

    for (size_t i = 0; i < tileThumbnails.size(); ++i) {
        sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
        tile.setPosition(x, y);
        tile.setTexture(tileThumbnails[i].getTexture());
        tile.setTextureRect(tileThumbnails[i].getTextureRect());
        
        // Highlight selected tile
        if (static_cast<int>(i) == selectedTileIndex) {
            tile.setOutlineColor(sf::Color::Red);
            tile.setOutlineThickness(2);
        }
        
        window.draw(tile);

        x += tileSize + spacing;
        if (x + tileSize > contentArea.getSize().x) {
            x = 0;
            y += tileSize + spacing;
        }
    }

    window.setView(oldView);
}

void Editor::loadEntities() {
    entityManager.loadEntitiesFromDirectory("entities");
}

void Editor::createEntityThumbnails() {
    float y = 10;
    for (const auto& entity : entityManager.getEntities()) {
        sf::RectangleShape thumbnail(sf::Vector2f(180, 50));
        thumbnail.setPosition(10, y);
        thumbnail.setTexture(entity->getSprite().getTexture());
        entityThumbnails.push_back(thumbnail);
        y += 60;
    }
}

void Editor::createTileThumbnails() {
    tileThumbnails.clear();
    
    if (!selectedEntity) return;
    
    const sf::Texture* texture = selectedEntity->getTexture();
    const auto& spriteInfos = selectedEntity->getSpriteInfos();
    
    float x = 10;
    float y = 400; // Start below entity thumbnails
    
    for (const auto& spriteInfo : spriteInfos) {
        sf::RectangleShape tileThumbnail(sf::Vector2f(64, 64)); // Use a smaller size for thumbnails
        tileThumbnail.setPosition(x, y);
        tileThumbnail.setTexture(texture);
        tileThumbnail.setTextureRect(spriteInfo.rect);
        tileThumbnails.push_back(tileThumbnail);
        
        x += 70;
        if (x > 160) { // Wrap to next row
            x = 10;
            y += 70;
        }
    }
    
    // If no sprite infos were loaded, fall back to dividing the texture into 128x128 tiles
    if (spriteInfos.empty()) {
        sf::Vector2u textureSize = texture->getSize();
        int tileSize = 128; // Use 128x128 tiles
        
        for (unsigned int j = 0; j < textureSize.y; j += tileSize) {
            for (unsigned int i = 0; i < textureSize.x; i += tileSize) {
                sf::RectangleShape tileThumbnail(sf::Vector2f(64, 64)); // Use a smaller size for thumbnails
                tileThumbnail.setPosition(x, y);
                tileThumbnail.setTexture(texture);
                tileThumbnail.setTextureRect(sf::IntRect(i, j, tileSize, tileSize));
                tileThumbnails.push_back(tileThumbnail);
                
                x += 70;
                if (x > 160) { // Wrap to next row
                    x = 10;
                    y += 70;
                }
            }
        }
    }
}

void Editor::handleMouseClick(sf::Vector2i mousePos) {
    if (mousePos.x < 200) {
        // Check entity thumbnails
        for (size_t i = 0; i < entityThumbnails.size(); ++i) {
            if (entityThumbnails[i].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectedEntity = entityManager.getEntities()[i].get();
                createTileThumbnails();
                isFloatingWindowOpen = true;
                floatingWindowPosition = sf::Vector2f(200, 0); // Position the window next to the project area
                return;
            }
        }
    } else if (isFloatingWindowOpen) {
        // Check if click is on close button
        sf::FloatRect closeButtonBounds(floatingWindowPosition + sf::Vector2f(270, 5), sf::Vector2f(20, 20));
        if (closeButtonBounds.contains(mousePos.x, mousePos.y)) {
            isFloatingWindowOpen = false;
            return;
        }
        
        // Check if click is inside the floating window
        sf::FloatRect windowBounds(floatingWindowPosition, sf::Vector2f(300, 400));
        if (windowBounds.contains(mousePos.x, mousePos.y)) {
            handleFloatingWindowClick(sf::Vector2f(mousePos) - floatingWindowPosition);
        }
    } else {
        // Click in edit area
        if (selectedEntity && selectedTile.width > 0) {
            placeTile(mousePos);
        }
    }
}

void Editor::handleFloatingWindowClick(sf::Vector2f relativePos) {
    const float padding = 10.f;
    const float titleHeight = 30.f;
    const float tileSize = 64.f;
    const float spacing = 2.f;

    // Adjust relativePos to account for padding and title
    relativePos.x -= padding;
    relativePos.y -= (titleHeight + padding);

    // Calculate which tile was clicked
    int col = static_cast<int>(relativePos.x / (tileSize + spacing));
    int row = static_cast<int>(relativePos.y / (tileSize + spacing));

    int index = row * static_cast<int>((300 - 2 * padding) / (tileSize + spacing)) + col;

    if (index >= 0 && index < static_cast<int>(tileThumbnails.size())) {
        selectedTile = tileThumbnails[index].getTextureRect();
        selectedTileIndex = index;  // Add this line to keep track of the selected tile
        // Note: We're not closing the window here anymore
    }
}

void Editor::placeTile(sf::Vector2i mousePos) {
    // Implement tile placement logic here
    // You'll need to create a data structure to store placed tiles
    // and their positions in the edit area
}