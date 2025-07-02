#ifndef ENTITY_H
#define ENTITY_H

#include "component.h"
#include <vector>

class Entity {
public:
  std::vector<Component *> components;
};

#endif // ENTITY_H
