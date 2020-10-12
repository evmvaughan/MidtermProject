
//*************************************************************************************

// include OpenGL and GLFW libraries
#include <GL/glew.h>                    // include GLEW to get our OpenGL 3.0+ bindings
#include <GLFW/glfw3.h>			        // include GLFW framework header

// include GLM libraries and matrix functions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// include C/C++ libraries
#include <cstdio>				        // for printf functionality
#include <cstdlib>				        // for exit functionality
#include <vector>                       // for vector functionality

// include our class CSCI441 libraries
#include <CSCI441/OpenGLUtils.hpp>      // to print info about OpenGL
#include <CSCI441/objects.hpp>          // to render our 3D primitives
#include <CSCI441/ShaderProgram.hpp>    // a wrapper class for Shader Programs

//*************************************************************************************
//
// Global Parameters

// global variables to keep track of window width and height.
// set to initial values for convenience
const GLint WINDOW_WIDTH = 640, WINDOW_HEIGHT = 480;

GLboolean leftMouseDown;    	 		// status ff the left mouse button is pressed
glm::vec2 mousePosition;				// last known X and Y of the mouse


GLboolean keys[256] = {0};              // keep track of our key states

GLfloat propAngle;                      // angle of rotation for our plane propeller

struct BuildingData {                   // stores the information unique to a single building
    glm::mat4 modelMatrix;                  // the translation/scale of each building
    glm::vec3 color;                        // the color of each building
};
std::vector<BuildingData> buildings;    // information for all of our buildings

GLuint groundVAO;                       // the VAO descriptor for our ground plane

// Shader Program information
CSCI441::ShaderProgram *lightingShader = nullptr;   // the wrapper for our shader program
struct LightingShaderUniforms {         // stores the locations of all of our shader uniforms
    GLint mNormal;
    GLint lightDirection;
    GLint lightColor;
    GLint mvpMatrix;
    GLint materialColor;
} lightingShaderUniforms;
struct LightingShaderAttributes {       // stores the locations of all of our shader attributes
    GLint vNormal;
    GLint vPos;
} lightingShaderAttributes;

struct TreeTrunkData {                       // keeps track of an individual building's attributes
    glm::mat4 modelMatrix;                      // its position and size
    glm::vec3 color;                            // its color
};

struct TreeLeavesData {
    glm::mat4 modelMatrix;
    glm::vec3 color;
};

std::vector<TreeTrunkData> treeTrunks;        // stores all of our building information
std::vector<TreeLeavesData> treeLeafLayer1;
std::vector<TreeLeavesData> treeLeafLayer2;
std::vector<TreeLeavesData> treeLeafLayer3;


// Camera definitions

bool firstPerson = true;
bool arcBall;
bool freeCam;

GLdouble cameraTheta, cameraPhi;            // camera DIRECTION in spherical coordinates

glm::vec3 camPos;            			// camera position in cartesian coordinates
glm::vec3 camAngles;               		// camera DIRECTION in spherical coordinates stored as (theta, phi, radius)
glm::vec3 camDir; 			            // camera DIRECTION in cartesian coordinates
glm::vec2 cameraSpeed;                  // cameraSpeed --> x = forward/backward delta, y = rotational delta

int cameraTypeRotation = 0;

// Hero definitions

enum heros {
    NotEvanVaughan,
    TriangleMan
} selectedHero;

// Not Evan Vaughan

float NotEvanVaughanXLocation = 0;
float NotEvanVaughanYLocation = 0;

int carWidth = 4;
int carLength = 8;

float carSpeed = 0;
float carRotation = - float(M_PI/2);
float rotateWheelSpeed = carSpeed / 2;
float NotEvanVaughanMotion = 0;


//*************************************************************************************
//
// Helper Functions

// getRand() ///////////////////////////////////////////////////////////////////
//
//  Simple helper function to return a random number between 0.0f and 1.0f.
//
////////////////////////////////////////////////////////////////////////////////
GLfloat getRand() {
    return (GLfloat)rand() / (GLfloat)RAND_MAX;
}

// updateCameraDirection() /////////////////////////////////////////////////////
//
// This function updates the camera's position in cartesian coordinates based
//  on its position in spherical coordinates. Should be called every time
//  cameraTheta, cameraPhi, or cameraRadius is updated.
//
////////////////////////////////////////////////////////////////////////////////
void updateCameraDirection() {

    // ensure phi doesn't flip our camera
    if( camAngles.y <= 0 ) camAngles.y = 0.0f + 0.001f;
    if( camAngles.y >= M_PI ) camAngles.y = M_PI - 0.001f;

    if (arcBall) {
        // convert from spherical to cartesian in our RH coord. sys.
        camDir.x = camAngles.z * sinf(camAngles.x) * sinf(camAngles.y);
        camDir.y = camAngles.z * cosf(camAngles.y);
        camDir.z = camAngles.z * -cosf(camAngles.x) * sinf(camAngles.y);

        // normalize the direction for a free cam
        camDir = glm::normalize(camDir);
        camDir *= 20;
    } else if (firstPerson) {
        // convert from spherical to cartesian in our RH coord. sys.
        camDir.x = camAngles.z * sinf(camAngles.x) * sinf(camAngles.y);
        camDir.y = camAngles.z * -cosf(camAngles.y);
        camDir.z = camAngles.z * -cosf(camAngles.x) * sinf(camAngles.y);

        // normalize the direction for a free cam
        camDir = glm::normalize(camDir);
    } else {
        // convert from spherical to cartesian in our RH coord. sys.
        camDir.x = camAngles.z * sinf(camAngles.x) * sinf(camAngles.y);
        camDir.y = camAngles.z * -cosf(camAngles.y);
        camDir.z = camAngles.z * -cosf(camAngles.x) * sinf(camAngles.y);

        // normalize the direction for a free cam
        camDir = glm::normalize(camDir);
    }

}

// computeAndSendMatrixUniforms() ///////////////////////////////////////////////
//
// This function precomputes the matrix uniforms CPU-side and then sends them
// to the GPU to be used in the shader for each vertex.  It is more efficient
// to calculate these once and then use the resultant product in the shader.
//
////////////////////////////////////////////////////////////////////////////////
void computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {
    // precompute the Model-View-Projection matrix on the CPU
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    // then send it to the shader on the GPU to apply to every vertex
    glUniformMatrix4fv( lightingShaderUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0] );

    glm::mat3 normalMtx = glm::mat3( glm::transpose( glm::inverse( modelMtx ) ) );
    glUniformMatrix3fv( lightingShaderUniforms.mNormal, 1, GL_FALSE, &normalMtx[0][0] );

}

//*************************************************************************************
//
// Event Callbacks

// error_callback() /////////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's error callback.
//	When an error within GLFW occurs, GLFW will tell us by calling
//	this function.  We can then print this info to the terminal to
//	alert the user.
//
////////////////////////////////////////////////////////////////////////////////
static void error_callback( int error, const char* description ) {
	fprintf( stderr, "[ERROR]: (%d) %s\n", error, description );
}

// keyboard_callback() /////////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's keyboard callback.
//	We only log if a key was pressed/released and will handle the actual event
//  updates later in updateScene().
//      Pressing Q or ESC does close the window and quit the program.
//
////////////////////////////////////////////////////////////////////////////////
static void keyboard_callback( GLFWwindow *window, int key, int scancode, int action, int mods ) {
	if( action == GLFW_PRESS ) {
	    // store that a key had been pressed
	    keys[key] = GL_TRUE;

		switch( key ) {
		    // close our window and quit our program
			case GLFW_KEY_ESCAPE:
			case GLFW_KEY_Q:
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
		    case GLFW_KEY_C:

		        if (cameraTypeRotation > 2) {
		            cameraTypeRotation = 0;
		        }

                if (cameraTypeRotation == 0) {
                    freeCam = true;
                    firstPerson = false;
                    arcBall = false;

                } else if (cameraTypeRotation == 1) {
                    arcBall = true;
                    firstPerson = false;
                    freeCam = false;

                } else if (cameraTypeRotation == 2) {
                    firstPerson = true;
                    freeCam = false;
                    arcBall = false;

		        }
                camAngles.x = -M_PI / 3.0f;
                camAngles.y = M_PI / 2.8f;
                updateCameraDirection();

                cameraTypeRotation++;

            default:
		        break;
		}
	} else if( action == GLFW_RELEASE ) {
	    // store that a key is no longer being pressed
	    keys[key] = GL_FALSE;
	}
}

// cursor_callback() ///////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's cursor movement callback.
//	Responds to mouse movement.
//
////////////////////////////////////////////////////////////////////////////////
static void cursor_callback( GLFWwindow* window, double xPos, double yPos) {
    // make sure movement is in bounds of the window
    // glfw captures mouse movement on entire screen
    if( xPos > 0 && xPos < WINDOW_WIDTH ) {
        if( yPos > 0 && yPos < WINDOW_HEIGHT ) {
            // active motion
            if( leftMouseDown ) {
                // ensure we've moved at least one pixel to register a movement from the click
                if( !((mousePosition.x - -9999.0f) < 0.001f) ) {
                    camAngles.x += (xPos - mousePosition.x) * 0.005f;
                    camAngles.y += (mousePosition.y - yPos) * 0.005f;

                    updateCameraDirection();
                }
                mousePosition = glm::vec2( xPos, yPos);
            }
            // passive motion
            else {

            }
        }
    }
}

// mouse_button_callback() /////////////////////////////////////////////////////
//
//		We will register this function as GLFW's mouse button callback.
//	Responds to mouse button presses and mouse button releases.
//
////////////////////////////////////////////////////////////////////////////////
static void mouse_button_callback( GLFWwindow *window, int button, int action, int mods ) {
	if( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        leftMouseDown = GL_TRUE;
  } else {
        leftMouseDown = GL_FALSE;
	    mousePosition = glm::vec2(-9999.0f, -9999.0f);
	}
}

//*************************************************************************************
//
// Drawing Funcs

void drawWheel(int wheelNumber,  glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) {

    float x_location = 0;
    float z_location = 0;

    float wheelFacing = float(M_PI / 2);

    switch (wheelNumber) {
        case 1:
            x_location = -carWidth / 2;
            z_location = +carLength / 2;
            break;
        case 2:
            x_location = -carWidth / 2;
            z_location = -carLength / 2;
            break;

        case 3:
            x_location = carWidth / 2;
            z_location = carLength / 2;
            wheelFacing += float(M_PI);
            break;

        case 4:
            x_location = carWidth / 2;
            z_location = -carLength / 2;
            wheelFacing += float(M_PI);

            break;

        default:
            break;
    }

    modelMtx = glm::translate(modelMtx, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(x_location, 0.0f, z_location));

    modelMtx1 = glm::scale(modelMtx1, glm::vec3(1.0f, 1.0f, 1.0f));
    modelMtx1 = glm::rotate(modelMtx1, wheelFacing, CSCI441::Z_AXIS);
    modelMtx1 = glm::rotate(modelMtx1, float(M_PI) + rotateWheelSpeed, CSCI441::Y_AXIS);

    computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);

    glm::vec3 bodyColor(0.2f, 0.2f, 0.2f);
    glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
    CSCI441::drawSolidCylinder(1.0, 1.0, 1.0, 10, 10);

    glm::mat4 modelMtx2 = glm::translate(modelMtx, glm::vec3(x_location, 0.0f, z_location));

    modelMtx2 = glm::scale(modelMtx2, glm::vec3(1.0f, 1.0f, 1.0f));
    modelMtx2 = glm::rotate(modelMtx2, wheelFacing, CSCI441::Y_AXIS);
    modelMtx2 = glm::rotate(modelMtx2, float(M_PI) + rotateWheelSpeed, CSCI441::Z_AXIS);

    computeAndSendMatrixUniforms(modelMtx2, viewMtx, projMtx);

    glm::vec3 bodyColor1(0.2f, 0.2f, 0.2f);
    glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor1[0]);
    CSCI441::drawSolidDisk(0.2f, 1.0f, 10, 1);
}

void drawCarBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {

    modelMtx = glm::translate(modelMtx, glm::vec3(-1.5, 0.5f * sin(NotEvanVaughanMotion) + 1.0f, -3.5));

    modelMtx = glm::scale(modelMtx, glm::vec3(1.0f, 1.0f, 1.0f));

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 8; row++) {
            glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(col, 1.0f, row));
            computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
            glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
            glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
            CSCI441::drawSolidCubeFlat(1);
        }
    }

    for (int col = 0; col < 4; col++) {
        glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(col, 2.0f, 7.0f));
        computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
        glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
        CSCI441::drawSolidCubeFlat(1);
    }
//
    for (int col = 0; col < 4; col++) {
        glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(col, 3.0f, 6.0f));
        computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
        glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
        CSCI441::drawSolidCubeFlat(1);
    }
//
    for (int col = 0; col < 2; col++) {

        glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(col + 1.0f, 2.0f, 0));
        computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
        glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
        CSCI441::drawSolidCubeFlat(1);

    }
//
    for (int row = 0; row < 7; row++) {
        glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(0, 2.0f, row));
        computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
        glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
        CSCI441::drawSolidCubeFlat(1);

    }
//
    for (int row = 0; row < 7; row++) {

        glm::mat4 modelMtx1 = glm::translate(modelMtx, glm::vec3(3.0f, 2.0f, row));
        computeAndSendMatrixUniforms(modelMtx1, viewMtx, projMtx);
        glm::vec3 bodyColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &bodyColor[0]);
        CSCI441::drawSolidCubeFlat(1);

    }

}

void drawNotEvanVaughan(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {

    modelMtx = glm::translate(modelMtx, glm::vec3(NotEvanVaughanXLocation, 0.0f, NotEvanVaughanYLocation));
    modelMtx = glm::rotate(modelMtx, carRotation, CSCI441::Y_AXIS);

    drawCarBody(modelMtx, viewMtx, projMtx);

    drawWheel(1, modelMtx, viewMtx, projMtx);
    drawWheel(2, modelMtx, viewMtx, projMtx);
    drawWheel(3, modelMtx, viewMtx, projMtx);
    drawWheel(4, modelMtx, viewMtx, projMtx);
}

// renderScene() ///////////////////////////////////////////////////////////////
//
//  Responsible for drawing all of our objects that make up our world.  Must
//      use the corresponding Shaders, VAOs, and set uniforms as appropriate.
//
////////////////////////////////////////////////////////////////////////////////
void renderScene( glm::mat4 viewMtx, glm::mat4 projMtx )  {
    // use our lighting shader program
    lightingShader->useProgram();

    //// BEGIN DRAWING THE GROUND PLANE ////
    // draw the ground plane
    glm::mat4 groundModelMtx = glm::scale( glm::mat4(1.0f), glm::vec3(55.0f, 1.0f, 55.0f));
    computeAndSendMatrixUniforms(groundModelMtx, viewMtx, projMtx);

    glm::vec3 groundColor(0.3f, 0.8f, 0.2f);
    glUniform3fv(lightingShaderUniforms.materialColor, 1, &groundColor[0]);

    glBindVertexArray(groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    //// END DRAWING THE GROUND PLANE ////

    //// BEGIN DRAWING THE BUILDINGS ////
    for (int i = 0; i < treeTrunks.size(); i++) {

        TreeTrunkData currentTrunk = treeTrunks.at(i);
        TreeLeavesData currentLeafLayer1 = treeLeafLayer1.at(i);
        TreeLeavesData currentLeafLayer2 = treeLeafLayer2.at(i);
        TreeLeavesData currentLeafLayer3 = treeLeafLayer3.at(i);

        computeAndSendMatrixUniforms(currentTrunk.modelMatrix, viewMtx, projMtx);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &currentTrunk.color[0]);
        CSCI441::drawSolidCube(1.0);

        computeAndSendMatrixUniforms(currentLeafLayer1.modelMatrix, viewMtx, projMtx);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &currentLeafLayer1.color[0]);
        CSCI441::drawSolidCube(1.0);

        computeAndSendMatrixUniforms(currentLeafLayer2.modelMatrix, viewMtx, projMtx);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &currentLeafLayer2.color[0]);
        CSCI441::drawSolidCube(1.0);

        computeAndSendMatrixUniforms(currentLeafLayer3.modelMatrix, viewMtx, projMtx);
        glUniform3fv(lightingShaderUniforms.materialColor, 1, &currentLeafLayer3.color[0]);
        CSCI441::drawSolidCube(1.0);

//        CSCI441::SimpleShader3::pushTransformation(currentTrunk.modelMatrix);
//        CSCI441::SimpleShader3::setMaterialColor(currentTrunk.color);
//        CSCI441::drawSolidCube(1.0);
//        CSCI441::SimpleShader3::popTransformation();
//
//        CSCI441::SimpleShader3::pushTransformation(currentLeafLayer1.modelMatrix);
//        CSCI441::SimpleShader3::setMaterialColor(currentLeafLayer1.color);
//        CSCI441::drawSolidCube(1.0);
//        CSCI441::SimpleShader3::popTransformation();
//
//        CSCI441::SimpleShader3::pushTransformation(currentLeafLayer2.modelMatrix);
//        CSCI441::SimpleShader3::setMaterialColor(currentLeafLayer2.color);
//        CSCI441::drawSolidCube(1.0);
//        CSCI441::SimpleShader3::popTransformation();
//
//        CSCI441::SimpleShader3::pushTransformation(currentLeafLayer3.modelMatrix);
//        CSCI441::SimpleShader3::setMaterialColor(currentLeafLayer3.color);
//        CSCI441::drawSolidCube(1.0);
//        CSCI441::SimpleShader3::popTransformation();
    }


//    for( const BuildingData& currentBuilding : buildings ) {
//        computeAndSendMatrixUniforms(currentBuilding.modelMatrix, viewMtx, projMtx);
//
//        glUniform3fv(lightingShaderUniforms.materialColor, 1, &currentBuilding.color[0]);
//
//        CSCI441::drawSolidCube(1.0);
//    }
    //// END DRAWING THE BUILDINGS ////

    //// BEGIN DRAWING THE HERO ////
    glm::mat4 modelMtx(1.0f);

    drawNotEvanVaughan(modelMtx, viewMtx, projMtx);

    // we are going to cheat and use our look at point to place our plane so it is always in view
//    modelMtx = glm::translate( modelMtx, camPos+camDir );
//    // rotate the plane with our camera theta direction (we need to rotate the opposite direction so we always look at the back)
//    modelMtx = glm::rotate( modelMtx, -camAngles.x, CSCI441::Y_AXIS );
//    // rotate the plane with our camera phi direction
//    modelMtx = glm::rotate( modelMtx,  camAngles.y, CSCI441::X_AXIS );
    // draw our plane now
//    drawPlane( modelMtx, viewMtx, projMtx );
    //// END DRAWING THE PLANE ////
}

// updateScene() ////////////////////////////////////////////////////////////////
//
//  Responsible for updating any of the attributes/parameters for any of our
//      world objects.  This includes any objects that should be animated AND
//      updating our camera.
//
////////////////////////////////////////////////////////////////////////////////
void updateScene() {

    // Character movements

    // turn right
    if( keys[GLFW_KEY_D] ) {
        if (selectedHero == NotEvanVaughan) {
            carRotation -= 0.05f;
        }
        if (firstPerson) {
            camAngles.x  += 0.05f;
            updateCameraDirection();
        }
    }
    // turn left
    if( keys[GLFW_KEY_A] ) {
        if (selectedHero == NotEvanVaughan)  {
            carRotation += 0.05f;
        }
        if (firstPerson) {
            camAngles.x -= 0.05f;
            updateCameraDirection();
        }
    }
    // pitch up
    if( keys[GLFW_KEY_W] ) {
        if (selectedHero == NotEvanVaughan) {
            rotateWheelSpeed -= 0.5f;
            if (NotEvanVaughanYLocation < 50 || NotEvanVaughanYLocation + cos(carRotation) < 50) {
                if (NotEvanVaughanYLocation + cos(carRotation) > -50) {
                    NotEvanVaughanYLocation += cos(carRotation) * 0.5;
                }
            }
            if (NotEvanVaughanXLocation < 50 || NotEvanVaughanXLocation + sin(carRotation) < 50) {
                if (NotEvanVaughanXLocation + sin(carRotation) > -50) {
                    NotEvanVaughanXLocation += sin(carRotation)  * 0.5;
                }
            }
        }
    }
    // pitch down
    if( keys[GLFW_KEY_S] ) {
        if (selectedHero == NotEvanVaughan) {
            rotateWheelSpeed += 0.5f;
            if (NotEvanVaughanYLocation > -50 || NotEvanVaughanYLocation - cos(carRotation) > -50) {
                if (NotEvanVaughanYLocation - cos(carRotation) < 50) {
                    NotEvanVaughanYLocation -= cos(carRotation)  * 0.5;
                }
            }
            if (NotEvanVaughanXLocation > -50 || NotEvanVaughanXLocation - sin(carRotation) > -50) {
                if (NotEvanVaughanXLocation - sin(carRotation) < 50) {
                    NotEvanVaughanXLocation -= sin(carRotation) * 0.5;
                }
            }
        }
    }

    // Free Camera Movements
    // go forward
    if( keys[GLFW_KEY_SPACE] && freeCam) {
        camPos += camDir * cameraSpeed.x;

        // rotate the propeller to make the plane fly!
        propAngle += M_PI/16.0f;
        if( propAngle > 2*M_PI ) propAngle -= 2*M_PI;
    }
    // go backward
    if( keys[GLFW_KEY_X] && freeCam ) {
        camPos -= camDir * cameraSpeed.x;

        // rotate the propeller to make the plane fly!
        propAngle -= M_PI/16.0f;
        if( propAngle < 0 ) propAngle += 2*M_PI;
    }
}

//*************************************************************************************
//
// Setup Functions

// generateEnvironmentDL() /////////////////////////////////////////////////////
//
//  This function creates all the static scenery in our world.
//
////////////////////////////////////////////////////////////////////////////////
void generateEnvironmentDL() {
    const GLint GRID_WIDTH = 100;
    const GLint GRID_LENGTH = 100;
    const GLfloat GRID_SPACING = 1.1f;
    const GLint GRID_START = - GRID_WIDTH/2;
    const GLint GRID_END = GRID_WIDTH/2;

    //everything's on a grid.
//    for( int i = 0; i < GRID_WIDTH - 1; i++) {
//        for( int j = 0; j < GRID_LENGTH - 1; j++) {
//            //don't just draw a building ANYWHERE.
//            if( i % 2 && j % 2 && getRand() < 0.4f ) {
//                // translate to spot
//                // compute random height
//                GLdouble height = powf(getRand(), 2.5)*10 + 1;  //center buildings are bigger!
//                // translate up to grid plus make them float slightly to avoid depth fighting/aliasing
//                modelMtx = glm::translate( modelMtx, glm::vec3(0, height/2.0f + 0.1, 0) );
//                // scale to building size
//                modelMtx = glm::scale( modelMtx, glm::vec3(1, height, 1) );
//
//                // compute random color
//                glm::vec3 color( getRand(), getRand(), getRand() );
//
//                // store building properties
//                BuildingData currentBuilding;
//                currentBuilding.modelMatrix = modelMtx;
//                currentBuilding.color = color;
//                buildings.emplace_back( currentBuilding );
//            }
//        }
//    }

//    glm::mat4 modelMtx = glm::translate( glm::mat4(1.0), glm::vec3(((GLfloat) - GRID_WIDTH / 2.0f) * GRID_SPACING, 0.0f, ((GLfloat) - GRID_LENGTH / 2.0f) * GRID_SPACING) );


    for (int row = GRID_START; row < GRID_END; row++) {
        for (int column = GRID_START; column < GRID_END; column++) {
            if (row % 2 == 0 && column % 2 == 0 && getRand() < 0.05) {

                float treeHeight = getRand() * 20;

                glm::mat4 trunkScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, treeHeight / 8, 1.0f));
                glm::mat4 trunkTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(row, 0.0f, column));
                glm::mat4 trunkHeightTranslationMatrix = glm::translate(glm::mat4(1.0f),
                                                                        glm::vec3(0.0f, treeHeight / 8, 0.0f));

                TreeTrunkData treeTrunk = {trunkHeightTranslationMatrix * trunkTranslationMatrix * trunkScaleMatrix,
                                           glm::vec3(0.38, 0.2, 0.07)};

                glm::mat4 leaf1ScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, treeHeight / 8, 2.0f));
                glm::mat4 leaf1TranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(row, 0.0f, column));
                glm::mat4 leaf1HeightTranslationMatrix = glm::translate(glm::mat4(1.0f),
                                                                        glm::vec3(0.0f, 2 * treeHeight / 8, 0.0f));

                TreeLeavesData treeLeaf1 = {leaf1HeightTranslationMatrix * leaf1TranslationMatrix * leaf1ScaleMatrix,
                                            glm::vec3(0, 1, 0)};

                glm::mat4 leaf2ScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, treeHeight / 8, 1.5f));
                glm::mat4 leaf2TranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(row, 0.0f, column));
                glm::mat4 leaf2HeightTranslationMatrix = glm::translate(glm::mat4(1.0f),
                                                                        glm::vec3(0.0f, 3 * treeHeight / 8, 0.0f));

                TreeLeavesData treeLeaf2 = {leaf2HeightTranslationMatrix * leaf2TranslationMatrix * leaf2ScaleMatrix,
                                            glm::vec3(0, 1, 0)};

                glm::mat4 leaf3ScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, treeHeight / 8, 1.0f));
                glm::mat4 leaf3TranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(row, 0.0f, column));
                glm::mat4 leaf3HeightTranslationMatrix = glm::translate(glm::mat4(1.0f),
                                                                        glm::vec3(0.0f, 4 * treeHeight / 8, 0.0f));

                TreeLeavesData treeLeaf3 = {leaf3HeightTranslationMatrix * leaf3TranslationMatrix * leaf3ScaleMatrix,
                                            glm::vec3(0, 1, 0)};

                treeTrunks.push_back(treeTrunk);
                treeLeafLayer1.push_back(treeLeaf1);
                treeLeafLayer2.push_back(treeLeaf2);
                treeLeafLayer3.push_back(treeLeaf3);

            }
        }
    }
}

// setupGLFW() //////////////////////////////////////////////////////////////////
//
//      Used to setup everything GLFW related.  This includes the OpenGL context
//	and our window.
//
////////////////////////////////////////////////////////////////////////////////
GLFWwindow* setupGLFW() {
	// set what function to use when registering errors
	// this is the ONLY GLFW function that can be called BEFORE GLFW is initialized
	// all other GLFW calls must be performed after GLFW has been initialized
	glfwSetErrorCallback( error_callback );

	// initialize GLFW
	if( !glfwInit() ) {
		fprintf( stderr, "[ERROR]: Could not initialize GLFW\n" );
		exit( EXIT_FAILURE );
	} else {
		fprintf( stdout, "[INFO]: GLFW initialized\n" );
	}

    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );					// request forward compatible OpenGL context
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );	    // request OpenGL Core Profile context
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );	                // request OpenGL v4.X
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );	                // request OpenGL vX.1
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );		                    // do not allow our window to be able to be resized
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );                         // request double buffering

	// create a window for a given size, with a given title
	GLFWwindow *window = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "Lab05 - Flight Simulator v0.41 alpha", nullptr, nullptr );
	if( !window ) {						// if the window could not be created, NULL is returned
		fprintf( stderr, "[ERROR]: GLFW Window could not be created\n" );
		glfwTerminate();
		exit( EXIT_FAILURE );
	} else {
		fprintf( stdout, "[INFO]: GLFW Window created\n" );
	}

	glfwMakeContextCurrent(window);		                                    // make the created window the current window
	glfwSwapInterval(1);				     	                    // update our screen after at least 1 screen refresh

	glfwSetKeyCallback( window, keyboard_callback );						// set our keyboard callback function
	glfwSetCursorPosCallback( window, cursor_callback );					// set our cursor position callback function
	glfwSetMouseButtonCallback( window, mouse_button_callback );	        // set our mouse button callback function

	return window;						                                    // return the window that was created
}

// setupOpenGL() ///////////////////////////////////////////////////////////////
//
//      Used to setup everything OpenGL related.
//
////////////////////////////////////////////////////////////////////////////////
void setupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                            // enable depth testing
    glDepthFunc( GL_LESS );							                        // use less than depth test

    glEnable(GL_BLEND);									                    // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	                    // use one minus blending equation

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	        // clear the frame buffer to black
}

// setupGLEW() /////////////////////////////////////////////////////////////////
//
//      Used to initialize GLEW
//
////////////////////////////////////////////////////////////////////////////////
void setupGLEW() {
    glewExperimental = GL_TRUE;
    GLenum glewResult = glewInit();

    // check for an error
    if( glewResult != GLEW_OK ) {
        printf( "[ERROR]: Error initializing GLEW\n");
        // Problem: glewInit failed, something is seriously wrong.
        fprintf( stderr, "[ERROR]: %s\n", glewGetErrorString(glewResult) );
        exit(EXIT_FAILURE);
    } else {
        fprintf( stdout, "[INFO]: GLEW initialized\n" );
    }
}

// setupShaders() //////////////////////////////////////////////////////////////
//
//      Create our shaders.  Send GLSL code to GPU to be compiled.  Also get
//  handles to our uniform and attribute locations.
//
////////////////////////////////////////////////////////////////////////////////
void setupShaders() {
    lightingShader = new CSCI441::ShaderProgram( "shaders/lab05.v.glsl", "shaders/lab05.f.glsl" );
    lightingShaderUniforms.mvpMatrix      = lightingShader->getUniformLocation("mvpMatrix");
    lightingShaderUniforms.mNormal        = lightingShader->getUniformLocation("mNormal");
    lightingShaderUniforms.lightDirection = lightingShader->getUniformLocation("lightDirection");
    lightingShaderUniforms.lightColor     = lightingShader->getUniformLocation("lightColor");
    lightingShaderUniforms.materialColor  = lightingShader->getUniformLocation("materialColor");
    lightingShaderAttributes.vNormal      = lightingShader->getAttributeLocation("vNormal");
    lightingShaderAttributes.vPos         = lightingShader->getAttributeLocation("vPos");
}

// setupBuffers() //////////////////////////////////////////////////////////////
//
//      Create our VAOs.  Send vertex data to the GPU to be stored.
//
////////////////////////////////////////////////////////////////////////////////
void setupBuffers() {
    struct VertexNormal {
        GLfloat x, y, z;
        GLint a;
    };

    VertexNormal groundQuad[4] = {
            {-1.0f, 0.0f, -1.0f, lightingShaderAttributes.vNormal},
            { 1.0f, 0.0f, -1.0f, lightingShaderAttributes.vNormal},
            {-1.0f, 0.0f,  1.0f, lightingShaderAttributes.vNormal},
            { 1.0f, 0.0f,  1.0f, lightingShaderAttributes.vNormal}
    };

    GLushort indices[4] = {0,1,2,3};

    glGenVertexArrays(1, &groundVAO);
    glBindVertexArray(groundVAO);

    GLuint vbods[2];       // 0 - VBO, 1 - IBO
    glGenBuffers(2, vbods);
    glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundQuad), groundQuad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(lightingShaderAttributes.vPos);
    glVertexAttribPointer(lightingShaderAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormal), (void*)0);

    glEnableVertexAttribArray(lightingShaderAttributes.vNormal);
    glVertexAttribPointer(lightingShaderAttributes.vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormal), (void*)(sizeof(float)*3));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

// setupScene() /////////////////////////////////////////////////////////////////
//
//	Setup everything specific to our scene.  in this case,
//	position our camera
//
////////////////////////////////////////////////////////////////////////////////
void setupScene() {
    leftMouseDown = false;
    mousePosition = glm::vec2(-9999.0f, -9999.0f);

	// give the camera a scenic starting point.
	camPos = glm::vec3(60.0f, 40.0f, 30.0f);
	camAngles = glm::vec3( -float(M_PI/2), 1.5f, 1.0f);
	cameraSpeed = glm::vec2(0.25f, 0.02f);
	updateCameraDirection();

	propAngle = 0.0f;

    srand( time(nullptr) );                 // seed our random number generator
    generateEnvironmentDL();                // create our environment display list

    lightingShader->useProgram();           // use our lighting shader program so when
                                            // assign uniforms, they get sent to this shader

    glm::vec3 lightDirection = glm::vec3(-1.0,-1.0,-1.0);
    glm::vec3 lightColor = glm::vec3(1,1,1);

    glUniform3fv(lightingShaderUniforms.lightDirection, 1, &lightDirection[0]);
    glUniform3fv(lightingShaderUniforms.lightColor, 1, &lightColor[0]);


}

///*************************************************************************************
//
// Our main function

// main() //////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main() {
    // GLFW sets up our OpenGL context so must be done first
    GLFWwindow *window = setupGLFW();	                    // initialize all of the GLFW specific information related to OpenGL and our window
    setupOpenGL();										    // initialize all of the OpenGL specific information
    setupGLEW();											// initialize all of the GLEW specific information

    CSCI441::OpenGLUtils::printOpenGLInfo();

    setupShaders();                                         // load our shader program into memory
    setupBuffers();
    setupScene();

    selectedHero = NotEvanVaughan;



    // needed to connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( lightingShaderAttributes.vPos, lightingShaderAttributes.vNormal );

	//  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
	//	until the user decides to close the window and quit the program.  Without a loop, the
	//	window will display once and then the program exits.
	while( !glfwWindowShouldClose(window) ) {	// check if the window was instructed to be closed
        glDrawBuffer( GL_BACK );				// work with our back frame buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

        NotEvanVaughanMotion += 0.05f;

        // update the projection matrix based on the window size
		// the GL_PROJECTION matrix governs properties of the view coordinates;
		// i.e. what gets seen - use a perspective projection that ranges
		// with a FOV of 45 degrees, for our current aspect ratio, and Z ranges from [0.001, 1000].
		glm::mat4 projMtx = glm::perspective( 45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.001f, 10000.0f );

		// Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
		// when using a Retina display the actual window can be larger than the requested window.  Therefore
		// query what the actual size of the window we are rendering to is.
		GLint framebufferWidth, framebufferHeight;
		glfwGetFramebufferSize( window, &framebufferWidth, &framebufferHeight );

		// update the viewport - tell OpenGL we want to render to the whole window
		glViewport( 0, 0, framebufferWidth, framebufferHeight );

		// set up our look at matrix to position our camera

        glm::mat4 viewMtx = glm::lookAt( camPos, camPos + camDir, glm::vec3(  0,  1,  0 ) );

        if (arcBall) {
            viewMtx = glm::lookAt((camDir + glm::vec3(NotEvanVaughanXLocation, 0, NotEvanVaughanYLocation)),
                                  glm::vec3(NotEvanVaughanXLocation, 0, NotEvanVaughanYLocation),
                                  glm::vec3(0, 1, 0));
        } else if (firstPerson) {
            viewMtx = glm::lookAt(glm::vec3(NotEvanVaughanXLocation, 6, NotEvanVaughanYLocation),
                                  camDir + glm::vec3(NotEvanVaughanXLocation, 6, NotEvanVaughanYLocation),
                                  glm::vec3(0, 1, 0));
		}

		renderScene( viewMtx, projMtx );					// draw everything to the window

		glfwSwapBuffers(window);                            // flush the OpenGL commands and make sure they get rendered!
		glfwPollEvents();				                    // check for any events and signal to redraw screen

        updateScene();
	}

	fprintf( stdout, "[INFO]: Shutting down.......\n" );
	fprintf( stdout, "[INFO]: ...freeing memory...\n" );

    delete lightingShader;                                  // delete our shader program
    glDeleteBuffers(1, &groundVAO);                         // delete our ground VAO
    CSCI441::deleteObjectVBOs();                            // delete our library VBOs
    CSCI441::deleteObjectVAOs();                            // delete our library VAOs

    fprintf( stdout, "[INFO]: ...closing GLFW.....\n" );

    glfwDestroyWindow( window );                            // clean up and close our window
    glfwTerminate();						                // shut down GLFW to clean up our context

    fprintf( stdout, "[INFO]: ............complete\n" );
    fprintf( stdout, "[INFO]: Goodbye\n" );

	return EXIT_SUCCESS;				                    // exit our program successfully!
}
