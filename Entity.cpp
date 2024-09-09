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
    if (!root) {
        std::cerr << "Missing Ethanon root element in " << filename << std::endl;
        return;
    }

    auto entityElement = root->FirstChildElement("Entity");
    if (!entityElement) {
        std::cerr << "Missing Entity element in " << filename << std::endl;
        return;
    }

    name = filename;

    auto spriteElement = entityElement->FirstChildElement("Sprite");
    if (spriteElement && spriteElement->GetText()) {
        spritePath = spriteElement->GetText();
        fs::path entityPath(filename);
        fs::path texturePath = entityPath.parent_path() / spritePath;
        
        if (!texture.loadFromFile(texturePath.string())) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
        } else {
            std::cout << "Successfully loaded texture: " << texturePath << std::endl;
            sprite.setTexture(texture);
            
            auto spriteCutElement = entityElement->FirstChildElement("SpriteCut");
            if (spriteCutElement) {
                int cutX = spriteCutElement->IntAttribute("x", 1);
                int cutY = spriteCutElement->IntAttribute("y", 1);
                loadTextureAtlas(texturePath.string(), cutX, cutY);
            } else {
                loadTextureAtlas(texturePath.string(), 1, 1);
            }
        }
    }

    auto collisionElement = entityElement->FirstChildElement("Collision");
    if (collisionElement) {
        auto sizeElement = collisionElement->FirstChildElement("Size");
        if (sizeElement) {
            collisionSize.x = sizeElement->FloatAttribute("x");
            collisionSize.y = sizeElement->FloatAttribute("y");
        }
    }

    // Se não houver sprite, use o tamanho da colisão para definir o tamanho da entidade
    if (spriteDefinitions.empty() && collisionSize.x > 0 && collisionSize.y > 0) {
        SpriteDefinition collisionDef;
        collisionDef.name = "collision";
        collisionDef.rect = sf::IntRect(0, 0, collisionSize.x, collisionSize.y);
        spriteDefinitions.push_back(collisionDef);
    }

    auto customDataElement = entityElement->FirstChildElement("CustomData");
    if (customDataElement) {
        loadCustomData(customDataElement);
    }
}

Entity::Entity(const Entity& other)
    : sprite(other.sprite), texture(other.texture), name(other.name),
      spriteDefinitions(other.spriteDefinitions), customData(other.customData),
      spritePath(other.spritePath), collisionSize(other.collisionSize) 
{
    sprite.setTexture(texture);
}

void Entity::loadCustomData(const tinyxml2::XMLElement* customDataElement) {
    for (auto variableElement = customDataElement->FirstChildElement("Variable");
         variableElement;
         variableElement = variableElement->NextSiblingElement("Variable")) {
        
        auto nameElement = variableElement->FirstChildElement("Name");
        auto valueElement = variableElement->FirstChildElement("Value");
        
        if (nameElement && valueElement) {
            std::string key = nameElement->GetText();
            std::string value = valueElement->GetText();
            customData[key] = value;
        }
    }
}

void Entity::setCustomData(const std::string& key, const std::string& value) {
    customData[key] = value;
}

std::string Entity::getCustomData(const std::string& key) const {
    auto it = customData.find(key);
    if (it != customData.end()) {
        return it->second;
    }
    return "";
}

void Entity::loadTextureAtlas(const std::string& atlasPath, int cutX, int cutY) {
    tinyxml2::XMLDocument atlasDoc;
    std::string xmlPath = atlasPath.substr(0, atlasPath.find_last_of('.')) + ".xml";
    
    if (atlasDoc.LoadFile(xmlPath.c_str()) == tinyxml2::XML_SUCCESS) {
        auto atlas = atlasDoc.FirstChildElement("TextureAtlas");
        if (atlas) {
            for (auto spriteElement = atlas->FirstChildElement("sprite"); spriteElement; spriteElement = spriteElement->NextSiblingElement("sprite")) {
                const char* name = spriteElement->Attribute("n");
                if (!name) continue;

                SpriteDefinition spriteDef;
                spriteDef.name = name;
                spriteDef.rect = sf::IntRect(
                    spriteElement->IntAttribute("x"),
                    spriteElement->IntAttribute("y"),
                    spriteElement->IntAttribute("w"),
                    spriteElement->IntAttribute("h")
                );
                spriteDefinitions.push_back(spriteDef);
            }
        }
    } else {
        // Se não houver arquivo XML, usar SpriteCut ou dividir a textura em tiles
        sf::Vector2u textureSize = texture.getSize();
        int tileWidth = textureSize.x / cutX;
        int tileHeight = textureSize.y / cutY;

        for (int y = 0; y < cutY; ++y) {
            for (int x = 0; x < cutX; ++x) {
                SpriteDefinition spriteDef;
                spriteDef.name = "tile_" + std::to_string(y * cutX + x);
                spriteDef.rect = sf::IntRect(x * tileWidth, y * tileHeight, tileWidth, tileHeight);
                spriteDefinitions.push_back(spriteDef);
            }
        }
    }

    std::cout << "Loaded " << spriteDefinitions.size() << " sprite definitions." << std::endl;
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