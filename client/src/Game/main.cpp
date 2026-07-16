#include <Game/Game.hpp>

int main(){

    try {
        Game game;
        while(!game.connectToServer("127.0.0.1"));
        std::cout << "Connected to server!" << std::endl;
        game.run();
    } 
    catch (const std::exception& e){
        std::cerr << "Fatal Error during startup: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}