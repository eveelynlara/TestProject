#include "Entity.hpp"
#include <tinyxml2.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Entity::Entity(const std::string& filename) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load " << filename << std::endl;
        return;
    }

    auto root = doc.FirstChildElement("Ethanon");
    if (!root) return;

    auto entityElement = root->FirstChildElement("Entity");
    if (!entityElement) return;

    auto spriteElement = entityElement->FirstChildElement("Sprite");
    if (spriteElement) {
        std::string spritePath = spriteElement->GetText();
        fs::path entityPath(filename);
        fs::path texturePath = entityPath.parent_path() / spritePath;
        
        if (!texture.loadFromFile(texturePath.string())) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }
        sprite.setTexture(texture);
        
        // Load XML atlas if it exists
        fs::path atlasPath = texturePath.replace_extension("xml");
        if (fs::exists(atlasPath)) {
            loadTextureAtlas(atlasPath.string());
        }
    }

    name = filename;
}void Entity::loadTextureAtlas(const std::string& atlasPath) {
    tinyxml2::XMLDocument atlasDoc;
    if (atlasDoc.LoadFile(atlasPath.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load texture atlas: " << atlasPath << std::endl;
        return;
    }

    auto atlas = atlasDoc.FirstChildElement("TextureAtlas");
    if (!atlas) return;

    for (auto spriteElement = atlas->FirstChildElement("sprite"); spriteElement; spriteElement = spriteElement->NextSiblingElement("sprite")) {
        SpriteInfo info;
        info.name = spriteElement->Attribute("n");
        int x = spriteElement->IntAttribute("x");
        int y = spriteElement->IntAttribute("y");
        int w = spriteElement->IntAttribute("w");
        int h = spriteElement->IntAttribute("h");

        info.rect = sf::IntRect(x, y, w, h);
        spriteInfos.push_back(info);
    }

    std::cout << "Loaded " << spriteInfos.size() << " sprite definitions from atlas." << std::endl;
}

void Entity::draw(sf::RenderWindow& window) const {
    window.draw(sprite);
}

sf::Vector2f Entity::getPosition() const {
    return sprite.getPosition();
}

void Entity::setPosition(float x, float y) {
    sprite.setPosition(x, y);
}