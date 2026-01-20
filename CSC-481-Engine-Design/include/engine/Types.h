#pragma once

// Struct for ordered pairs with zero init
struct OrderedPair {
	float x{};
	float y{};
};

// Struct for velocity, composed of a direction vector and a magnitude.
struct Velocity {
	OrderedPair direction{};
	float magnitude{};
};