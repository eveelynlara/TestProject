#include "EntityManager.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void EntityManager::loadEntitiesFromDirectory(const std::string& directory) {
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".ent") {
            try {
                auto entity = std::make_unique<Entity>(entry.path().string());
                std::string relativePath = fs::relative(entry.path(), directory).string();
                entityPathMap[relativePath] = entity.get();
                entities.push_back(std::move(entity));
                std::cout << "Entidade carregada: " << relativePath << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Falha ao carregar entidade " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }
    }
}

void EntityManager::drawEntities(sf::RenderWindow& window) const {
    for (const auto& entity : entities) {
        entity->draw(window);
    }
}

Entity* EntityManager::getEntityByPath(const std::string& path) {
    std::string adjustedPath = path;
    if (path.find("entities/") == 0) {
        adjustedPath = path.substr(9); // Remove "entities/" do início
    }
    auto it = entityPathMap.find(adjustedPath);
    if (it != entityPathMap.end()) {
        return it->second;
    }
    return nullptr;
}