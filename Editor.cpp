#include "Editor.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <set>
#include <tinyxml2.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <array>

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

void Editor::createMenu() {
    if (!menuFont.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        std::cerr << "Falha ao carregar a fonte do menu" << std::endl;
    }

    sf::Text fileMenu;
    fileMenu.setFont(menuFont);
    fileMenu.setString("File");
    fileMenu.setCharacterSize(20);
    fileMenu.setFillColor(sf::Color::Black);
    fileMenu.setPosition(10, 5);
    menuItems.push_back(fileMenu);

    isMenuOpen = false;
}

void Editor::handleMenu() {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        for (const auto& menuItem : menuItems) {
            if (menuItem.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                if (menuItem.getString() == "File") {
                    isMenuOpen = !isMenuOpen;
                }
            }
        }

        if (isMenuOpen) {
            sf::FloatRect menuBounds = menuItems[0].getGlobalBounds();
            sf::Vector2f savePos(menuBounds.left, menuBounds.top + menuBounds.height);
            if (sf::FloatRect(savePos.x, savePos.y, 100, 30).contains(mousePos.x, mousePos.y)) {
                showSaveFileDialog();
            }
        }
    }
}

void Editor::showSaveFileDialog() {
    // No macOS, usaremos o comando 'osascript' para abrir uma caixa de diálogo nativa
    std::string command = "osascript -e 'tell application \"System Events\" to activate' -e 'tell application \"System Events\" to set filePath to choose file name with prompt \"Save Scene As:\" default name \"scene.esc\"' -e 'return POSIX path of filePath'";
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Erro ao abrir o diálogo de salvamento" << std::endl;
        return;
    }
    
    char buffer[256];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 256, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    
    // Remove a nova linha no final do caminho do arquivo
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    if (!result.empty()) {
        saveFilePath = result;
        exportScene(saveFilePath);
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

void Editor::exportScene(const std::string& filename) {
    tinyxml2::XMLDocument doc;
    
    tinyxml2::XMLDeclaration* decl = doc.NewDeclaration();
    doc.InsertFirstChild(decl);

    tinyxml2::XMLElement* root = doc.NewElement("Ethanon");
    doc.InsertEndChild(root);

    // Scene Properties
    tinyxml2::XMLElement* sceneProps = doc.NewElement("SceneProperties");
    sceneProps->SetAttribute("lightIntensity", "2");
    sceneProps->SetAttribute("parallaxIntensity", "0");
    root->InsertEndChild(sceneProps);

    tinyxml2::XMLElement* ambient = doc.NewElement("Ambient");
    ambient->SetAttribute("r", "1");
    ambient->SetAttribute("g", "1");
    ambient->SetAttribute("b", "1");
    sceneProps->InsertEndChild(ambient);

    tinyxml2::XMLElement* zAxis = doc.NewElement("ZAxisDirection");
    zAxis->SetAttribute("x", "0");
    zAxis->SetAttribute("y", "-1");
    sceneProps->InsertEndChild(zAxis);

    // Entities in Scene
    tinyxml2::XMLElement* entitiesInScene = doc.NewElement("EntitiesInScene");
    root->InsertEndChild(entitiesInScene);

    int entityId = 1;
    for (const auto& entity : placedEntities) {
        tinyxml2::XMLElement* entityElement = doc.NewElement("Entity");
        entityElement->SetAttribute("id", entityId++);
        entityElement->SetAttribute("spriteFrame", entity->getSelectedTileIndex());
        entitiesInScene->InsertEndChild(entityElement);

        // Extrair apenas o nome do arquivo da entidade
        std::string entityFileName = entity->getName();
        size_t lastSlash = entityFileName.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            entityFileName = entityFileName.substr(lastSlash + 1);
        }

        tinyxml2::XMLElement* entityNameElement = doc.NewElement("EntityName");
        entityNameElement->SetText(entityFileName.c_str());
        entityElement->InsertEndChild(entityNameElement);

        tinyxml2::XMLElement* position = doc.NewElement("Position");
        sf::Vector2f entityPos = entity->getPosition();
        position->SetAttribute("x", static_cast<int>(entityPos.x));
        position->SetAttribute("y", static_cast<int>(entityPos.y));
        position->SetAttribute("z", 0);
        position->SetAttribute("angle", 0);
        entityElement->InsertEndChild(position);

        tinyxml2::XMLElement* entityDetails = doc.NewElement("Entity");
        entityElement->InsertEndChild(entityDetails);

        tinyxml2::XMLElement* fileName = doc.NewElement("FileName");
        fileName->SetText(entityFileName.c_str());
        entityDetails->InsertEndChild(fileName);

        // Adicionar CustomData
        tinyxml2::XMLElement* customData = doc.NewElement("CustomData");
        entityDetails->InsertEndChild(customData);

        // Adicionar variáveis de CustomData
        addCustomDataVariable(doc, customData, "uint", "allowDecals", "1");
        addCustomDataVariable(doc, customData, "string", "material", "stone");
    }

    // Salvar o documento XML
    tinyxml2::XMLError result = doc.SaveFile(filename.c_str());
    if (result == tinyxml2::XML_SUCCESS) {
        std::cout << "Cena exportada com sucesso para " << filename << std::endl;
    } else {
        std::cerr << "Erro ao exportar a cena para " << filename << std::endl;
    }
}

void Editor::renderPlacedEntities() {
    for (const auto& entity : placedEntities) {
        sf::Vector2f pos = entity->getPosition();
        pos.x += editArea.getPosition().x;
        pos.y += editArea.getPosition().y;
        
        if (entity->hasSprite()) {
            sf::Sprite sprite = entity->getSprite();
            sprite.setPosition(pos);
            window.draw(sprite);
        } else {
            renderInvisibleEntity(window, entity.get());
        }
    }
}

// Função auxiliar para adicionar variáveis de CustomData
void Editor::addCustomDataVariable(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* customData, 
                                   const std::string& type, const std::string& name, const std::string& value) {
    tinyxml2::XMLElement* variable = doc.NewElement("Variable");
    customData->InsertEndChild(variable);

    tinyxml2::XMLElement* typeElement = doc.NewElement("Type");
    typeElement->SetText(type.c_str());
    variable->InsertEndChild(typeElement);

    tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
    nameElement->SetText(name.c_str());
    variable->InsertEndChild(nameElement);

    tinyxml2::XMLElement* valueElement = doc.NewElement("Value");
    valueElement->SetText(value.c_str());
    variable->InsertEndChild(valueElement);
}

void Editor::selectEntity(const std::string& path) {
    Entity* entity = entityManager.getEntityByPath(path);
    if (entity) {
        selectedEntity = entity;
        updateGridSize();
        selectedEntityPath = path;
        selectedTileIndex = 0;
        isFloatingWindowOpen = true;
        floatingWindowPosition = sf::Vector2f(324, 0);
        createTileThumbnails();
        std::cout << "Entidade selecionada: " << path << std::endl;
    } else {
        std::cout << "Entidade não encontrada: " << path << std::endl;
    }
}

void Editor::run() {
    createMenu();
    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        updateEntityPreview(mousePos);
        
        handleEvents();
        handleMenu();
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
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                toggleGrid();
            }
        }
        if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                toggleGrid();
            }
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

    if (showGrid) {
        drawGrid();
    }

    // Renderize as entidades colocadas
    renderPlacedEntities();


    // Renderize o preview da entidade
    if (selectedEntity) {
        window.draw(entityPreview);
    }

    if (selectedEntity && selectedTileIndex >= 0) {
        window.draw(entityPreview);
    }

    if (isFloatingWindowOpen && selectedEntity) {
        drawFloatingWindow();
    }
    
    // Desenhar menu
    for (const auto& menuItem : menuItems) {
        window.draw(menuItem);
    }

    if (isMenuOpen) {
        sf::RectangleShape menuBackground(sf::Vector2f(100, 30));
        menuBackground.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect menuBounds = menuItems[0].getGlobalBounds();
        menuBackground.setPosition(menuBounds.left, menuBounds.top + menuBounds.height);
        window.draw(menuBackground);

        sf::Text saveOption;
        saveOption.setFont(menuFont);
        saveOption.setString("Save");
        saveOption.setCharacterSize(18);
        saveOption.setFillColor(sf::Color::Black);
        saveOption.setPosition(menuBackground.getPosition() + sf::Vector2f(5, 5));
        window.draw(saveOption);
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
        case sf::Keyboard::S:
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem) || sf::Keyboard::isKeyPressed(sf::Keyboard::RSystem)) {
                showSaveFileDialog();
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
    //std::cout << "Desenhando janela flutuante" << std::endl;
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

        sf::Texture invisibleTexture;
        if (invisibleTexture.loadFromFile("entities/invisible.png")) {
            sf::Sprite invisibleSprite(invisibleTexture);
            invisibleSprite.setPosition(
                floatingWindowPosition.x + (windowWidth - invisibleTexture.getSize().x) / 2,
                floatingWindowPosition.y + 60
            );
            window.draw(invisibleSprite);
        }

        sf::Text infoText("Entidade invisível", font, 16);
        infoText.setPosition(floatingWindowPosition.x + 10, floatingWindowPosition.y + 40);
        infoText.setFillColor(sf::Color::Black);
        window.draw(infoText);
    }
}

void Editor::updatePlacedEntitySpriteFrame(Entity* entity, int tileIndex) {
    if (entity && tileIndex >= 0) {
        entity->setSelectedTileIndex(tileIndex);
        const auto& spriteDefinitions = entity->getSpriteDefinitions();
        if (tileIndex < spriteDefinitions.size()) {
            entity->setTextureRect(spriteDefinitions[tileIndex].rect);
        }
    }
}

void Editor::placeEntity(sf::Vector2i mousePos) {
    if (!selectedEntity) {
        std::cout << "Não foi possível colocar a entidade. Nenhuma entidade selecionada." << std::endl;
        return;
    }

    // Calcula a posição relativa à área de edição
    sf::Vector2f relativePos(mousePos.x - editArea.getPosition().x, mousePos.y - editArea.getPosition().y);

    // Alinha à grade
    int gridX = static_cast<int>(relativePos.x / gridSize) * gridSize;
    int gridY = static_cast<int>(relativePos.y / gridSize) * gridSize;

    auto newEntity = std::make_unique<Entity>(*selectedEntity);
    
    // Define a posição da entidade em relação à área de edição
    newEntity->setPosition(gridX, gridY);
    
    if (selectedEntity->hasSprite() && selectedTileIndex >= 0) {
        const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
        if (selectedTileIndex < spriteDefinitions.size()) {
            newEntity->setTextureRect(spriteDefinitions[selectedTileIndex].rect);
            newEntity->setSelectedTileIndex(selectedTileIndex);
        }
    } else {
        // Para entidades invisíveis, use o tamanho da colisão
        sf::Vector2f collisionSize = selectedEntity->getCollisionSize();
        newEntity->setTextureRect(sf::IntRect(0, 0, collisionSize.x, collisionSize.y));
        newEntity->setSelectedTileIndex(0);
    }

    placedEntities.push_back(std::move(newEntity));

    std::cout << "Entidade " << (selectedEntity->hasSprite() ? "" : "invisível ")
              << "colocada na posição: (" << gridX << ", " << gridY << ")" << std::endl;
    std::cout << "Total de entidades colocadas: " << placedEntities.size() << std::endl;
}

void Editor::updateEntityPreview(sf::Vector2i mousePos) {
    if (selectedEntity && editArea.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
        sf::Vector2f adjustedPos(mousePos.x - editArea.getPosition().x, mousePos.y - editArea.getPosition().y);

        // Alinha à grade
        int gridX = static_cast<int>(adjustedPos.x / gridSize) * gridSize;
        int gridY = static_cast<int>(adjustedPos.y / gridSize) * gridSize;

        // Define a posição do preview
        entityPreview.setPosition(editArea.getPosition().x + gridX, editArea.getPosition().y + gridY);
        
        if (selectedEntity->hasSprite()) {
            const auto& spriteDefinitions = selectedEntity->getSpriteDefinitions();
            if (selectedTileIndex >= 0 && selectedTileIndex < spriteDefinitions.size()) {
                entityPreview.setTextureRect(spriteDefinitions[selectedTileIndex].rect);
            }
            entityPreview.setTexture(*selectedEntity->getTexture());
            entityPreview.setColor(sf::Color(255, 255, 255, 128)); // Semi-transparente
        } else {
            // Para entidades invisíveis, use o tamanho da colisão
            sf::Vector2f collisionSize = selectedEntity->getCollisionSize();
            entityPreview.setTexture(sf::Texture());  // Remove a textura
            entityPreview.setTextureRect(sf::IntRect(0, 0, collisionSize.x, collisionSize.y));
            entityPreview.setColor(sf::Color(200, 0, 0, 128));  // Vermelho semi-transparente
        }
    }
}

void Editor::renderInvisibleEntity(sf::RenderWindow& window, const Entity* entity) {
    sf::Vector2f entityPos = entity->getPosition();
    entityPos.x += editArea.getPosition().x;
    entityPos.y += editArea.getPosition().y;
    sf::Vector2f entitySize = entity->getCollisionSize();

    sf::RectangleShape shape(entitySize);
    shape.setPosition(entityPos);
    shape.setFillColor(sf::Color(200, 0, 0, 128));  // Vermelho semi-transparente
    shape.setOutlineColor(sf::Color::Red);
    shape.setOutlineThickness(1);
    window.draw(shape);

    static sf::Texture invisibleTexture;
    static bool textureLoaded = false;
    if (!textureLoaded) {
        textureLoaded = invisibleTexture.loadFromFile("entities/invisible.png");
    }
    
    if (textureLoaded) {
        sf::Sprite invisibleSprite(invisibleTexture);
        invisibleSprite.setPosition(entityPos);
        invisibleSprite.setScale(entitySize.x / invisibleTexture.getSize().x, entitySize.y / invisibleTexture.getSize().y);
        window.draw(invisibleSprite);
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

void Editor::updateGridSize() {
    currentGridSize = sf::Vector2f(32, 32);  // Tamanho fixo da grade
}

void Editor::toggleGrid() {
    showGrid = !showGrid;
}

void Editor::drawGrid() {
    if (!showGrid) return;

    sf::VertexArray lines(sf::Lines);
    for (float x = editArea.getPosition().x; x <= editArea.getPosition().x + editArea.getSize().x; x += currentGridSize.x) {
        lines.append(sf::Vertex(sf::Vector2f(x, editArea.getPosition().y), sf::Color(200, 200, 200, 100)));
        lines.append(sf::Vertex(sf::Vector2f(x, editArea.getPosition().y + editArea.getSize().y), sf::Color(200, 200, 200, 100)));
    }
    for (float y = editArea.getPosition().y; y <= editArea.getPosition().y + editArea.getSize().y; y += currentGridSize.y) {
        lines.append(sf::Vertex(sf::Vector2f(editArea.getPosition().x, y), sf::Color(200, 200, 200, 100)));
        lines.append(sf::Vertex(sf::Vector2f(editArea.getPosition().x + editArea.getSize().x, y), sf::Color(200, 200, 200, 100)));
    }
    window.draw(lines);
}