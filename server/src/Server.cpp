#include "Server.hpp"

Server::Server() {
    if (enet_initialize() != 0) {
        throw std::runtime_error("Failed to initialize ENet");
    }

    address.host = ENET_HOST_ANY; // Bind to all available interfaces
    address.port = 1234; // Set the port number

    server = enet_host_create(&address, 32, 2, 0, 0); // Create the server host
    if (server == nullptr) {
        throw std::runtime_error("Failed to create ENet server host");
    }

    std::cout << "Server initialized and listening on port " << address.port << std::endl;

}

Server::~Server() {
    enet_host_destroy(server);
    enet_deinitialize();    
}

void Server::run() {
    
    running = true;
    consoleThread = std::thread(&Server::consoleInput, this);

    while(running) {

        ENetEvent event;
        while (enet_host_service(server, &event, 50) > 0){
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "A new client connected from "
                              << event.peer->address.host << ":" << event.peer->address.port
                              << std::endl;
                    event.peer->data = (void*)"Player 1";
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << (char *)event.peer->data << " disconnected." << std::endl;
                    event.peer->data = nullptr;
                    break;
                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }

        // input handling
        std::string command = "";
        {
            std::lock_guard<std::mutex> lock(commandMutex);
            if (!commandQueue.empty()) {
                command = commandQueue.front();
                commandQueue.pop();
            }
        }

        // command processing
        if (!command.empty()) {
            if (command == "exit") {
                std::cout << "Shutting down server..." << std::endl;
                running = false;
            } else {
                std::cout << "Unknown command: " << command << std::endl;
            }
        }

    }

    consoleThread.detach();

}

void Server::consoleInput() {

    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (!line.empty()) {
            std::lock_guard<std::mutex> lock(commandMutex);
            commandQueue.push(line);
        }
    }

}