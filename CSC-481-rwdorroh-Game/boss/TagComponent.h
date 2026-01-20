#pragma once
#include <engine/Component.h>
#include <string>

class TagComponent : public Component {
public:
    explicit TagComponent(const std::string& t) : tag(t) {}

    const std::string& getTag() const { return tag; }

    bool hasTag(const std::string& t) const {
        return tag == t;
    }

private:
    std::string tag;
};
