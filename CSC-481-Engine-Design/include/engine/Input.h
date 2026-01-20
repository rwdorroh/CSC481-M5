#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <map>

class Input {
public:
    // Check if a key is currently being pressed.
    static bool isKeyPressed(SDL_Scancode key);

	// Check if mouse button is currently being pressed
	static bool isMouseButtonPressed(uint8_t button);

	// Get mouse poisition
	static void getMousePosition(float& x, float& y);

    // Grabs the latest keyboard state. Call this once per frame.
	static void updateKeyboardState();

	// Bind a key to an action bit index (0 to 31)
	static void bindAction(SDL_Scancode key, uint32_t bit);

	// Bind a mouse button to an action bit index
	static void bindMouseAction(uint8_t button, uint32_t bit);

	// Bind a chord to an action bit
	static void bindChord(const std::vector<SDL_Scancode>& keys, uint32_t bit);

	// Clear all bindings
	static void clearBindings();

	// Build an action mask for this fram
	static uint32_t getActionMask();

private:
	struct ChordBinding {
		std::vector<SDL_Scancode> keys;
		uint32_t actionBit;
	};

	struct KeyState {
		uint64_t pressTime = 0;
		bool consumed = false;
	};

    // A list of all keys on the keyboard and whether each one is pressed or not.
    static const bool* keyboardState;
	static std::vector<bool> previousKeyboardState;
	static int numKeys;

	// Mouse State
	static uint32_t mouseButtonState;
	static float mouseX;
	static float mouseY;

	// Map from SDL key to which bit it controls
	static std::unordered_map<SDL_Scancode, uint32_t> keyBindings;

	// Map from mouse button to action bit
	static std::unordered_map<uint8_t, uint32_t> mouseBindings;

	// Chord bindings, sorted largets first
	static std::vector<ChordBinding> chordBindings;

	// Track key states for chord detection
	static std::map<SDL_Scancode, KeyState> keyStates;

	// Current time in ms
	static uint64_t currentTime_ms;

	// Chord winow
	static constexpr uint32_t CHORD_WINDOW_MS = 200;

	// Helper functions
	static bool isKeyJustPressed(SDL_Scancode key);
	static void updateTimers();
	static void resetKeyStates();
	static uint32_t processChords();
};