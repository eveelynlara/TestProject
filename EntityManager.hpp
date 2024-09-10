#pragma once
#include "Entity.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

class EntityManager {
public:
    void loadEntitiesFromDirectory(const std::string& directory);
    void drawEntities(sf::RenderWindow& window) const;
    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return entities; }
    Entity* getEntityByPath(const std::string& path);

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<std::string, Entity*> entityPathMap;
};