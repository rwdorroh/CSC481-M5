#include <engine/EventManager.h>

void EventManager::subscribe(const std::string& eventType, Handler handler) {
	listeners[eventType].push_back(handler);
}

void EventManager::raise(const Event& event, float timestamp) {
	std::lock_guard<std::mutex> lock(queueMutex);
	eventQueue.push(QueuedEvent{ timestamp, event });
}

void EventManager::dispatch(float currentTime) {
	std::lock_guard<std::mutex> lock(queueMutex);

	while (!eventQueue.empty() && eventQueue.top().time <= currentTime) {
		QueuedEvent qe = eventQueue.top();
		eventQueue.pop();

		auto it = listeners.find(qe.event.type);
		if (it != listeners.end()) {
			for (auto& handler : it->second) {
				handler(qe.event);
			}
		}
	}
}