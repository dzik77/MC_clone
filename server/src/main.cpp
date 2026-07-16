#include <Server.hpp>

int main(){
    
    try {
        Server server;
        server.run();
    } 
    catch (const std::exception& e){
        std::cerr << "Fatal Error during startup: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}