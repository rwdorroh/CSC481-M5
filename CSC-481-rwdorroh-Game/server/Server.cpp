#include <zmq.hpp>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>
#include <memory>
#include <csignal>

#include "../include/engine/GameObject.h"
#include "../include/engine/TransformComponent.h"
#include "../include/engine/NetworkComponent.h"
#include "../include/engine/RenderComponent.h"
#include "../include/engine/ColliderComponent.h"
#include "../include/engine/NetworkTypes.h"
#include "../include/engine/Types.h"
#include "../include/engine/Event.h"
#include "../include/engine/EventManager.h"

// Struct with player information server needs
struct PlayerInfo {
    float x, y; // position
    std::chrono::steady_clock::time_point lastUpdate; // time of last update
};

std::unordered_map<int, std::unique_ptr<GameObject>> syncedObjects; // list of server held objects
std::unordered_map<int, PlayerInfo> players; // list of all players
std::mutex objectsMutex;
std::mutex playersMutex;

std::atomic<bool> running{ true };
std::atomic<int> nextClientId{ 0 };

EventManager serverEventManager;

// Queue for events that need to be raised after current dispatch
std::vector<Event> pendingServerEvents;
std::mutex pendingEventsMutex;

// Function to set up event handlers inside the server, only Death and Spawn currently do anything inside the server
void setupServerEventHandlers() {
    serverEventManager.subscribe("InputPressed", [](const Event& e) {
        int playerId = e.getParam("playerId").asInt;
        int key = e.getParam("key").asInt;
        });

    serverEventManager.subscribe("Collision", [](const Event& e) {
        int playerId = e.getParam("playerId").asInt;
        int objectId = e.getParam("objectId").asInt;
        });

    serverEventManager.subscribe("Death", [](const Event& e) {
        int playerId = e.getParam("playerId").asInt;

        // Don't raise immediately - queue it to avoid deadlock
        Event spawn("Spawn");
        spawn.addParam("playerId", Variant(playerId));
        spawn.addParam("spawnIndex", Variant(0));
        spawn.priority = 1;

        std::lock_guard<std::mutex> lock(pendingEventsMutex);
        pendingServerEvents.push_back(spawn);
        });

    serverEventManager.subscribe("Spawn", [](const Event& e) {
        int playerId = e.getParam("playerId").asInt;
        int spawnIndex = e.getParam("spawnIndex").asInt;

        float spawnX = 300.f;
        float spawnY = 500.f;
        if (spawnIndex == 1) {
            spawnX = 700.f;
            spawnY = 400.f;
        }

        std::lock_guard<std::mutex> lock(playersMutex);
        if (players.find(playerId) != players.end()) {
            players[playerId].x = spawnX;
            players[playerId].y = spawnY;
            players[playerId].lastUpdate = std::chrono::steady_clock::now();
        }
        });
}

void initializeSyncedObjects() {
    std::lock_guard<std::mutex> lock(objectsMutex);

    auto platform = std::make_unique<GameObject>();
    platform->addComponent(std::make_unique<TransformComponent>(1100.f, 700.f, 200.f, 32.f, 150.f, 0.f));
    platform->addComponent(std::make_unique<NetworkComponent>(true, 0, 0));
    syncedObjects[0] = std::move(platform);

    auto orb = std::make_unique<GameObject>();
    orb->addComponent(std::make_unique<TransformComponent>(1920.f - 128.f, 0.f, 128.f, 128.f, -400.f, 180.f));
    orb->addComponent(std::make_unique<NetworkComponent>(true, 1, 1));
    syncedObjects[1] = std::move(orb);

    auto platform2 = std::make_unique<GameObject>();
    platform2->addComponent(std::make_unique<TransformComponent>(100.f, 300.f, 200.f, 32.f, 0.f, 150.f));
    platform2->addComponent(std::make_unique<NetworkComponent>(true, 2, 2));
    syncedObjects[2] = std::move(platform2);
}

void updateSyncedObjects(float deltaTime) {
    std::lock_guard<std::mutex> lock(objectsMutex);
    for (auto& [id, obj] : syncedObjects) {
        auto* t = obj->getComponent<TransformComponent>();
        auto* n = obj->getComponent<NetworkComponent>();
        if (!t || !n || !n->serverControlled) continue;

        if (n->typeId == 0) {
            Vec2 pos = t->getPosition();
            Vec2 vel = t->getVelocity();
            pos.x += vel.x * deltaTime;
            if (pos.x <= 1000.f || pos.x + 200.f >= 1500.f)
                vel.x *= -1;
            t->setPosition(pos.x, pos.y);
            t->setVelocity(vel.x, vel.y);
        }
        if (n->typeId == 1) {
            Vec2 pos = t->getPosition();
            Vec2 vel = t->getVelocity();
            pos.x += vel.x * deltaTime;
            pos.y += vel.y * deltaTime;
            if (pos.x + 128.f < 0.f || pos.y > 1080.f)
                pos = { 1920.f - 128.f, 0.f };
            t->setPosition(pos.x, pos.y);
        }
        if (n->typeId == 2) {
            Vec2 pos = t->getPosition();
            Vec2 vel = t->getVelocity();
            pos.y += vel.y * deltaTime;
            if (pos.y <= 100.f || pos.y + 200.f >= 1000.f)
                vel.y *= -1;
            t->setPosition(pos.x, pos.y);
            t->setVelocity(vel.x, vel.y);
        }
    }
}

void reply_handler(zmq::context_t& context, int clientID, int port) {
    zmq::socket_t responder(context, zmq::socket_type::rep);
    responder.bind("tcp://*:" + std::to_string(port));
    std::cout << "[Server] Listening for client " << clientID << " on port " << port << "\n";

    bool connected = true;

    while (running && connected) {
        zmq::message_t request;
        try {
            if (!responder.recv(request, zmq::recv_flags::none))
                continue;
        }
        catch (const zmq::error_t& e) {
            std::cerr << "[Server] Socket error (client " << clientID << "): " << e.what() << "\n";
            break;
        }

        std::string req(static_cast<char*>(request.data()), request.size());
        std::istringstream iss(req);
        std::string cmd;
        iss >> cmd;

        if (cmd == "CMD") {
            int id, tick;
            uint32_t actions;
            float x, y;
            int eventCount = 0;
            if (iss >> id >> tick >> actions >> x >> y >> eventCount) {
                {
                    std::lock_guard<std::mutex> lock(playersMutex);
                    players[id] = { x, y, std::chrono::steady_clock::now() };
                }

                for (int i = 0; i < eventCount; ++i) {
                    std::string line;
                    std::getline(iss >> std::ws, line);
                    std::istringstream lineStream(line);

                    std::string type;
                    int priority, paramCount;

                    if (!(lineStream >> type >> priority >> paramCount))
                        continue;

                    Event e(type);
                    e.priority = priority;

                    for (int j = 0; j < paramCount; ++j) {
                        std::string key, valType;
                        if (!(lineStream >> key >> valType)) break;

                        if (valType == "INT") {
                            int val;
                            lineStream >> val;
                            e.addParam(key, Variant(val));
                        }
                        else if (valType == "FLOAT") {
                            float val;
                            lineStream >> val;
                            e.addParam(key, Variant(val));
                        }
                    }

                    serverEventManager.raise(e, static_cast<float>(tick));
                }
            }
        }
        else if (cmd == "DISCONNECT") {
            int id;
            if (iss >> id) {
                {
                    std::lock_guard<std::mutex> lock(playersMutex);
                    players.erase(id);
                }
                std::cout << "[Server] Client " << id << " disconnected cleanly.\n";
                connected = false;
            }
        }

        try {
            static const std::string ack = "Acknowledged";
            responder.send(zmq::buffer(ack), zmq::send_flags::none);
        }
        catch (const zmq::error_t& e) {
            std::cerr << "[Server] Lost connection to client " << clientID << " (" << e.what() << ")\n";
            std::lock_guard<std::mutex> lock(playersMutex);
            players.erase(clientID);
            break;
        }
    }

    responder.close();
    std::cout << "[Server] Closed handler for client " << clientID << "\n";
}

void connection_listener(zmq::context_t& context) {
    zmq::socket_t registrar(context, zmq::socket_type::rep);
    registrar.bind("tcp://*:5550");
    std::cout << "[Server] Waiting for client registrations on tcp://*:5550\n";

    while (running) {
        zmq::message_t request;
        if (!registrar.recv(request, zmq::recv_flags::dontwait)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        int id = nextClientId++;
        int port = 5556 + id;

        std::ostringstream oss;
        oss << port;
        std::string portStr = oss.str();

        registrar.send(zmq::buffer(portStr), zmq::send_flags::none);
        std::cout << "[Server] Registered client " << id << " port " << port << "\n";

        std::thread(reply_handler, std::ref(context), id, port).detach();
    }

    registrar.close();
}

void pub_handler(zmq::context_t& context) {
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5555");
    std::cout << "[Server] Publishing updates on tcp://*:5555\n";

    initializeSyncedObjects();
    int tick = 0;

    static std::unordered_map<int, bool> previousPlayers;

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
        ++tick;
        updateSyncedObjects(0.033f);

        // Dispatch events
        serverEventManager.dispatch(static_cast<float>(tick));

        // Process any pending events that were queued during dispatch
        std::vector<Event> eventsToRaise;
        {
            std::lock_guard<std::mutex> lock(pendingEventsMutex);
            eventsToRaise = std::move(pendingServerEvents);
            pendingServerEvents.clear();
        }

        // Raise queued events and dispatch them
        for (auto& evt : eventsToRaise) {
            serverEventManager.raise(evt, static_cast<float>(tick));
        }

        if (!eventsToRaise.empty()) {
            serverEventManager.dispatch(static_cast<float>(tick));
        }

        {
            std::lock_guard<std::mutex> lock(playersMutex);
            auto now = std::chrono::steady_clock::now();
            const auto timeout = std::chrono::seconds(3);

            for (auto it = players.begin(); it != players.end(); ) {
                auto elapsed = now - it->second.lastUpdate;
                if (elapsed > timeout) {
                    std::cout << "[Server] Client " << it->first << " timed out.\n";
                    it = players.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        std::vector<std::tuple<int, float, float>> playerCopy;
        {
            std::lock_guard<std::mutex> lock(playersMutex);
            for (auto& [id, p] : players)
                playerCopy.emplace_back(id, p.x, p.y);
        }

        std::unordered_map<int, bool> currentPlayers;
        for (auto& p : playerCopy)
            currentPlayers[std::get<0>(p)] = true;

        std::vector<int> removedIds;
        for (auto& [oldId, _] : previousPlayers)
            if (currentPlayers.find(oldId) == currentPlayers.end())
                removedIds.push_back(oldId);

        previousPlayers = std::move(currentPlayers);

        std::ostringstream oss;
        oss << "SNAP " << tick << " "
            << playerCopy.size() << " "
            << syncedObjects.size() << " "
            << removedIds.size() << " ";

        for (int id : removedIds) oss << id << " ";

        for (auto& p : playerCopy)
            oss << std::get<0>(p) << " " << std::get<1>(p) << " " << std::get<2>(p) << " ";

        {
            std::lock_guard<std::mutex> lock(objectsMutex);
            for (auto& [id, obj] : syncedObjects) {
                auto* t = obj->getComponent<TransformComponent>();
                auto* n = obj->getComponent<NetworkComponent>();
                if (!t || !n) continue;
                oss << n->objectId << " " << n->typeId << " "
                    << t->getPosition().x << " " << t->getPosition().y << " ";
            }
        }

        const std::string snapshot = oss.str();
        try {
            publisher.send(zmq::buffer(snapshot), zmq::send_flags::none);
        }
        catch (const zmq::error_t& e) {
            std::cerr << "[Server] PUB send error: " << e.what() << "\n";
        }
    }

    publisher.close();
}

int main() {
    zmq::context_t context(4);

    setupServerEventHandlers();

    std::thread publisher(pub_handler, std::ref(context));
    std::thread listener(connection_listener, std::ref(context));

    ::signal(SIGINT, [](int) {
        std::cout << "\n[Server] Caught SIGINT — shutting down...\n";
        running = false;
        });

    publisher.join();
    listener.join();

    context.close();
    std::cout << "[Server] Clean shutdown.\n";
    return 0;
}