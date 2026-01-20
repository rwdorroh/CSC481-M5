#pragma once
#include <string>
#include <map>
#include <memory>

class GameObject;  // Forward declare

struct Variant {
	enum class Type {
		INT,
		FLOAT,
		GAMEOBJECT,
		// Add more as needed
	} type;

	union {
		int asInt;
		float asFloat;
		GameObject* asGameObject;
	};

	Variant() : type(Type::INT), asInt(0) {}
	Variant(int v) : type(Type::INT), asInt(v) {}
	Variant(float v) : type(Type::FLOAT), asFloat(v) {}
	Variant(GameObject* obj) : type(Type::GAMEOBJECT), asGameObject(obj) {}
};

class Event {
public:
	std::string type; // "spawn" "death" etc.
	std::map<std::string, Variant> parameters;
	int age = 0;
	int priority = 0;  // optional: LOW = 0, MEDIUM = 1, HIGH = 2, and so on if needed

	Event() = default;

	Event(const std::string& type) : type(type) {}

	void addParam(const std::string& key, Variant value) {
		parameters[key] = value;
	}

	Variant getParam(const std::string& key) const {
		auto it = parameters.find(key);
		if (it != parameters.end()) {
			return it->second;
		}
		return Variant(); // default
	}
};