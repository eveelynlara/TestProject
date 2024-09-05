#pragma once
#include "Entity.hpp"
#include <vector>
#include <memory>
#include <string>

class EntityManager {
public:
    void loadEntitiesFromDirectory(const std::string& directory);
    void drawEntities(sf::RenderWindow& window) const;
    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return entities; }

private:
    std::vector<std::unique_ptr<Entity>> entities;
};