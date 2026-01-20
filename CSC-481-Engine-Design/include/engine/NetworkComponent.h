#pragma once
#include "Component.h"

class NetworkComponent : public Component {
public:
    bool serverControlled = false;  // true = controlled by server, client only renders
    int objectId = -1;              // unique sync ID assigned by server
    int typeId = -1;                // optional: platform, orb, etc.

    NetworkComponent(bool serverOwned, int id, int type)
        : serverControlled(serverOwned), objectId(id), typeId(type) {
    }
};
