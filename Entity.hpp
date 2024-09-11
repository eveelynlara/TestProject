#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <tinyxml2.h>

struct SpriteDefinition {
    std::string name;
    sf::IntRect rect;
};

class Entity {
public:
    Entity(const std::string& filename);
    Entity(const Entity& other);  // Copy constructor

    void draw(sf::RenderWindow& window) const;
    sf::Vector2f getSize() const;
    sf::Vector2f getPosition() const;
    void setPosition(float x, float y);
    bool loadFromFile(const std::string &filename);
    const sf::Sprite &getSprite() const { return sprite; }
    const std::vector<SpriteDefinition>& getSpriteDefinitions() const { return spriteDefinitions; }
    const sf::Texture* getTexture() const { return sprite.getTexture(); }
    void setTextureRect(const sf::IntRect& rect) { sprite.setTextureRect(rect); }

    void setSelectedTileIndex(int index) { selectedTileIndex = index; }
    
    // Methods for custom data
    void setCustomData(const std::string& key, const std::string& value);
    std::string getCustomData(const std::string& key) const;

    // Adicionado: Método para obter o nome da entidade
    const std::string& getName() const { return name; }

    int getSelectedTileIndex() const { return selectedTileIndex; }

    bool hasSprite() const;
    sf::Vector2f getCollisionSize() const;

private:
    sf::Sprite sprite;
    sf::Texture texture;
    std::string name;
    std::vector<SpriteDefinition> spriteDefinitions;
    std::map<std::string, std::string> customData;
    std::string spritePath;
    sf::Vector2f collisionSize;

    int selectedTileIndex = -1;

    void loadTextureAtlas(const std::string& atlasPath, int cutX, int cutY);
    void loadCustomData(const tinyxml2::XMLElement* entityElement);
};