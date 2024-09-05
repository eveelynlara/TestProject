#include "EntityManager.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;  // Use std::experimental::filesystem if you're not using C++17

void EntityManager::loadEntitiesFromDirectory(const std::string& directory) {
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".ent") {
            try {
                entities.push_back(std::make_unique<Entity>(entry.path().string()));
                std::cout << "Loaded entity: " << entry.path().string() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to load entity " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }
    }
}

void EntityManager::drawEntities(sf::RenderWindow& window) const {
    for (const auto& entity : entities) {
        entity->draw(window);
    }
}