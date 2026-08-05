#include <Game.hpp>

int main(){

    try {
        Game game;
        game.run();
    } 
    catch (const std::exception& e){
        std::cerr << "Fatal Error during startup: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}