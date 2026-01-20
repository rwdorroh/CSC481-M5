#pragma once

#include <string>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>
#include "Event.h"

// Forward declaration
class EventManager {
public:
	using Handler = std::function<void(const Event&)>;

	// Register a function to handle a specific event type
	void subscribe(const std::string& eventType, Handler handler);

	// Raise an event to be handled later (queued)
	void raise(const Event& event, float timestamp);

	// Dispatch all ready events at or before currentTime
	void dispatch(float currentTime);

private:
	std::map<std::string, std::vector<Handler>> listeners;

	struct QueuedEvent {
		float time;      // Scheduled timestamp
		Event event;     // The event to dispatch

		bool operator<(const QueuedEvent& other) const {
			// Reverse order for min-heap (earliest events first)
			return time > other.time;
		}
	};

	std::priority_queue<QueuedEvent> eventQueue;
	std::mutex queueMutex;
};