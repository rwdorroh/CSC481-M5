#include <engine/Input.h>
#include <SDL3/SDL.h>
#include <algorithm>

// Initializes the static keyboardState pointer to null.
// This pointer will be used by SDL with the current keyboard state.
const bool* Input::keyboardState = nullptr;
std::vector<bool> Input::previousKeyboardState;
int Input::numKeys = 0;

// Init keyboard state
uint32_t Input::mouseButtonState = 0;
float Input::mouseX = 0;
float Input::mouseY = 0;

// Input maps
std::unordered_map<SDL_Scancode, uint32_t> Input::keyBindings;
std::unordered_map<uint8_t, uint32_t> Input::mouseBindings;
std::vector<Input::ChordBinding> Input::chordBindings;
std::map<SDL_Scancode, Input::KeyState> Input::keyStates;

uint64_t Input::currentTime_ms = 0;

/**
 * Updates the internal keyboard and mouse state by getting the latest state.
 * This function should be called once per frame in the main game loop
 * to ensure that the keyboard state is always up-to-date.
 */
void Input::updateKeyboardState() {

	// Update time
	currentTime_ms = SDL_GetTicks();

	// Get keyboard state
	keyboardState = SDL_GetKeyboardState(&numKeys);

	// Initialize previous keyboard state if needed
	if (previousKeyboardState.empty() && numKeys > 0) {
		previousKeyboardState.resize(numKeys, false);
	}

	// Get mouse state
	mouseButtonState = SDL_GetMouseState(&mouseX, &mouseY);

	// Reset consumed flags
	resetKeyStates();

	// Update timers for keys that were just pressed
	updateTimers();
}


/**
 * Checks the stored keyboard state to see if a specific key is pressed.
 * @param key The SDL_Scancode of the key to check.
 * @return true if the key is currently pressed, false otherwise.
 */
bool Input::isKeyPressed(SDL_Scancode key) {
    if (keyboardState != nullptr && key >= 0 && key < numKeys) {
        return keyboardState[key];
    }
    return false;
}

// Checks if a key was just pressed this frame
bool Input::isKeyJustPressed(SDL_Scancode key) {
	if (keyboardState != nullptr && key >= 0 && key < numKeys) {
		return keyboardState[key] && !previousKeyboardState[key];
	}

	return false;
}

// Checks if a mouse position is currently press
bool Input::isMouseButtonPressed(uint8_t button) {
	return (mouseButtonState & SDL_BUTTON_MASK(button)) != 0;
}

// Get the current mouse position
void Input::getMousePosition(float& x, float& y) {
	x = mouseX;
	y = mouseY;
}

// Bind key to bit
void Input::bindAction(SDL_Scancode key, uint32_t bit) {
	keyBindings[key] = bit;
}

// Bind a mouse button to a bit
void Input::bindMouseAction(uint8_t button, uint32_t bit) {
	mouseBindings[button] = bit;
}

// Bind a chord to an action bit
void Input::bindChord(const std::vector<SDL_Scancode>& keys, uint32_t bit) {
	
	chordBindings.push_back({ keys, bit });

	for (auto key : keys) {
		keyStates[key] = KeyState{};
	}

	std::sort(chordBindings.begin(), chordBindings.end(),
		[](const auto& a, const auto& b) {
			return a.keys.size() > b.keys.size();
		}
	);
}

// Clear all bindings
void Input::clearBindings() {
	keyBindings.clear();
	mouseBindings.clear();
	chordBindings.clear();
	keyStates.clear();
}

// Resets the consume flag for all tracked keys
void Input::resetKeyStates() {
	for (auto& pair : keyStates) {
		pair.second.consumed = false;
	}
}

// Update press time for any tracked key that was just pressed this frame
void Input::updateTimers() {
	for (auto& pair : keyStates) {
		if (isKeyJustPressed(pair.first)) {
			pair.second.pressTime = currentTime_ms;
		}
	}
}

// Process all chord bindings and return chord action mask
uint32_t Input::processChords() {
	
	uint32_t chordMask = 0;

	// Iterate all defined chords
	for (const auto& binding : chordBindings) {
		bool chordTriggeredThisFrame = false;
		bool allKeysPending = true;
		uint64_t oldestPress = currentTime_ms;
		uint64_t newestPress = 0;

		// Check all keys in this chord
		for (auto key : binding.keys) {
			auto it = keyStates.find(key);
			if (it == keyStates.end()) {
				allKeysPending = false;
				break;
			}

			KeyState& state = it->second;

			// If any key not pressed (timer = 0), chord not complete
			if (state.pressTime == 0) {
				allKeysPending = false;
				break;
			}

			// Check if this key was the one just pressed
			if (isKeyJustPressed(key)) {
				chordTriggeredThisFrame = true;
			}

			// Find the time window
			if (state.pressTime < oldestPress) {
				oldestPress = state.pressTime;
			}

			if (state.pressTime > newestPress) {
				newestPress = state.pressTime;
			}
		}

		// Conditions to fire:
		// 1. A key in the chord was just pressed
		// 2. All keys in the chord have a valid pressTIme
		// 3. The time between the first and last key is within the chord window
		if (chordTriggeredThisFrame && allKeysPending && (newestPress - oldestPress < CHORD_WINDOW_MS)) {

			// Final check: are any of the keys already consumed by a longer chord
			bool alreadyConsumed = false;
			for (auto key : binding.keys) {
				if (keyStates[key].consumed) {
					alreadyConsumed = true;
					break;
				}
			}

			if (!alreadyConsumed) {

				// Set the action bit for this chord
				chordMask |= (1u << binding.actionBit);

				// Consume and reset all keys in this chord
				for (auto key : binding.keys) {
					keyStates[key].consumed = true;
					keyStates[key].pressTime = 0;
				}
			}
		}
	}
	return chordMask;
}


// Get the action mask for the game
uint32_t Input::getActionMask() {
	uint32_t mask = 0;

	if (keyboardState == nullptr) {
		return mask;
	}

	// First check for chords
	mask |= processChords();

	// Add individual key bindings (if not in chord)
	for (auto& [key, bit] : keyBindings) {

		// Check if this key is part of a chord and was consumed
		auto it = keyStates.find(key);
		bool isConsumed = (it != keyStates.end() && it->second.consumed);

		// Only set the bit if the key is pressed AND not consued
		if (!isConsumed && keyboardState[key]) {
			mask |= (1u << bit);
		}
	}

	// Add mouse button bindings
	for (auto& [button, bit] : mouseBindings) {
		if (isMouseButtonPressed(button)) {
			mask |= (1u << bit);
		}
	}

	// Save previous keyboard state for next frame
	if (numKeys > 0) {
		for (int i = 0; i < numKeys; i++) {
			previousKeyboardState[i] = keyboardState[i];
		}
	}

	return mask;
}
