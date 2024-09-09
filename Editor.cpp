#include "Editor.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <set>

Editor::Editor() : gridSize(32), selectedEntity(nullptr), selectedTileIndex(-1), isFloatingWindowOpen(false), selectedEntityIndex(-1) {
    window.create(sf::VideoMode(1024, 768), "Editor de Entidades");
    entityManager.loadEntitiesFromDirectory("entities");
    
    if (entityManager.getEntities().empty()) {
        std::cerr << "No entities loaded. Check your entities directory." << std::endl;
    }
    
    loadEntities();
    createEntityThumbnails();
    
    editArea.setSize(sf::Vector2f(700, 768));
    editArea.setPosition(324, 0);
    editArea.setFillColor(sf::Color(240, 240, 240));
    
    sidebarArea.setSize(sf::Vector2f(324, 768));
    sidebarArea.setPosition(0, 0);
    sidebarArea.setFillColor(sf::Color(220, 220, 220));
    
    createGrid();
    
    if (!font.loadFromFile("path/to/your/font.ttf")) {
        std::cerr << "Failed to load font" << std::endl;
    }
}

void Editor::createGrid() {
    for (float x = 0; x <= 700; x += gridSize) {
        gridLines.emplace_back(sf::Vector2f(x + editArea.getPosition().x, editArea.getPosition().y));
        gridLines.emplace_back(sf::Vector2f(x + editArea.getPosition().x, editArea.getPosition().y + 768));
    }
    for (float y = 0; y <= 768; y += gridSize) {
        gridLines.emplace_back(sf::Vector2f(editArea.getPosition().x, y + editArea.getPosition().y));
        gridLines.emplace_back(sf::Vector2f(editArea.getPosition().x + 700, y + editArea.getPosition().y));
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
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                handleMouseClick(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            }
        } else if (event.type == sf::Event::KeyPressed) {
            handleKeyPress(event.key.code);
        }
    }
}

void Editor::update() {
    // Atualize a lógica do editor aqui, se necessário
}

void Editor::render() {
    window.clear(sf::Color::White);
    
    window.draw(editArea);
    window.draw(sidebarArea);
    
    window.draw(gridLines.data(), gridLines.size(), sf::PrimitiveType::Lines);

    for (const auto& entity : placedEntities) {
        entity->draw(window);
    }

    renderSidebar();

    if (isFloatingWindowOpen && selectedEntity) {
        drawFloatingWindow();
    }

    window.display();
}

void Editor::renderSidebar() {
    const float thumbnailSize = 100.0f;
    const float padding = 10.0f;
    float yPos = padding;

    for (size_t i = 0; i < entityThumbnails.size(); ++i) {
        sf::RectangleShape& thumbnail = entityThumbnails[i];
        thumbnail.setPosition(padding, yPos);
        window.draw(thumbnail);

        // Destacar a entidade selecionada
        if (static_cast<int>(i) == selectedEntityIndex) {
            sf::RectangleShape highlight(sf::Vector2f(thumbnailSize, thumbnailSize));
            highlight.setPosition(padding, yPos);
            highlight.setFillColor(sf::Color(255, 255, 0, 100));
            window.draw(highlight);
        }

        // Desenhar o nome da entidade
        sf::Text nameText(entityManager.getEntities()[i]->getName(), font, 12);
        nameText.setPosition(padding, yPos + thumbnailSize + 5);
        nameText.setFillColor(sf::Color::Black);
        window.draw(nameText);

        yPos += thumbnailSize + padding + 20; // 20 para o texto
    }
}

void Editor::handleKeyPress(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Up:
            if (selectedEntityIndex > 0) {
                selectEntity(selectedEntityIndex - 1);
            }
            break;
        case sf::Keyboard::Down:
            if (selectedEntityIndex < static_cast<int>(entityManager.getEntities().size()) - 1) {
                selectEntity(selectedEntityIndex + 1);
            }
            break;
        case sf::Keyboard::Enter:
            if (selectedEntityIndex >= 0) {
                showEntityDetails();
            }
            break;
        default:
            break;
    }
}

void Editor::selectEntity(int index) {
    if (index >= 0 && index < static_cast<int>(entityManager.getEntities().size())) {
        selectedEntityIndex = index;
        selectedEntity = entityManager.getEntities()[index].get();
        createTileThumbnails();
        selectedTileIndex = -1;
        std::cout << "Entidade selecionada: " << selectedEntity->getName() << std::endl;
    }
}

void Editor::showEntityDetails() {
    if (selectedEntity) {
        isFloatingWindowOpen = true;
        floatingWindowPosition = sf::Vector2f(324, 0);
    }
}

void Editor::drawFloatingWindow() {
    if (isFloatingWindowOpen && selectedEntity) {
        const float windowWidth = 400;
        const float windowHeight = 500;
        sf::RectangleShape floatingWindow(sf::Vector2f(windowWidth, windowHeight));
        floatingWindow.setPosition(floatingWindowPosition);
        floatingWindow.setFillColor(sf::Color(200, 200, 200));
        window.draw(floatingWindow);

        sf::Text nameText(selectedEntity->getName(), font, 20);
        nameText.setPosition(floatingWindowPosition.x + 10, floatingWindowPosition.y + 10);
        nameText.setFillColor(sf::Color::Black);
        window.draw(nameText);

        if (selectedEntity->hasSprite()) {
            const sf::Texture* texture = selectedEntity->getTexture();
            if (texture) {
                sf::Sprite fullSprite(*texture);
                
                // Calcular escala para caber na janela
                float scaleX = (windowWidth - 20) / fullSprite.getLocalBounds().width;
                float scaleY = (windowHeight - 60) / fullSprite.getLocalBounds().height;
                float scale = std::min(scaleX, scaleY);
                
                fullSprite.setScale(scale, scale);
                fullSprite.setPosition(
                    floatingWindowPosition.x + 10,
                    floatingWindowPosition.y + 50
                );
                
                window.draw(fullSprite);

                // Desenhar grade de tiles
                const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
                for (size_t i = 0; i < spriteDefinitions.size(); ++i) {
                    const auto& spriteDef = spriteDefinitions[i];
                    sf::RectangleShape tileOutline(sf::Vector2f(spriteDef.rect.width * scale, spriteDef.rect.height * scale));
                    tileOutline.setPosition(
                        fullSprite.getPosition().x + spriteDef.rect.left * scale,
                        fullSprite.getPosition().y + spriteDef.rect.top * scale
                    );
                    tileOutline.setFillColor(sf::Color::Transparent);
                    tileOutline.setOutlineColor(sf::Color::Black);
                    tileOutline.setOutlineThickness(1);
                    window.draw(tileOutline);

                    // Destacar o tile selecionado
                    if (static_cast<int>(i) == selectedTileIndex) {
                        sf::RectangleShape highlight(sf::Vector2f(spriteDef.rect.width * scale, spriteDef.rect.height * scale));
                        highlight.setPosition(tileOutline.getPosition());
                        highlight.setFillColor(sf::Color(255, 255, 0, 100));
                        window.draw(highlight);
                    }
                }
            }
        } else {
            // Entidade sem sprite (apenas colisão)
            sf::Vector2f collisionSize = selectedEntity->getCollisionSize();
            float scaleX = (windowWidth - 40) / collisionSize.x;
            float scaleY = (windowHeight - 80) / collisionSize.y;
            float scale = std::min(scaleX, scaleY);
            
            sf::RectangleShape collisionShape(collisionSize * scale);
            collisionShape.setFillColor(sf::Color::Transparent);
            collisionShape.setOutlineColor(sf::Color::Red);
            collisionShape.setOutlineThickness(2);
            collisionShape.setPosition(
                floatingWindowPosition.x + (windowWidth - collisionShape.getSize().x) / 2,
                floatingWindowPosition.y + 60 + (windowHeight - 80 - collisionShape.getSize().y) / 2
            );
            
            window.draw(collisionShape);

            sf::Text infoText("Collision-only entity", font, 16);
            infoText.setPosition(floatingWindowPosition.x + 10, floatingWindowPosition.y + 40);
            infoText.setFillColor(sf::Color::Black);
            window.draw(infoText);
        }
    }
}

void Editor::handleMouseClick(sf::Vector2i mousePos) {
    if (sidebarArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        // Clique na barra lateral
        for (size_t i = 0; i < entityThumbnails.size(); ++i) {
            if (entityThumbnails[i].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectEntity(i);
                showEntityDetails();
                return;
            }
        }
    } else if (isFloatingWindowOpen) {
        // Verificar se o clique foi na janela flutuante
        sf::FloatRect windowBounds(floatingWindowPosition, sf::Vector2f(400, 500));
        if (windowBounds.contains(mousePos.x, mousePos.y)) {
            handleFloatingWindowClick(sf::Vector2f(mousePos) - floatingWindowPosition);
        }
    } else if (editArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        // Clique na área de edição
        if (selectedEntity && selectedTileIndex >= 0) {
            std::cout << "Tentando colocar entidade na posição: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
            placeEntity(mousePos);
        } else {
            std::cout << "Não foi possível colocar a entidade. selectedEntity: " << (selectedEntity ? "true" : "false")
                      << ", selectedTileIndex: " << selectedTileIndex << std::endl;
        }
    }
}

void Editor::loadEntities() {
    entityManager.loadEntitiesFromDirectory("entities");
    if (entityManager.getEntities().empty()) {
        std::cerr << "No entities loaded. Check your entities directory." << std::endl;
    } else {
        std::cout << "Loaded " << entityManager.getEntities().size() << " entities." << std::endl;
    }
    createEntityThumbnails();
}

void Editor::placeEntity(sf::Vector2i mousePos) {
    if (!selectedEntity || selectedTileIndex < 0) {
        std::cout << "Não foi possível colocar a entidade. selectedEntity: " << (selectedEntity ? "true" : "false")
                  << ", selectedTileIndex: " << selectedTileIndex << std::endl;
        return;
    }

    sf::Vector2f adjustedPos(mousePos.x - editArea.getPosition().x, mousePos.y - editArea.getPosition().y);
    int gridX = static_cast<int>(adjustedPos.x / gridSize) * gridSize;
    int gridY = static_cast<int>(adjustedPos.y / gridSize) * gridSize;

    auto newEntity = std::make_unique<Entity>(*selectedEntity);
    newEntity->setPosition(editArea.getPosition().x + gridX, editArea.getPosition().y + gridY);
    
    const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
    if (selectedTileIndex < spriteDefinitions.size()) {
        newEntity->setTextureRect(spriteDefinitions[selectedTileIndex].rect);
    }

    placedEntities.push_back(std::move(newEntity));

    std::cout << "Entidade colocada na posição: (" << (editArea.getPosition().x + gridX) << ", " << (editArea.getPosition().y + gridY) << ")" << std::endl;
    std::cout << "Total de entidades colocadas: " << placedEntities.size() << std::endl;
}

void Editor::createEntityThumbnails() {
    const float thumbnailSize = 100.0f;
    const float padding = 10.0f;
    float yPos = padding;

    std::set<std::string> addedEntities;  // Para rastrear entidades já adicionadas

    entityThumbnails.clear();  // Limpar miniaturas existentes

    for (const auto& entity : entityManager.getEntities()) {
        if (!entity) {
            std::cerr << "Null entity encountered in createEntityThumbnails" << std::endl;
            continue;
        }

        // Verifica se esta entidade já foi adicionada
        std::string entityName = entity->getName();
        if (addedEntities.find(entityName) != addedEntities.end()) {
            continue;  // Pula esta entidade se já foi adicionada
        }

        sf::RectangleShape thumbnail(sf::Vector2f(thumbnailSize, thumbnailSize));
        
        if (entity->hasSprite()) {
            const sf::Texture* texture = entity->getTexture();
            if (texture) {
                thumbnail.setTexture(texture);
                thumbnail.setTextureRect(sf::IntRect(0, 0, texture->getSize().x, texture->getSize().y));
            } else {
                thumbnail.setFillColor(sf::Color::Red);  // Indicar erro de textura
            }
        } else {
            // Entidade sem sprite (apenas colisão)
            sf::Vector2f collisionSize = entity->getCollisionSize();
            float scale = std::min(thumbnailSize / collisionSize.x, thumbnailSize / collisionSize.y);
            sf::RectangleShape collisionShape(collisionSize * scale);
            collisionShape.setFillColor(sf::Color::Transparent);
            collisionShape.setOutlineColor(sf::Color::Red);
            collisionShape.setOutlineThickness(2);
            collisionShape.setPosition((thumbnailSize - collisionShape.getSize().x) / 2, (thumbnailSize - collisionShape.getSize().y) / 2);
            
            sf::RenderTexture renderTexture;
            renderTexture.create(thumbnailSize, thumbnailSize);
            renderTexture.clear(sf::Color::White);
            renderTexture.draw(collisionShape);
            renderTexture.display();
            
            thumbnail.setTexture(&renderTexture.getTexture());
        }

        thumbnail.setPosition(padding, yPos);
        entityThumbnails.push_back(thumbnail);
        yPos += thumbnailSize + padding;

        addedEntities.insert(entityName);  // Marca esta entidade como adicionada
    }

    std::cout << "Criadas " << entityThumbnails.size() << " miniaturas de entidades únicas." << std::endl;
}

void Editor::createTileThumbnails() {
    tileThumbnails.clear();
    if (selectedEntity) {
        const float thumbnailSize = 50.0f;
        const float padding = 5.0f;
        float xPos = floatingWindowPosition.x + padding;
        float yPos = floatingWindowPosition.y + padding;
        int cols = 5;
        int col = 0;

        const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
        for (const auto& spriteDef : spriteDefinitions) {
            sf::RectangleShape thumbnail(sf::Vector2f(thumbnailSize, thumbnailSize));
            thumbnail.setTexture(selectedEntity->getTexture());
            thumbnail.setTextureRect(spriteDef.rect);
            thumbnail.setPosition(xPos, yPos);

            tileThumbnails.push_back(thumbnail);

            col++;
            if (col >= cols) {
                col = 0;
                xPos = floatingWindowPosition.x + padding;
                yPos += thumbnailSize + padding;
            } else {
                xPos += thumbnailSize + padding;
            }
        }
    }
}

void Editor::handleFloatingWindowClick(sf::Vector2f localPosition) {
    if (selectedEntity && selectedEntity->hasSprite()) {
        const sf::Texture* texture = selectedEntity->getTexture();
        if (!texture) return;

        const float windowWidth = 400;
        const float windowHeight = 500;
        
        sf::Vector2f spritePosition(10, 50);
        sf::Vector2f spriteSize(texture->getSize());
        
        float scaleX = (windowWidth - 20) / spriteSize.x;
        float scaleY = (windowHeight - 60) / spriteSize.y;
        float scale = std::min(scaleX, scaleY);

        sf::Vector2f scaledSpriteSize = spriteSize * scale;
        
        if (localPosition.x >= spritePosition.x && localPosition.x < spritePosition.x + scaledSpriteSize.x &&
            localPosition.y >= spritePosition.y && localPosition.y < spritePosition.y + scaledSpriteSize.y) {
            
            sf::Vector2f relativePos = (localPosition - spritePosition) / scale;
            
            const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
            std::cout << "Total de tiles: " << spriteDefinitions.size() << std::endl;
            std::cout << "Posição do clique (local): (" << localPosition.x << ", " << localPosition.y << ")" << std::endl;
            std::cout << "Posição relativa do clique: (" << relativePos.x << ", " << relativePos.y << ")" << std::endl;

            for (size_t i = 0; i < spriteDefinitions.size(); ++i) {
                const auto& spriteDef = spriteDefinitions[i];
                // Aumentar a margem de erro para 2 pixels
                if (relativePos.x >= spriteDef.rect.left - 2.0f && 
                    relativePos.x < spriteDef.rect.left + spriteDef.rect.width + 2.0f &&
                    relativePos.y >= spriteDef.rect.top - 2.0f && 
                    relativePos.y < spriteDef.rect.top + spriteDef.rect.height + 2.0f) {
                    selectedTileIndex = i;
                    std::cout << "Tile selecionado: " << i << std::endl;
                    std::cout << "Limites do tile: (" << spriteDef.rect.left << ", " << spriteDef.rect.top << ", " 
                              << spriteDef.rect.width << ", " << spriteDef.rect.height << ")" << std::endl;
                    return;
                }

                // Verificação específica para os tiles 3 e 7
                if (i == 3 || i == 7) {
                    std::cout << "Verificando tile " << i << ":" << std::endl;
                    std::cout << "  Limites: (" << spriteDef.rect.left << ", " << spriteDef.rect.top << ", " 
                              << spriteDef.rect.width << ", " << spriteDef.rect.height << ")" << std::endl;
                    std::cout << "  Distância X: " << std::abs(relativePos.x - (spriteDef.rect.left + spriteDef.rect.width / 2)) << std::endl;
                    std::cout << "  Distância Y: " << std::abs(relativePos.y - (spriteDef.rect.top + spriteDef.rect.height / 2)) << std::endl;
                }
            }
            // Se chegou aqui, não encontrou nenhum tile
            std::cout << "Nenhum tile selecionado." << std::endl;
        } else {
            std::cout << "Clique fora da área do sprite." << std::endl;
        }
    }
}

void Editor::saveScene(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Não foi possível abrir o arquivo para salvar a cena." << std::endl;
        return;
    }

    for (const auto& entity : placedEntities) {
        file << entity->getName() << " " 
             << entity->getPosition().x << " " 
             << entity->getPosition().y << " ";
        
        if (entity->hasSprite()) {
            const sf::IntRect& textureRect = entity->getSprite().getTextureRect();
            file << textureRect.left << " " 
                 << textureRect.top << " " 
                 << textureRect.width << " " 
                 << textureRect.height;
        } else {
            file << "0 0 0 0";  // Placeholder para entidades sem sprite
        }
        
        file << std::endl;
    }

    file.close();
    std::cout << "Cena salva em " << filename << std::endl;
}