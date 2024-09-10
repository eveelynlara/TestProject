#include "Editor.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <set>

namespace fs = std::filesystem;

Editor::Editor() : gridSize(32), selectedEntity(nullptr), selectedTileIndex(-1), isFloatingWindowOpen(false), selectedEntityIndex(-1), selectedEntityPath(""), selectedNodeIndex(-1) {
    window.create(sf::VideoMode(1024, 768), "Editor de Entidades");
    entityManager.loadEntitiesFromDirectory("entities");
    
    if (entityManager.getEntities().empty()) {
        std::cerr << "Nenhuma entidade carregada. Verifique o diretório de entidades." << std::endl;
    }
    
    rootNode.name = "entities";
    rootNode.isDirectory = true;
    rootNode.isOpen = true;
    loadFileStructure("entities", rootNode);
    
    editArea.setSize(sf::Vector2f(700, 768));
    editArea.setPosition(324, 0);
    editArea.setFillColor(sf::Color(240, 240, 240));
    
    sidebarArea.setSize(sf::Vector2f(324, 768));
    sidebarArea.setPosition(0, 0);
    sidebarArea.setFillColor(sf::Color(220, 220, 220));
    
    createGrid();
    
    if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        std::cerr << "Falha ao carregar a fonte" << std::endl;
    }
}

void Editor::loadFileStructure(const std::string& path, FileNode& node) {
    std::cout << "Carregando estrutura de arquivos para: " << path << std::endl;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory() || (entry.path().extension() == ".ent")) {
            FileNode childNode;
            childNode.name = entry.path().filename().string();
            childNode.isDirectory = entry.is_directory();
            childNode.isOpen = false;
            
            if (childNode.isDirectory) {
                loadFileStructure(entry.path().string(), childNode);
            }
            
            node.children.push_back(childNode);
            std::cout << "Adicionado: " << childNode.name << (childNode.isDirectory ? " (diretório)" : " (arquivo)") << std::endl;
        }
    }
}

void Editor::renderSidebar() {
    const float padding = 10.0f;
    float yOffset = padding;
    int currentIndex = 0;
    renderFileNode(rootNode, 0, yOffset, currentIndex);
}

void Editor::renderFileNode(const FileNode& node, int depth, float& yOffset, int& currentIndex) {
    const float indentSize = 20.0f;
    const float lineHeight = 20.0f;
    const float xPos = 10.0f + depth * indentSize;

    sf::Text text(node.name, font, 12);
    text.setPosition(xPos, yOffset);
    text.setFillColor(sf::Color::Black);

    if (node.isDirectory) {
        text.setString((node.isOpen ? "- " : "+ ") + node.name);
    }

    // Highlight para nós selecionados (diretórios e arquivos)
    bool isSelected = (!node.isDirectory && currentIndex == selectedNodeIndex) || 
                      (selectedEntityPath == getFullPath(node));
    
    if (isSelected) {
        sf::RectangleShape highlight(sf::Vector2f(sidebarArea.getSize().x - xPos, lineHeight));
        highlight.setPosition(xPos, yOffset);
        highlight.setFillColor(sf::Color(200, 200, 255, 100));
        window.draw(highlight);
    }

    window.draw(text);

    yOffset += lineHeight;

    if (!node.isDirectory) {
        currentIndex++;
    }

    if (node.isDirectory && node.isOpen) {
        for (const auto& child : node.children) {
            renderFileNode(child, depth + 1, yOffset, currentIndex);
        }
    }
}

void Editor::handleMouseClick(sf::Vector2i mousePos) {
    if (sidebarArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        float yOffset = 10.0f;
        int currentIndex = 0;
        std::string clickedPath = getClickedEntityPath(mousePos.x, mousePos.y, yOffset, currentIndex);
        std::cout << "Caminho clicado: " << clickedPath << std::endl;
        if (!clickedPath.empty()) {
            selectedNodeIndex = currentIndex - 1; // Atualize o índice selecionado
            selectEntity(clickedPath);
            showEntityDetails();
        }
    } else if (isFloatingWindowOpen) {
        sf::FloatRect windowBounds(floatingWindowPosition, sf::Vector2f(400, 500));
        if (windowBounds.contains(mousePos.x, mousePos.y)) {
            handleFloatingWindowClick(sf::Vector2f(mousePos) - floatingWindowPosition);
        } else if (editArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
            if (selectedEntity && selectedTileIndex >= 0) {
                std::cout << "Tentando colocar entidade na posição: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
                placeEntity(mousePos);
            }
        }
    } else if (editArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        if (selectedEntity && selectedTileIndex >= 0) {
            std::cout << "Tentando colocar entidade na posição: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
            placeEntity(mousePos);
        }
    }
}

std::string Editor::getFullPath(const FileNode& node) {
    std::string path = "entities/";
    path += node.name;
    return path;
}

std::string Editor::getClickedEntityPath(float x, float y, float& yOffset, int& outIndex) {
    outIndex = 0;
    return getClickedEntityPathRecursive(rootNode, 0, x, y, yOffset, outIndex);
}

std::string Editor::getClickedEntityPathRecursive(const FileNode& node, int depth, float x, float y, float& yOffset, int& currentIndex) {
    const float indentSize = 20.0f;
    const float lineHeight = 20.0f;
    const float xPos = 10.0f + depth * indentSize;

    if (y >= yOffset && y < yOffset + lineHeight) {
        if (!node.isDirectory) {
            currentIndex++;
            return getFullPath(node);
        }
    }

    yOffset += lineHeight;

    if (node.isDirectory && node.isOpen) {
        for (const auto& child : node.children) {
            std::string result = getClickedEntityPathRecursive(child, depth + 1, x, y, yOffset, currentIndex);
            if (!result.empty()) {
                return result;
            }
        }
    }

    if (!node.isDirectory) {
        currentIndex++;
    }

    return "";
}

void Editor::showEntityDetails() {
    if (selectedEntity) {
        isFloatingWindowOpen = true;
        floatingWindowPosition = sf::Vector2f(324, 0);
        std::cout << "Mostrando detalhes da entidade: " << selectedEntity->getName() << std::endl;
    } else {
        std::cout << "Nenhuma entidade selecionada para mostrar detalhes." << std::endl;
    }
}

void Editor::toggleNodeOpen(FileNode& node) {
    if (node.isDirectory) {
        node.isOpen = !node.isOpen;
    }
}

void Editor::selectEntity(const std::string& path) {
    Entity* entity = entityManager.getEntityByPath(path);
    if (entity) {
        selectedEntity = entity;
        selectedEntityPath = path;
        selectedTileIndex = -1;
        isFloatingWindowOpen = true;
        floatingWindowPosition = sf::Vector2f(324, 0);
        createTileThumbnails();
        std::cout << "Entidade selecionada: " << path << std::endl;
    } else {
        std::cout << "Entidade não encontrada: " << path << std::endl;
    }
}

void Editor::run() {
    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        updateEntityPreview(mousePos);
        
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
                sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
                handleMouseClick(mousePos);
            }
        } else if (event.type == sf::Event::KeyPressed) {
            std::cout << "Tecla pressionada: " << event.key.code << std::endl;
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

    renderSidebar();

    // Renderize as entidades colocadas
    for (const auto& entity : placedEntities) {
        entity->draw(window);
    }

    // Renderize o preview da entidade
    if (selectedEntity && selectedTileIndex >= 0) {
        window.draw(entityPreview);
    }

    if (isFloatingWindowOpen && selectedEntity) {
        drawFloatingWindow();
    }

    window.display();
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

void Editor::handleKeyPress(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Up:
            navigateEntities(-1);
            break;
        case sf::Keyboard::Down:
            navigateEntities(1);
            break;
        case sf::Keyboard::Enter:
            if (selectedNodeIndex >= 0) {
                selectEntityAtIndex(selectedNodeIndex);
            }
            break;
        default:
            break;
    }
}

void Editor::selectEntityAtIndex(int index) {
    std::vector<std::string> entityPaths;
    collectEntityPaths(rootNode, entityPaths);

    if (index >= 0 && index < static_cast<int>(entityPaths.size())) {
        selectEntity(entityPaths[index]);
    }
}

void Editor::navigateEntities(int direction) {
    std::vector<std::string> entityPaths;
    collectEntityPaths(rootNode, entityPaths);

    if (entityPaths.empty()) return;

    selectedNodeIndex += direction;
    if (selectedNodeIndex < 0) selectedNodeIndex = entityPaths.size() - 1;
    if (selectedNodeIndex >= static_cast<int>(entityPaths.size())) selectedNodeIndex = 0;

    std::cout << "Navegando para entidade: " << entityPaths[selectedNodeIndex] << std::endl;
    selectEntity(entityPaths[selectedNodeIndex]);
}

void Editor::collectEntityPaths(const FileNode& node, std::vector<std::string>& paths) {
    if (!node.isDirectory) {
        paths.push_back(getFullPath(node));
    }
    for (const auto& child : node.children) {
        collectEntityPaths(child, paths);
    }
}

void Editor::toggleSelectedNode() {
    int currentIndex = 0;
    toggleSelectedNodeRecursive(rootNode, currentIndex);
}

void Editor::toggleSelectedNodeRecursive(FileNode& node, int& currentIndex) {
    if (currentIndex == selectedNodeIndex) {
        if (node.isDirectory) {
            node.isOpen = !node.isOpen;
        } else {
            selectEntity(node.name);
        }
        return;
    }

    currentIndex++;

    if (node.isDirectory && node.isOpen) {
        for (auto& child : node.children) {
            toggleSelectedNodeRecursive(child, currentIndex);
            if (currentIndex > selectedNodeIndex) {
                return;
            }
        }
    }
}

void Editor::drawFloatingWindow() {
    std::cout << "Desenhando janela flutuante" << std::endl;
    if (!selectedEntity) {
        std::cout << "Nenhuma entidade selecionada para desenhar janela flutuante" << std::endl;
        return;
    }

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
            
            float scaleX = (windowWidth - 20) / fullSprite.getLocalBounds().width;
            float scaleY = (windowHeight - 60) / fullSprite.getLocalBounds().height;
            float scale = std::min(scaleX, scaleY);
            
            fullSprite.setScale(scale, scale);
            fullSprite.setPosition(
                floatingWindowPosition.x + 10,
                floatingWindowPosition.y + 50
            );
            
            window.draw(fullSprite);

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

                if (static_cast<int>(i) == selectedTileIndex) {
                    sf::RectangleShape highlight(sf::Vector2f(spriteDef.rect.width * scale, spriteDef.rect.height * scale));
                    highlight.setPosition(tileOutline.getPosition());
                    highlight.setFillColor(sf::Color(255, 255, 0, 100));
                    window.draw(highlight);
                }
            }
        }
    } else {
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

        sf::Text infoText("Entidade apenas com colisão", font, 16);
        infoText.setPosition(floatingWindowPosition.x + 10, floatingWindowPosition.y + 40);
        infoText.setFillColor(sf::Color::Black);
        window.draw(infoText);
    }
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

void Editor::updateEntityPreview(sf::Vector2i mousePos) {
    if (selectedEntity && selectedTileIndex >= 0 && editArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        sf::Vector2f adjustedPos(mousePos.x - editArea.getPosition().x, mousePos.y - editArea.getPosition().y);
        int gridX = static_cast<int>(adjustedPos.x / gridSize) * gridSize;
        int gridY = static_cast<int>(adjustedPos.y / gridSize) * gridSize;

        entityPreview.setPosition(editArea.getPosition().x + gridX, editArea.getPosition().y + gridY);
        
        const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
        if (selectedTileIndex < spriteDefinitions.size()) {
            entityPreview.setTextureRect(spriteDefinitions[selectedTileIndex].rect);
        }

        entityPreview.setTexture(*selectedEntity->getTexture());
        entityPreview.setColor(sf::Color(255, 255, 255, 128)); // Semi-transparente
    }
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
            for (size_t i = 0; i < spriteDefinitions.size(); ++i) {
                const auto& spriteDef = spriteDefinitions[i];
                if (relativePos.x >= spriteDef.rect.left - 2.0f && 
                    relativePos.x < spriteDef.rect.left + spriteDef.rect.width + 2.0f &&
                    relativePos.y >= spriteDef.rect.top - 2.0f && 
                    relativePos.y < spriteDef.rect.top + spriteDef.rect.height + 2.0f) {
                    selectedTileIndex = i;
                    std::cout << "Tile selecionado: " << i << std::endl;
                    return;
                }
            }
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