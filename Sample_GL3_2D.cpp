#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <ao/ao.h>
#include <mpg123.h>


#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define BITS 8


using namespace std;
//GLOBAL COUNT MOVES
GLint count=0;

//GLOBAL LIVES
GLint lives=3;

//Set camera zoom
GLfloat camera_zoom = 0.2;


//Mousepress variable to toggle
GLint mousepress=0;



//Define the mouse VARIABLES
GLdouble rightmouse_click=0;
GLdouble rightmouse_x;
GLdouble rightmouse_y;
GLdouble newrightmouse_x;
GLdouble newrightmouse_y;

GLdouble leftmouse_click=0;
GLdouble leftmouse_x;
GLdouble leftmouse_y;
GLdouble newleftmouse_x;
GLdouble newleftmouse_y;


//Theta definition Where
GLdouble theta=0.0f;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;
//Define the type of view
std::string view="default";
GLint flag=1;

struct GLMatrices {
    glm::mat4 projectionO, projectionP;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

GLuint programID;
int proj_type;
glm::vec3 tri_pos, rect_pos;

typedef struct COLOR {
    float r;
    float g;
    float b;
} COLOR;

//Draw the struct base object
typedef struct Base {
	string name;
	COLOR rgb_color;
    
	GLfloat x,y,z;
	VAO* object;
	GLint status;
	GLfloat height,width, length;
	GLfloat x_speed,y_speed;
    GLint  i, j;
	GLfloat angle; //Current Angle (Actual rotated angle of the object)
	GLint inAir;
	GLfloat radius;
	GLint fixed;
	GLfloat friction; //Value from 0 to 1
	GLint health;
	GLint isRotating;
	GLint direction; //0 for clockwise and 1 for anticlockwise for animation
	GLfloat remAngle; //the remaining angle to finish animation
	GLint isMovingAnim;
	GLint dx;
	GLint dy;
	int col_type;
	GLfloat weight;
    glm::mat4 translate_matrix;
    glm::mat4 rotate_matrix;
}Base;

//Previous handler
string previous;
//ACTIVATED PART OF Block
string activated="n";

//AUDIO INITIALISATION
mpg123_handle *mh;
unsigned char *buffer;
size_t buffer_size;
size_t done;
int err;

int driver;
ao_device *dev;

ao_sample_format format;
int channels, encoding;
long rate;

void audio_init() {
    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size= 3000;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "./breakout.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);
}

void audio_play() {
    /* decode and play */
    if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        ao_play(dev, (char*) buffer, done);
    else mpg123_seek(mh, 0, SEEK_SET);
}

void audio_close() {
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
}


struct Base tiles[100][100];
struct Base Blockobj;

std::string bstatus = "up";
std::string level="level1";


/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
	{
	    std::string Line = "";
	    while(getline(VertexShaderStream, Line))
		VertexShaderCode += "\n" + Line;
	    VertexShaderStream.close();
	}

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
	std::string Line = "";
	while(getline(FragmentShaderStream, Line))
	    FragmentShaderCode += "\n" + Line;
	FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    //    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    //    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    //    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void initGLEW(void){
    glewExperimental = GL_TRUE;
    if(glewInit()!=GLEW_OK){
	fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
    }
    if(!GLEW_VERSION_3_3)
	fprintf(stderr, "3.3 version not available\n");
}



/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);
    
    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);
    //glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
   // glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    


}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

void mousescroll(GLFWwindow *window, double xoffset, double yoffset)
{
    camera_zoom += yoffset / 10;
}



 void translatetiles(float x, float y , float z, int i, int j)
  {
        tiles[i][j].translate_matrix = glm::translate(glm::vec3(x, y, z));
  }

  void blocktranslate (float x, float y,float  z)
  {
    Blockobj.translate_matrix = glm::translate(glm::vec3(x, y, z));

  }


  void rendertiles(int i, int  j)
  {
      glm::mat4 VP = Matrices.projectionP * Matrices.view;
      glm::mat4 MVP;
      Matrices.model = glm::mat4(1.0f);
      Matrices.model = tiles[i][j].translate_matrix;
      MVP = VP * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      draw3DObject(tiles[i][j].object);
      return;
  }

    void blockrotator(float rotation, glm::vec3 rotating_vector=glm::vec3(0,0,1))
  {
      Blockobj.rotate_matrix = glm::rotate((float)(rotation*M_PI/180.0f), rotating_vector);
  }

  void renderblock()
  {

      
      blocktranslate (Blockobj.x, Blockobj.y, Blockobj.z);
    

      glm::mat4 MVP;
      Matrices.model =glm::mat4(1.0f);
      glm::mat4 VP = Matrices.projectionP * Matrices.view;
      Matrices.model = Blockobj.translate_matrix*Blockobj.rotate_matrix;
      MVP = VP * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      draw3DObject(Blockobj.object);

  }


  int level1[6][10] = {
        {1,1,1,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,0,0,0},
        {1,1,1,3,1,1,1,1,1,0},
        {0,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,1,1,4,3,1},
        {0,0,0,0,0,0,1,1,1,0}
           };

   int level2[6][15] = {
        {0,0,0,0,0,0,1,1,1,1,1,1,1,0,0},
        {1,1,1,1,0,0,1,1,1,0,0,1,1,0,0},
        {1,1,1,3,1,1,1,1,1,0,0,1,1,1,1},
        {1,1,1,1,0,0,0,0,0,0,0,1,3,4,1},
        {1,1,1,1,0,0,0,0,0,0,0,1,1,1,1},
        {1,1,1,1,0,0,0,0,0,0,0,0,1,1,1}

   };

 
   int level3[6][15] = {
        {0,0,0,0,0,0,1,1,1,1,0,0,1,1,1},
        {1,1,1,1,0,0,1,1,1,1,0,0,1,4,1},
        {1,1,1,1,0,0,1,1,1,1,0,0,1,1,1},
        {1,1,1,1,0,0,1,1,1,1,0,0,1,1,1},
        {1,1,1,1,0,0,1,1,1,1,0,0,1,1,1},
        {1,1,1,1,0,0,1,1,1,1,0,0,0,0,0},

   };
void drawtiles ( int a, int b , string s )
{
    if(s=="level1")
    {
    for(int i = 0; i < a  ; i++ ){
        for(int j = 0; j < b  ; j++ ){            
                translatetiles (tiles[i][j].x, tiles[i][j].y, tiles[i][j].z, i, j);   
                if(level1[i][j]==1 || level1[i][j]==3)
                    rendertiles(i,j);
        }
    }
    return;
    }
     if(s=="level2")
    {
    for(int i = 0; i < a  ; i++ ){
        for(int j = 0; j < b  ; j++ ){            
                translatetiles (tiles[i][j].x, tiles[i][j].y, tiles[i][j].z, i, j);   
                if(level2[i][j]==1 || level2[i][j]==3)
                    rendertiles(i,j);
        }
    }
    }
    if(s=="level3")
    {
        for(int i = 0; i < a  ; i++ ){
        for(int j = 0; j < b  ; j++ ){            
                translatetiles (tiles[i][j].x, tiles[i][j].y, tiles[i][j].z, i, j);   
                if(level3[i][j]==1 || level3[i][j]==3)
                    rendertiles(i,j);
        }
    }
    }
    return;
}

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
	case GLFW_KEY_C:
	    rectangle_rot_status = !rectangle_rot_status;
	    break;
	case GLFW_KEY_P:
	    triangle_rot_status = !triangle_rot_status;
	    break;
	case GLFW_KEY_X:
	    // do something ..
	    break;
	default:
	    break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {

    case GLFW_KEY_T:

                view="top";                        
                

    break;

    case GLFW_KEY_D:
                view ="default";
                break;

    case GLFW_KEY_B:
                view="block";
                break;

    case GLFW_KEY_F:
                view="followcam";
                break;
    case GLFW_KEY_H:
                
                view="helicopter";
                break;
    case GLFW_KEY_LEFT:
                count++;
                if(bstatus == "up")
                {
                    Blockobj.z += 0.3f;
                    Blockobj.y =1.3f;

                    Blockobj.i+=2;
                    previous="left";
                    activated="l";

                    
                   /* */
                    float rots=0.0f;
                    while(rots<=90.0f)
                        {
                            blockrotator(rots, glm::vec3(1, 0, 0));
                          
                            rots+=0.01f;
                            //sleep(0.1);
                            renderblock();
                        }
                          
                    cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                   
                    bstatus="horizdown";
                }
               else if (bstatus=="horizdown")
               {
                   Blockobj.z += 2*0.3f;
                  Blockobj.y = 1.0f;

                  //Assign i and j values depending on previous key pressed
                  if(activated=="l")
                  {
                  Blockobj.i+=1;

                  }
                  else if(activated=="r")
                  {
                      Blockobj.i+=2;
                  }
                      
                previous="left";
                activated="n";

                    blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );

                    renderblock();
                     
                    bstatus="up";
                    cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                }
          
          else if (bstatus=="lateraldown")
            {
                Blockobj.z += 0.3f;
                  Blockobj.y = 1.3f;
                
                  if(activated=="u")
                  {
                  Blockobj.i+=1;
                  activated="u";

                  }
                  else if(activated=="d")
                  {
                      Blockobj.i+=1;
                      activated="d";
                  }
                             
                previous="left";
                

                   
                    //blockrotator ( 0.0f, glm::vec3 ( 0,0,-1 ) );

                    renderblock();
                    cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
             }
                break;

     case GLFW_KEY_RIGHT:
            count++;
            if(bstatus=="up")
            {
                Blockobj.z -= 2*0.3f;
                Blockobj.y =1.3f;
                float rots=0.0f;

                Blockobj.i-=2;
                activated="r";

                  
                previous="right";
                    
                            blockrotator(90.0f, glm::vec3(1, 0, 0));
                          
                           
                //blockrotator(90.0f, glm::vec3(1, 0, 0));
                
                cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                bstatus="horizdown";
                   
                
            }
            else if (bstatus=="horizdown")
               {
                  Blockobj.z -= 0.3f;
                  Blockobj.y = 1.0f;
                  
                  if(activated=="l")
                  {
                  Blockobj.i-=2;
                  activated="n";

                  }
                  else if(activated=="r")
                  {
                      Blockobj.i-=1;
                      activated="n";
                  }
                     
                previous="right";

                   
                    blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );

                    renderblock();
                     cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                    bstatus="up";
                    
                }
            else if (bstatus=="lateraldown")
            {
                Blockobj.z -= 0.3f;
                  Blockobj.y = 1.3f;

                  if(activated=="u")
                  {
                  Blockobj.i-=1;
                  activated="u";

                  }
                  else if(activated=="d")
                  {
                      Blockobj.i-=1;
                      activated="d";
                  }
                   previous="right";
                    //blockrotator ( 0.0f, glm::vec3 ( 0,0,-1 ) );

                    renderblock();
                    cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
             }
             break;

        case GLFW_KEY_UP:
            count++;
            if(bstatus=="up")
            {
                Blockobj.x -= 2*0.3f;
                Blockobj.y =1.3f;

                Blockobj.j-=2;
                previous="up";
                activated="u";


                blockrotator(90.0f, glm::vec3(0, 0, -1));
                renderblock();
                     
                    bstatus="lateraldown";
                    cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
            }              
            else if (bstatus=="lateraldown")
            {
                Blockobj.x -= 0.3f;
                  Blockobj.y = 1.0f;
                   

                   if(activated=="u")
                   {
                       Blockobj.j-=1;
                       activated="n";
                   }
                   else if (activated=="d")
                   {
                       Blockobj.j-=2;
                       activated="n";
                   }
                   previous="up";




                    blockrotator ( 0.0f, glm::vec3 ( 0,0,-1 ) );

                    renderblock();
                    bstatus="up";
                   cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;

            }
             else if(bstatus=="horizdown") 
            {
                  Blockobj.x -= 0.3f;
                  Blockobj.y = 1.3f;
                  bstatus="horizdown";

                  if(activated=="l")
                  {
                      Blockobj.j-=1;
                      activated="l";
                  }
                  else if (activated=="r")
                  {
                      Blockobj.j-=1;
                      activated="r";
                  }

                  previous="up";
                    //blockrotator ( 180.0f, glm::vec3 ( 0,0,-1 ) );
                  renderblock();
                  cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                     
            }
            break;

        case GLFW_KEY_DOWN:
            count++;
            if(bstatus=="up")
            {
                Blockobj.x += 0.3f;
                Blockobj.y =1.3f;

                Blockobj.j+=2;
                previous="down";
                activated="d";

                blockrotator(-90.0f, glm::vec3(0, 0, 1));
                renderblock();
                     
                    bstatus="lateraldown";
                      cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
            }              
            else if (bstatus=="lateraldown")
            {
                Blockobj.x += 2*0.3f;
                  Blockobj.y = 1.0f;

                  if(activated=="u"){
                      Blockobj.j+=2;
                      activated="n";
                  }
                  else if (activated=="d")
                  {
                      Blockobj.j+=1;
                      activated="n";
                  }
                  previous="down";

                   
                    blockrotator ( 0.0f, glm::vec3 ( 0,0,1 ) );
                    renderblock();
                    bstatus="up";

                     cout << "haha X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                    cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
            }
            else if(bstatus=="horizdown")
            {
                  Blockobj.x += 0.3f;
                  Blockobj.y = 1.3f;

                  if(activated=="r")
                  {
                      Blockobj.j+=1;
                      activated="r";
                  }

                  else if(activated=="l")
                  {
                      Blockobj.j+=1;
                      activated="l";
                  }

                  previous="down";
                   // blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );
                  renderblock();
                 cout << "X is "<< Blockobj.x<< endl;
                    cout << "Y is "<< Blockobj.y<< endl;
                    cout <<"Z is "<< Blockobj.z << endl;
                cout << "COORDS ARE : " << "[" << Blockobj.i << "][" << Blockobj.j << "]" << endl;
                     
            }
            break;
                
	case GLFW_KEY_ESCAPE:
	    quit(window);
	    break;
	default:
	    break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
    case 'Q':
    case 'q':
	quit(window);
	break;
    case ' ':
	proj_type ^= 1;
	break;
    case 'a':
	tri_pos.x -= 0.2;
	break;
    case 'd':
	tri_pos.x += 0.2;
	break;
    case 'w':
	tri_pos.y += 0.2;
	break;
    case 's':
	tri_pos.y -= 0.2;
	break;
    case 'f':
	tri_pos.z += 0.2;
	break;
    case 'r':
	tri_pos.z -= 0.2;
	break;
    case 'j':
	rect_pos.x -= 0.2;
	break;
    case 'l':
	rect_pos.x += 0.2;
	break;
    case 'i':
	rect_pos.y += 0.2;
	break;
    case 'k':
	rect_pos.y -= 0.2;
	break;
    case 'y':
	rect_pos.z -= 0.2;
	break;
    case 'h':
	rect_pos.z += 0.2;
	break;
    default:
	break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
	if (action == GLFW_RELEASE)
    {
            mousepress=0;
    }
	    
    else if(action == GLFW_PRESS)
    {
            mousepress=1;
            view="helicopter";
    }
	break;

    case GLFW_MOUSE_BUTTON_RIGHT:
	if (action == GLFW_RELEASE) {
	    rectangle_rot_dir *= -1;
	}
	break;
    default:
	break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2.0f;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    Matrices.projectionP = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
   //Matrices.projectionO = glm::ortho(-3.4f, 3.4f, -3.4f, 3.4f, 0.1f, 5000.0f);
}

VAO *triangle, *rectangle, *box, *block;

// Creates the triangle object used in this sample code
void createTriangle ()
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data [] = {
	0, 0, 0, // vertex 0
	-1, 1, 0, // vertex 1
	-1, 0, 0, // vertex 2
    };

    static const GLfloat color_buffer_data [] = {
	1,0,0, // color 0
	1,0,0, // color 1
	1,0,0, // color 2
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}



void createBox(float l, float b, float h, float r, float g, float bl, float x, float y , float z,int i, int j)
{
    GLfloat vertex_buffer_data[ ] = {
        0, 0, 0, b, 0, 0, b, h, 0, b, h, 0, 0, h, 0, 0, 0 , 0,        //1
        0, 0, 0, 0, h, 0, 0, h, l, 0, h, l, 0, 0, l, 0, 0, 0,    //2
        0, 0, 0, 0, 0, l, b, 0, l, b, 0, l, b, 0, 0, 0, 0, 0,        //3 

        0, 0, l, b, 0, l, b, h, l, b, h, l, 0, h, l, 0, 0, l,        //4
        b, 0, l, b, 0, 0, b, h, 0, b, h, 0, b, h, l, b, 0, l,            //5
        0, h, l, b, h, l, b, h, 0, b, h, 0, 0, h, 0, 0, h, l         //6
    };
    GLfloat color_buffer_data [ ] = {
    r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl,
        r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl,
        r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl,
        
        r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl,
        r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl,
        r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl, r, g, bl
    };
    box = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    //Create the Base temporary object here corresponding to the name and this gets sorted

	Base tempobj = {};
	tempobj.rgb_color = {r,g,bl};
	tempobj.object = box;
	tempobj.x=x;
	tempobj.y=y;
    tempobj.z=z;
	tempobj.height=h;
	tempobj.width=b;
	tempobj.status=1;
	tempobj.inAir=0;
	tempobj.angle=0;
	tempobj.x_speed=0;
	tempobj.y_speed=0;
	tempobj.fixed=0;
	tempobj.radius=(sqrt(h*h + b*b))/2;
	tempobj.friction=0.4;
	tempobj.health=100;
//	tempobj.weight=weight;

    tiles[i][j]=tempobj;
}

void createBlock(float l, float b, float h, float r, float g, float bl, float x, float y , float z, int i, int j)
{
    GLfloat vertex_buffer_data[ ] = {
        0, 0, 0, b, 0, 0, b, h, 0, b, h, 0, 0, h, 0, 0, 0 , 0,        //1
        0, 0, 0, 0, h, 0, 0, h, l, 0, h, l, 0, 0, l, 0, 0, 0,    //2
        0, 0, 0, 0, 0, l, b, 0, l, b, 0, l, b, 0, 0, 0, 0, 0,        //3 

        0, 0, l, b, 0, l, b, h, l, b, h, l, 0, h, l, 0, 0, l,        //4
        b, 0, l, b, 0, 0, b, h, 0, b, h, 0, b, h, l, b, 0, l,            //5
        0, h, l, b, h, l, b, h, 0, b, h, 0, 0, h, 0, 0, h, l         //6
    };
    GLfloat color_buffer_data [ ] = {
    0.583f,  0.771f,  0.014f,
    0.609f,  0.115f,  0.436f,
    0.327f,  0.483f,  0.844f,
    0.822f,  0.569f,  0.201f,
    0.435f,  0.602f,  0.223f,
    0.310f,  0.747f,  0.185f,
    0.597f,  0.770f,  0.761f,
    0.559f,  0.436f,  0.730f,
    0.359f,  0.583f,  0.152f,
    0.483f,  0.596f,  0.789f,
    0.559f,  0.861f,  0.639f,
    0.195f,  0.548f,  0.859f,
    0.014f,  0.184f,  0.576f,
    0.771f,  0.328f,  0.970f,
    0.406f,  0.615f,  0.116f,
    0.676f,  0.977f,  0.133f,
    0.971f,  0.572f,  0.833f,
    0.140f,  0.616f,  0.489f,
    0.997f,  0.513f,  0.064f,
    0.945f,  0.719f,  0.592f,
    0.543f,  0.021f,  0.978f,
    0.279f,  0.317f,  0.505f,
    0.167f,  0.620f,  0.077f,
    0.347f,  0.857f,  0.137f,
    0.055f,  0.953f,  0.042f,
    0.714f,  0.505f,  0.345f,
    0.783f,  0.290f,  0.734f,
    0.722f,  0.645f,  0.174f,
    0.302f,  0.455f,  0.848f,
    0.225f,  0.587f,  0.040f,
    0.517f,  0.713f,  0.338f,
    0.053f,  0.959f,  0.120f,
    0.393f,  0.621f,  0.362f,
    0.673f,  0.211f,  0.457f,
    0.820f,  0.883f,  0.371f,
    0.982f,  0.099f,  0.879f
    };
    block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

    //Create the Base temporary object here corresponding to the name and this gets sorted

	Base tempobj = {};
	tempobj.rgb_color = {r,g,bl};
	tempobj.object = block;
	tempobj.x=x;
	tempobj.y=y;
    tempobj.z=z;
    tempobj.i=i;
    tempobj.j=j;
	tempobj.height=h;
	tempobj.width=b;
    tempobj.length=0.3f;
	tempobj.status=1;
	tempobj.inAir=0;
	tempobj.angle=0;
	tempobj.x_speed=0;
	tempobj.y_speed=0;
	tempobj.fixed=0;
	tempobj.radius=(sqrt(h*h + b*b))/2;
	tempobj.friction=0.4;
	tempobj.health=100;
//	tempobj.weight=weight;

    Blockobj=tempobj;
    

}

// Creates the rectangle object used in this sample code
void createRectangle ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
	0, 0, 0, // vertex 1
	1, 0, 0, // vertex 2
	1, 1, 0, // vertex 3

	0, 0, 0, // vertex 1
	1, 1, 0, // vertex 3
	0, 1, 0  // vertex 4
    };

    static const GLfloat color_buffer_data [] = {
	0,1,0, // color 1
	0,1,0, // color 2
	0,1,0, // color 3

	0,1,0, // color 1
	0,1,0, // color 3
	0,1,0  // color 4
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 45.0f;
float rectangle_rotation = 0;
float triangle_rotation = 0;

//TIME LAPSE AT SWITCH
double t1;
double t2;



/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window, float x, float y, float w, float h)
{
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport(0, 0, (int)(w*fbwidth), (int)(h*fbheight));


    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    if(view=="default")
    { 
    // Eye - Location of camera. Don't change unless you are sure!!
   // glm::vec3 eye ( 3*cos(camera_rotation_angle*M_PI/180.0f), 3, 3*sin(camera_rotation_angle*M_PI/180.0f) );
     glm::vec3 eye ( 3.8,3.8,4);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (2,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane
    }


    if(view=="helicopter" && mousepress==1)
    {
        

     glm::vec3 eye ( 3*cos(theta*M_PI/180.0f),3,3*sin(theta*M_PI/180));

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane
    }

    if(view=="top")
    {
         
         
         if(level=="level1")
         {
             glm::vec3 eye ( 1.1,3,1);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (1.1,0,1);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (-1,0,0);
         Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane

         }


         else if(level=="level2")
         {
             glm::vec3 eye ( 2.5,4,0);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
             glm::vec3 target (2.5,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
             glm::vec3 up (-1,0,0);
         Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane

         }
         else if(level=="level3")
         {
             glm::vec3 eye ( 2.8,4,0);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
             glm::vec3 target (2.8,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
             glm::vec3 up (-1,0,0);
         Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane

         }
    }

    if(view=="block")
    {
        glm::vec3 eye ( Blockobj.x-0.8f,1.6f ,Blockobj.z);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (-1000.0f, -100, Blockobj.z);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);
     Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane

    }

    if(view=="followcam")
    {
         glm::vec3 eye ( Blockobj.x+1.2f,Blockobj.y+0.8f,Blockobj.z);

    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (-1000.0f,-100,Blockobj.z);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);
     Matrices.view = glm::lookAt(eye, target, up)*glm::scale(glm::vec3(exp(camera_zoom))); // Fixed camera for 2D (ortho) in XY plane

    }
    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    //glm::mat4 VP = (proj_type?Matrices.projectionP:Matrices.projectionO) * Matrices.view;
    glm::mat4 VP = Matrices.projectionP * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    /*
    glm::mat4 translateTriangle = glm::translate (tri_pos); // glTranslatef
    glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
    glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
    Matrices.model *= triangleTransform; 
    MVP = VP * Matrices.model; // MVP = p * V * M

    //  Don't change unless you are sure!!
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(triangle);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateRectangle = glm::translate (rect_pos);        // glTranslatef
    glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(rectangle);




    /*glm::mat4 translatebox = glm::translate (rect_pos);        // glTranslatef
    glm::mat4 rotatebox = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translatebox * rotatebox);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);   */ 
   
    if(level=="level1")
       
       {
            drawtiles(6,10, "level1");
            
            if( level1[Blockobj.i][Blockobj.j] ==0 || Blockobj.i <0 ||  Blockobj.i > 5 || Blockobj.j <0 || Blockobj.j >9 )

                    {
                        Blockobj.y -=0.1f;
                    }
            if(bstatus=="up" && level1[Blockobj.i][Blockobj.j] == 3 )
                    {
                        Blockobj.y -=0.1f;
                    }
                       /* cout << " HI I AM " << (int)(Blockobj.z/0.3f) << endl;
                        cout << " Hw I AM " << (int)(Blockobj.x/0.3f) << endl;
                        cout << "status is " << bstatus;*/

                    

            if(Blockobj.y <= -25.0f )
                        {
                            

                            Blockobj.x=0.3f;
                            Blockobj.y=1.0f;
                            Blockobj.z=0.3f;

                            lives--;

                            Blockobj.i=1;
                            Blockobj.j=1;
             if (bstatus=="horizdown")
               {
                  Blockobj.z = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );

                    renderblock();
                    
                    bstatus="up";
               }
            else if (bstatus=="lateraldown")
                     {
                    Blockobj.x = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 0,0,1 ) );
                    renderblock();
                    bstatus="up";
                    
                     }
             }
                    
            
       }
                        
    
    if(level=="level2")
       {

        drawtiles(6,15, "level2");
                                //cout << "status is " << bstatus << endl;

         if( ( (level2[Blockobj.i][Blockobj.j] ==0)) || (Blockobj.i <0 ||  Blockobj.j < 0 || Blockobj.i >=15 || Blockobj.j>= 15))
                     Blockobj.y -=0.1f;
         
          /*if(bstatus=="up" && level2[(int)((Blockobj.z/0.3f))][(int)(((Blockobj.x/0.3f)))] == 3   )
                    {
                        Blockobj.y -=0.1f;
                        cout << " HI I AM " << (Blockobj.x) << endl;
                        cout << " Hw I AM " << (int)(Blockobj.x/0.3f) << endl;
                        cout << "status is " << bstatus;

                    }*/

        //Fragile on level 2
        if(bstatus=="up" && level2[Blockobj.i][Blockobj.j]==3)
        {
            Blockobj.y -=0.1f;
                        /*cout << " HI I AM lllllll  X IS " << Blockobj.x<< endl;
                        cout << " Hw I AM Z IS " << Blockobj.z << endl;
                        cout << "status is " << bstatus;*/
        }
         if(Blockobj.y <= -25.0f )
                        {

                            Blockobj.x=0.3f;
                            Blockobj.y=1.0f;
                            Blockobj.z=0.3f;

                            lives--;

                            Blockobj.i=1;
                            Blockobj.j=1;
             if (bstatus=="horizdown")
               {
                  Blockobj.z = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );

                    renderblock();
                    
                    bstatus="up";
               }
            else if (bstatus=="lateraldown")
                     {
                    Blockobj.x = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 0,0,1 ) );
                    renderblock();
                    bstatus="up";
                    
                     }
             }
       }
 

    if(level=="level3")
    {           t2 = glfwGetTime(); //NOT WORKING. SEARCH FOR A DIFFERENT SOLUTION!

        drawtiles(6,15,"level3");
        
        //BRIDGE LEVELS
        if(Blockobj.i==2  && Blockobj.j==2 )
             {
                   level3[4][4]=1;
                      level3[4][5]=1;
             }     



        if(Blockobj.j==8  && Blockobj.i==1 )
             {
                    level3[4][10]=1;
                    level3[4][11]=1;
             }
        
        
         if(level3[Blockobj.i][Blockobj.j] ==0 || (Blockobj.i <0 ||  Blockobj.j < 0 || Blockobj.j >=15 || Blockobj.i >= 15))
                    Blockobj.y -=0.1f;  

        //Fragile tiles
        if(bstatus=="up" && Blockobj.j==3 && Blockobj.i==2)
                    {
                        Blockobj.y -=0.1f;
                        /*cout << " HI I AM " << (int)(Blockobj.z/0.3f) << endl;
                        cout << " Hw I AM " << (int)(Blockobj.x/0.3f) << endl;
                        cout << "status is " << bstatus;*/

                    }
      //Frgaile tiles
        if(bstatus=="up" && Blockobj.j==12 && Blockobj.i==3 )
                    {
                        Blockobj.y -=0.1f;
                        /*cout << " HI I AM " << (int)(Blockobj.z/0.3f) << endl;
                        cout << " Hw I AM " << (int)(Blockobj.x/0.3f) << endl;
                        cout << "status is " << bstatus;*/

                    }
         if(Blockobj.y <= -25.0f )
                        {

                            Blockobj.x=0.3f;
                            Blockobj.y=1.0f;
                            Blockobj.z=0.3f;

                            lives--;

                            Blockobj.i=1;
                            Blockobj.j=1;
             if (bstatus=="horizdown")
               {
                  Blockobj.z = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 1,0,0 ) );

                    renderblock();
                    
                    bstatus="up";
               }
            else if (bstatus=="lateraldown")
                     {
                    Blockobj.x = 0.3f;
                  Blockobj.y = 1.0f;
                   
                    blockrotator ( 0.0f, glm::vec3 ( 0,0,1 ) );
                    renderblock();
                    bstatus="up";
                    
                     }
             }

    if( fabs(Blockobj.x-3.9f)<0.1f && fabs(Blockobj.z-0.3f)<0.1f && bstatus=="up")
        {
        cout << "Congrats you win!"<<endl;
          glfwDestroyWindow(window);
          glfwTerminate();
          exit(EXIT_SUCCESS);
     }
    }

    
    renderblock();
    t1=t2;
    
      
    // Increment angles
    float increments = 1;

    //camera_rotation_angle++; // Simulating camera rotation
    //  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
    //  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
	exit(EXIT_FAILURE);
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    //    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); //enable mouse scrolling

    return window;
}
    
//GLOBAL VARIABLES
float x_ordinate = 0.0f , y_ordinate=0.0f, z_ordinate = 0.0f;

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    //createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    //createRectangle ();
	//createBox(0.3f, 0.3f, -0.3f, 1,1,0, 1,1,1,0,0);
    //Create the boxes


    //LEVEL 1
   /* if(level=="level1")
    {
    for(int i = 0; i < 6; i++ ){
         x_ordinate = 0.0f;
        for(int j = 0; j < 10;j++){
            if((i + j) % 2 == 0){
                y_ordinate =  1.0f;
                createBox(0.3f, 0.3f, -0.1f, 238.0/256.0, 47.0/256.0, 127.0/256.0, x_ordinate, y_ordinate, z_ordinate, i, j);
               
            }
            else{
                y_ordinate =  1.0f;
                createBox(0.3f, 0.3f, -0.1f, 59.0/256.0, 28.0/256.0, 180.0/256.0, x_ordinate, y_ordinate, z_ordinate, i, j);
                
            }
            x_ordinate += 0.3f;
        }
        z_ordinate += 0.3f;
    }
    x_ordinate = 0.3f;
    y_ordinate = 1.0f ;
    z_ordinate = 0.3f;
    createBlock(0.3f, 0.3f, 0.6f, 1, 0,1,x_ordinate, y_ordinate, z_ordinate);
    }

    //LEVEL 2
    else if(level=="level2")
    {*/
        for(int i = 0; i < 100; i++ ){
         x_ordinate = 0.0f;
        for(int j = 0; j < 100;j++){
            
            if((i + j) % 2 == 0){
                y_ordinate =  1.0f;
                createBox(0.3f, 0.3f, -0.1f, 238.0/256.0, 47.0/256.0, 127.0/256.0, x_ordinate, y_ordinate, z_ordinate, i, j);
               
            }
            else{
                y_ordinate =  1.0f;
                createBox(0.3f, 0.3f, -0.1f,59.0/256.0, 28.0/256.0, 180.0/256.0, x_ordinate, y_ordinate, z_ordinate, i, j);
                
            }
            x_ordinate += 0.3f;
        }
        z_ordinate += 0.3f;
    }
    x_ordinate = 0.3f;
    y_ordinate = 1.0f ;
    z_ordinate = 0.3f;
    createBlock(0.3f, 0.3f, 0.6f, 0.47, 0.3,0.56,x_ordinate, y_ordinate, z_ordinate, 1 , 1);
    

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    float arr[3]={	255, 94, 32	};
    // Background color of the scene
    glClearColor (arr[0]/256.0, arr[1]/256.0f, arr[2]/256.0, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    // cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    // cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    // cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    // cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
    int width = 1000;
    int height = 1000;
    proj_type = 1;
    tri_pos = glm::vec3(0, 0, 0);
    rect_pos = glm::vec3(0, 0, 0);

    GLFWwindow* window = initGLFW(width, height);
    initGLEW();
    initGL (window, width, height);
    audio_init();


    double last_update_time = glfwGetTime(), current_time;
t1 = glfwGetTime();
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

	// clear the color and depth in the frame buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Get the mouse cursor position      
    glfwGetCursorPos(window, &leftmouse_x, &leftmouse_y);
    
    theta = (leftmouse_x*360.0f)/1000.0f;


      // OpenGL Draw commands
	draw(window, 0, 0, 1, 1);

    audio_play();
    if(level=="level1")
    {
       createBox(0.3f, 0.3f, -0.1f, 100.0/256.0, 117.0/256.0, 167.0/256.0, 0.9f, 1.0f, 0.6f, 2, 3);
        createBox(0.3f, 0.3f, -0.1f, 100.0/256.0, 117.0/256.0, 167.0/256.0, 2.4f, 1.0f, 1.2f, 4, 8);

    }

    if(level=="level1" && level1[Blockobj.i][Blockobj.j]==4  && bstatus=="up" )
            {   level="level2";
                Blockobj.i=1;
                Blockobj.j=1;
                Blockobj.x=0.3f;
                Blockobj.y=1.0f;
                Blockobj.z=0.3f;
                count=0;
                cout << "SWITCHED LEVEL to LEVEL 2!";
                //if(level2[2][3]==3)
                 createBox(0.3f, 0.3f, -0.1f, 238.0/256.0, 47.0/256.0, 127.0/256.0, 0.9f, 1.0f, 0.6f, 2, 3);
                 //if(level2[4][8]==3)
                 createBox(0.3f, 0.3f, -0.1f, 238.0/256.0, 47.0/256.0, 127.0/256.0, 2.4f, 1.0f, 1.2f, 4, 8);

                Blockobj.x =0.3f;
                Blockobj.y=1.0f;
                Blockobj.z=0.3f;

                 createBox(0.3f, 0.3f, -0.1f, 100.0/256.0, 117.0/256.0, 167.0/256.0, 0.9f, 1.0f, 0.6f, 2, 3);
                 createBox(0.3f, 0.3f, -0.1f, 100.0/256.0, 117.0/256.0, 167.0/256.0, 3.6f, 1.0f, 0.9f, 3, 12);

                

                cout << "level2";
                flag=2;
               
            }
    if( level=="level2" && level2[Blockobj.i][Blockobj.j]==4 && bstatus=="up" )
    {
                        cout << "SWITCHED LEVEL to LEVEL 3!";

        count=0;
        Blockobj.i=4;
       Blockobj.j=1;
        Blockobj.x = 0.3f;
        Blockobj.y=1.0f;
        Blockobj.z=1.2f;
        level="level3";
        flag=3;
        createBox(0.3f, 0.3f, -0.1f, 200.0/256.0, 147.0/256.0, 27.0/256.0, 2.4f, 1.0f, 0.3f, 1, 8);
        createBox(0.3f, 0.3f, -0.1f, 200.0/256.0, 147.0/256.0, 27.0/256.0, 0.6f, 1.0f, 0.6f, 2,2);


    }

    if(lives<=0)
    {
        cout << "SORRY YOU LOSE!"<<endl;
       cout << "YOUR FINAL NO. OF MOVES FOR THIS LEVEL: " << count << endl;


          glfwDestroyWindow(window);
          glfwTerminate();
          exit(EXIT_SUCCESS);

    }

    
	// proj_type ^= 1;
	// draw(window, 0.5, 0, 0.5, 1);
	// proj_type ^= 1;

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        //Print the no. of MOVES
        cout << "__________________________" << endl;
        cout << "NO. OF MOVES: " << count << endl;
        cout << " LEVEL IS : " << flag << endl;
        cout << "LIVES LEFT : " <<lives <<endl;
        cout << "__________________________" << endl;



        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }
    audio_close();
    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}
