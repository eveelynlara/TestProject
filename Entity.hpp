#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct SpriteInfo {
    std::string name;
    sf::IntRect rect;
};

class Entity {
public:
    Entity(const std::string& filename);
    void draw(sf::RenderWindow& window) const;
    sf::Vector2f getPosition() const;
    void setPosition(float x, float y);
    const sf::Sprite& getSprite() const { return sprite; }
    const std::vector<SpriteInfo>& getSpriteInfos() const { return spriteInfos; }
    const sf::Texture* getTexture() const { return &texture; }

private:
    sf::Sprite sprite;
    sf::Texture texture;
    std::string name;
    std::vector<SpriteInfo> spriteInfos;

    void loadTextureAtlas(const std::string& atlasPath);
};