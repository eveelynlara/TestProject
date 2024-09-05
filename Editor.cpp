#include "Editor.hpp"
#include <iostream>
#include <filesystem>

Editor::Editor() : window(sf::VideoMode(1280, 720), "Entity Editor") {
    projectArea.setSize(sf::Vector2f(200, 720));
    projectArea.setFillColor(sf::Color(200, 200, 200));
    
    editArea.setSize(sf::Vector2f(1080, 720));
    editArea.setPosition(200, 0);
    editArea.setFillColor(sf::Color::White);
    
    loadEntities();
    createEntityThumbnails();
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
    
    for (const auto& thumbnail : tileThumbnails) {
        window.draw(thumbnail);
    }
    
    // Draw placed tiles in edit area
    
    window.display();
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
    // Check if click is in project area
    if (mousePos.x < 200) {
        // Check entity thumbnails
        for (size_t i = 0; i < entityThumbnails.size(); ++i) {
            if (entityThumbnails[i].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectedEntity = entityManager.getEntities()[i].get();
                createTileThumbnails();
                return;
            }
        }
        
        // Check tile thumbnails
        for (size_t i = 0; i < tileThumbnails.size(); ++i) {
            if (tileThumbnails[i].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectedTile = tileThumbnails[i].getTextureRect();
                return;
            }
        }
    } else {
        // Click in edit area
        if (selectedEntity && selectedTile.width > 0) {
            placeTile(mousePos);
        }
    }
}

void Editor::placeTile(sf::Vector2i mousePos) {
    // Implement tile placement logic here
    // You'll need to create a data structure to store placed tiles
    // and their positions in the edit area
}