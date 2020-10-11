
#include <CSCI441/OpenGLUtils.hpp>
#include <CSCI441/objects.hpp>  // for our 3D objects
#include <CSCI441/SimpleShader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>            // include GLFW framework header

// include GLM libraries and matrix functions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class MyClass {
public:
    float posx=10;
    float posy=0;
    float posz=0;
    float thata=1;
    float gama=2;
    float upsome=0;
    bool up;

    GLint triangleVAO;
    GLint size;


    MyClass(GLint VAO, int size) {
        triangleVAO = VAO;
        this->size = size;
    }

    void draw_triangle() {  // Method/function defined inside the class
        //rotateTeapot = glm::rotate(0.0, 0.75f, 0.5);
        //glm::mat4 rotateTeapot = glm::rotate(glm::mat4(2.0f), -thata, glm::vec3( gama, 2.0,0) );
        //CSCI441::SimpleShader3::pushTransformation( rotateTeapot );
        //CSCI441::SimpleShader3::setMaterialColor( glm::vec3( 0.9, 0,0) );

        CSCI441::SimpleShader3::draw(GL_TRIANGLES, triangleVAO, size);
        //CSCI441::SimpleShader3::popTransformation();

    }

    void triangered(){
        CSCI441::SimpleShader3::setMaterialColor( glm::vec3( 0.9, 0,0) );
        glm::mat4 t1 = glm::translate( glm::mat4(1.0), glm::vec3( -2.5, -2, 0 ) );
        CSCI441::SimpleShader3::pushTransformation( t1 );
        draw_triangle();
        CSCI441::SimpleShader3::popTransformation();

    }

    void triangeblue(){

        CSCI441::SimpleShader3::setMaterialColor( glm::vec3( 0, 0,0.9) );

        glm::mat4 t2 = glm::translate( glm::mat4(1.0), glm::vec3( 2.5, -2, 0  ) );
        CSCI441::SimpleShader3::pushTransformation( t2 );
        draw_triangle();
        CSCI441::SimpleShader3::popTransformation();


    }

    void trianglegreen(){

        CSCI441::SimpleShader3::setMaterialColor( glm::vec3( 0, 0.9,0) );
        glm::mat4 t3 = glm::translate( glm::mat4(1.0), glm::vec3( 1.75, 2.5, 0  ) );
        CSCI441::SimpleShader3::pushTransformation( t3 );
        draw_triangle();
        CSCI441::SimpleShader3::popTransformation();


    }

    void draw_triangleman(){
        if (up){

            upsome=5;
        }
        else{
            upsome=0;
        }

        glm::mat4 rotateTeapot = glm::rotate(glm::mat4(1.0f), thata+1.57f, CSCI441::Y_AXIS);
        CSCI441::SimpleShader3::disableLighting();


        glm::mat4 moves = glm::translate( glm::mat4(1.0), glm::vec3( posx, posy, posz ) );
        CSCI441::SimpleShader3::pushTransformation( moves );
        CSCI441::SimpleShader3::pushTransformation( rotateTeapot );
        glm::mat4 moves2 = glm::translate( glm::mat4(1.0), glm::vec3( 0, 0, upsome  ) );
        CSCI441::SimpleShader3::pushTransformation( moves2 );
        trianglegreen();
        CSCI441::SimpleShader3::popTransformation();
        triangeblue();
        CSCI441::drawSolidCube(1.0);
        triangered();
        CSCI441::SimpleShader3::enableLighting();
        CSCI441::SimpleShader3::popTransformation();
        CSCI441::SimpleShader3::popTransformation();

    }



};
