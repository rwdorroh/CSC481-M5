#pragma once
#include <zmq.hpp>
#include <string>
#include "NetworkTypes.h"

// ClientNetwork provides a simple wrapper around ZeroMQ REQ/SUB sockets.
// - REQ is used to send move updates to the server.
// - SUB is used to receive position updates for all players.
class Client {
public:
    Client();
    ~Client();

    // Connect to the server (default is localhost)
    bool connect(const std::string& serverAddress = "tcp://localhost");

    // Send this client's command to the server
	void sendCommand(const ClientCommand& cmd);

    // Poll for updates from the server (non-blocking)
    // Returns true if snapshot was received
    bool pollUpdate(WorldSnapshot& outSnapshot);

    static void setClientID(int id);

    // Effectively to check if server is connected
    bool hasReceivedSnapshot() const;

	void disconnect();


private:
    zmq::context_t context;
    zmq::socket_t requester;   // REQ socket (send commands, receive server acknowledgement)
    zmq::socket_t subscriber;  // SUB socket (receive world snapshots)
    static int clientID;

	bool awaitingReply = false;

    int latestTick = 0;
};
