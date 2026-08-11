#pragma once
#include <glad/glad.h>
#include <iostream>

#define DBG std::cout << "DBG: " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << std::endl;
 
struct DrawElementsIndirectCommand {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
};