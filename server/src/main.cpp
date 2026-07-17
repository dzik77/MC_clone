#include <Server.hpp>
#include <cstdlib>

int main(){
    
    try {
        Server server;
        server.run();
        std::_Exit(0);
    } 
    catch (const std::exception& e){
        std::cerr << "Fatal Error during startup: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}