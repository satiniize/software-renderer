#pragma once

#include <cstdint>
#include <unordered_set>

using EntityID = uint32_t;

class EntityManager {
public:
  EntityManager() : nextId(1) {}

  // Create a new entity and return its unique ID
  EntityID create() {
    EntityID id = nextId++;
    activeEntities.insert(id);
    return id;
  }

  // Optionally destroy an entity (removes from active set)
  void destroy(EntityID id) { activeEntities.erase(id); }

  // Check if an entity is still active
  bool is_active(EntityID id) const {
    return activeEntities.find(id) != activeEntities.end();
  }

private:
  EntityID nextId;
  std::unordered_set<EntityID> activeEntities;
};
