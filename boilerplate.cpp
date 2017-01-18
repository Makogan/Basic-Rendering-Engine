// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <cstring>
#include "glm/glm.hpp"
#include <vector>
#include <unistd.h>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader, string s)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource(s);
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		//cout << numComponents << endl;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

//Globalize variables because C++ is an arrogant shit
// three vertex positions and assocated colours of a triangle

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{

	GLfloat vertices[][2] = {
		{-1, -1},
		{-1, 1},
		{1, -1},
		{1, 1},
	};

	GLfloat textureCoords[][2] = {
		{-1, -1},
		{-1, 1},
		{1, -1},
		{1, 1},
	};

	GLfloat colours[][4] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f }
	};

	geometry->elementCount = 4;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
	const GLuint TEXTURE_INDEX = 2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//
	glGenBuffers(1, &geometry->textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// Tell openGL how the data is formatted
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXTURE_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	//glBindTexture(texture->target, texture->textureID);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	//glBindTexture(texture->target, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}
//--------------------------------------------------------------------------
//Global Variables

MyTexture texture;
MyGeometry geometry;
MyShader shader;

struct object
{
	int type;
	vec3 x, y, z;
	vec4 color;
	vec4 specularity;
	int shininess;
	float reflectance;
	float refraction;
};

void deconstructObjects	(vector<object> objects, int* types, float* xs, 
						float* ys, float* zs, float* color, float* specularities,
						int* shininesses, float* reflectances,
						float* refractions)
{
	for(int i=0; i<(int)objects.size(); i++)
	{
		types[i] = objects[i].type;
		
		for(int j=0; j<3; j++)
		{
			xs[i*3+j] =  objects[i].x[j];
			ys[i*3+j] =  objects[i].y[j];
			zs[i*3+j] =  objects[i].z[j];
			color[i*4+j] = objects[i].color[j];
			specularities[i*4+j] = objects[i].specularity[j];
		}
		color[i*4+3] = objects[i].color[3];
		specularities[i*4+3] = objects[i].specularity[3];
		
		shininesses[i] = objects[i].shininess;
		reflectances[i] = objects[i].reflectance;
		
		refractions[i] = objects[i].refraction;
				
	}
}

void setObjects(vector<object> objects, vector<float> lights, vector<float> lightIntensities)
{
	/*object o;
	o.type = 0;
	o.x = vec3(0,0,-10);
	o.y = vec3(2,0,0);
	o.z = vec3(0);
	o.color = vec4(0.5);
	o.specularity = vec4(1);
	o.shininess = 1;
	
	objects = {o};
	o.x=vec3(4,0,-15);
	o.color=vec4(0,1,0,0);
	objects.push_back(o);*/

	int types[objects.size()];
	float xs[objects.size()*3];
	float ys[objects.size()*3];
	float zs[objects.size()*3];
	float colors[objects.size()*4];
	float spec[objects.size()*4];
	int shine[objects.size()];
	float reflectances[objects.size()];
	float refractions[objects.size()];
	
	deconstructObjects(objects, types, xs, ys, zs, colors, spec, shine, 
						reflectances, refractions);

	glUseProgram(shader.program);
	GLuint loc = glGetUniformLocation(shader.program, "objectTypes");
	glUniform1iv(loc, objects.size(), types);
	
	loc = glGetUniformLocation(shader.program, "xs");
	glUniform3fv(loc, objects.size(), xs);
	
	loc = glGetUniformLocation(shader.program, "ys");
	glUniform3fv(loc, objects.size(), ys);
	
	loc = glGetUniformLocation(shader.program, "zs");
	glUniform3fv(loc, objects.size(), zs);
	
	loc = glGetUniformLocation(shader.program, "colors");
	glUniform4fv(loc, objects.size(), colors);
	
	loc = glGetUniformLocation(shader.program, "specularities");
	glUniform4fv(loc, objects.size(), spec);
	
	loc = glGetUniformLocation(shader.program, "shininesses");
	glUniform1iv(loc, objects.size(), shine);

	loc = glGetUniformLocation(shader.program, "reflectances");
	glUniform1fv(loc, objects.size(), reflectances);
	
	loc = glGetUniformLocation(shader.program, "refractions");
	glUniform1fv(loc, objects.size(), refractions);
	
	loc = glGetUniformLocation(shader.program, "numOfObjects");
	glUniform1i(loc, objects.size());

	loc = glGetUniformLocation(shader.program, "lights");
	glUniform3fv(loc, lights.size(), lights.data());

	loc = glGetUniformLocation(shader.program, "lightIntensities");
	glUniform1fv(loc, lightIntensities.size(), lightIntensities.data());

	loc = glGetUniformLocation(shader.program, "lightNum");
	glUniform1i(loc, lights.size()/3);
}

bool isValidObject(string w)
{	
	if(w=="sphere" || w=="plane" || w=="triangle")
		return true;
	return false;
}

struct Material
{
	vec4 spec;
	int phong;
	float reflectance;
	float refraction;
	float transparency;
};

object buildObject(string figure, Material m)
{
	int i = 13;
	object o;
	string term;
	istringstream processor(figure);
	vector<float> info;

	processor>>term;
	if (term=="sphere")
		o.type=0;
	else if(term=="plane")
		o.type=1;
	else if(term=="triangle")
		o.type=2;

	processor>>term;
	
	while(processor>>term && term.find("}"))
	{
		info.push_back(stof(term,NULL));
		i--;
	}

	if(i<0)
		cout<<"Too many numbers! you forgot to change the code dummy"<<endl;
	
	while(i>0)
	{
		i--;
		info.push_back(0);
	}
	
	if(o.type==0)
	{
		o.x=vec3(info[0],info[1],info[2]);
		o.y=vec3(info[3],0,0);
		o.z=vec3(0);
		o.color=vec4(info[4],info[5],info[6],info[7]);
	}

	else if(o.type==1)
	{
		o.x=vec3(info[0],info[1],info[2]);
		o.y=vec3(info[3],info[4],info[5]);
		o.z=vec3(0);
		o.color=vec4(info[6],info[7],info[8],info[9]);
	}

	else if(o.type==2)
	{
		o.x=vec3(info[0],info[1],info[2]);
		o.y=vec3(info[3],info[4],info[5]);
		o.z=vec3(info[6],info[7],info[8]);
		o.color=vec4(info[9],info[10],info[11],info[12]);
	}

	o.specularity=m.spec;
	o.shininess=m.phong;
	o.reflectance=m.reflectance;
	o.refraction=m.refraction;

	return o;
}

void buildMaterial(string material, Material *mater)
{
	istringstream bobTheBuilder(material);

	string property;
	bobTheBuilder>>property;

	while(bobTheBuilder>>property)
	{
		if(property=="phong:")
		{
			bobTheBuilder>>property;
			mater->phong= stoi(property,NULL);
		}

		else if(property=="spec:")
		{
			for(int i=0; i<4; i++)
			{
				bobTheBuilder>>property;
				mater->spec[i]= stof(property,NULL);
			}
		}

		else if(property=="reflectance:")
		{
			bobTheBuilder>>property;
			mater->reflectance= stof(property,NULL);
		}
		
		else if(property=="refraction:")
		{
			bobTheBuilder>>property;
			//cout<<"Look at those flaots: " <<property<< ":end." <<endl;
			mater->refraction= stof(property);
		}
	}

	//cout<<"Spec: "<<mater->spec[0]<< " " << mater->spec[1]<< " "<< mater->spec[2] <<" Phong: "<< mater->phong<<" Reflectance: "<<mater->reflectance<<endl;
}

void addLight(string info, vector<float> *lights, vector<float> *intensities)
{
	istringstream torch(info);

	string property;
	torch>>property;
	bool position =false, intensity=false;

	while(torch>>property)
	{
		if(property=="position:")
		{
			for(int i=0; i<3; i++)
			{
				torch>>property;
				lights->push_back(stof(property,NULL));
			}

			position = true;
		}

		else if(property=="intensity:")
		{
			torch>>property;
			//cout<<property<<endl;
			intensities->push_back(stof(property,NULL));
			intensity=true;
		}
	}

	if(!intensity)
	{
		intensities->push_back(1);
	}

	if(!position)
	{
		for(int i=0; i<3; i++)
			{
				lights->push_back(0);
			}
	}
}

bool parser(string file, vector<object>* objects, vector<float>* lights, vector<float>* lightIntensities)
{
	ifstream inFile(file);

	if (! inFile)
	{
		cerr << "unable to open input file\n";
		return false;
	}

	string line;
	Material m;
	m.spec=vec4(1);
	m.phong=1;
	m.reflectance=0;
	m.refraction=1;
	while(!inFile.eof())
	{
		getline(inFile, line);

		if(!(line[0]=='#'))
		{
			string word;
			string object = "";
			
			istringstream processor(line);
			processor>>word;
			
			if(isValidObject(word) && word != "light")
			{
				while(line!="}")
				{
					object += line;
					getline(inFile,line);
				}			

				object+=" } \n";
				objects->push_back(buildObject(object, m));
			}

			else if(word=="light")
			{
				string info = "";
				while(line!="}")
				{	
					info += line;
					getline(inFile,line);
				}		
				addLight(info, lights, lightIntensities);
			}

			else if(word=="material")
			{
				string material = "";
				while(line!="}")
				{	
					material += line;
					getline(inFile,line);
				}		
				buildMaterial(material, &m);
			}			
		}
	}
	return true;
}
// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

vector<object> objects;
vector<float> lights;
vector<float> lightIntensities;

float r=0;
float phi=0;
vec3 camPos = vec3(0); 
mat3 ry = mat3	(cos(r), 0, sin(r),
				 0,		1,		0,
				 -sin(r), 0, cos(r));

mat3 rx = mat3 (1, 0, 0,
				0, cos(phi), -sin(phi),
				0, sin(phi), cos(phi));

float fov = M_PI/3;
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_W)
	{
		vec3 pos =vec3(0, 0, 0.1);
		
		pos = ry*pos;

		camPos-=pos;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}

	if (key == GLFW_KEY_A )
	{
		vec3 pos =vec3(0.1, 0, 0);

		pos = ry*pos;
		
		camPos-=pos;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}

	if (key == GLFW_KEY_D )
	{
		vec3 pos =vec3(0.1, 0, 0);

		pos = ry*pos;
		
		camPos+=pos;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}

	if (key == GLFW_KEY_S )
	{
		vec3 pos =vec3(0, 0, 0.1);

		pos = ry*pos;
		
		camPos+=pos;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}

	if (key == GLFW_KEY_E )
	{
		camPos[1] += 0.1;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}

	if (key == GLFW_KEY_Q )
	{
		camPos[1] -= 0.1;
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}
		
	if (key == GLFW_KEY_LEFT )
	{
		r-=0.1;
		ry = mat3	(cos(r), 0, sin(r),
				 0,		1,		0,
				 -sin(r), 0, cos(r));
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "theta");
		glUniform1f(loc, r);
	}

	if (key == GLFW_KEY_RIGHT )
	{
		r+=0.1;
		ry = mat3	(cos(r), 0, sin(r),
				 0,		1,		0,
				 -sin(r), 0, cos(r));
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "theta");
		glUniform1f(loc, r);
	}

	if (key == GLFW_KEY_UP )
	{
		phi+=0.1;

		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "phi");
		glUniform1f(loc, phi);
	}

	if (key == GLFW_KEY_DOWN )
	{
		phi-=0.1;

		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "phi");
		glUniform1f(loc, phi);
	}
	
	if (key==GLFW_KEY_1 && action==GLFW_PRESS)
	{
		objects.clear();
		lights.clear();
		lightIntensities.clear();
		
		parser("Scenes/scene1.txt", &objects, &lights,&lightIntensities);	
		setObjects(objects, lights, lightIntensities);
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "ambientLight");
		glUniform1f(loc, 1);
	}
	
	if (key==GLFW_KEY_2 && action==GLFW_PRESS)
	{
		objects.clear();
		lights.clear();
		lightIntensities.clear();
		
		parser("Scenes/scene2.txt", &objects, &lights,&lightIntensities);	
		setObjects(objects, lights, lightIntensities);
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "ambientLight");
		glUniform1f(loc, 3);
	}

	if (key==GLFW_KEY_3 && action==GLFW_PRESS)
	{
		objects.clear();
		lights.clear();
		lightIntensities.clear();
		
		parser("Scenes/scene3.txt", &objects, &lights,&lightIntensities);	
		setObjects(objects, lights, lightIntensities);
		
		glUseProgram(shader.program);
		GLuint loc = glGetUniformLocation(shader.program, "ambientLight");
		glUniform1f(loc, 3);

		camPos=vec3(0, 4, 14);
		loc = glGetUniformLocation(shader.program, "cameraPos");
		glUniform3f(loc, camPos[0], camPos[1], camPos[2]);
	}
}
// handles scroll weel input
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov += yoffset/10;

	glUseProgram(shader.program);
	GLuint loc = glGetUniformLocation(shader.program, "fieldOfView");
	glUniform1f(loc, atan(fov)+M_PI/2);
}

bool pan = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{	
	
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	
}
// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1000, 1000, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwMakeContextCurrent(window);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	if (!InitializeShaders(&shader, "fragment.glsl")) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}
	
	// call function to create and fill buffers with geometry data
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to intialize geometry!" << endl;

	parser("Scenes/scene1.txt", &objects, &lights,&lightIntensities);	
	setObjects(objects, lights, lightIntensities);
	// run an event-triggered main loop
	
	while (!glfwWindowShouldClose(window))
	{

		// call function to draw our scene
		RenderScene(&geometry, &shader); //render scene with texture

		glfwSwapBuffers(window);

		glfwPollEvents();
		//	usleep(2000);
	}


	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}


//reference code
//	MyTexture t;
//	if(!InitializeTexture(&t, "Images/test.png", GL_TEXTURE_RECTANGLE))
//		cout << "Program failed to intialize texture!" << endl;

	// call function to create and fill buffers with geometry data
	//if (!InitializeGeometry(&geometry))
		//cout << "Program failed to intialize geometry!" << endl;
	//RenderScene(&geometry, &texture, &shader);

//	GLuint frameBuffer;
//	glGenFramebuffers(1, &frameBuffer);
//	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);


//	texture.target = GL_TEXTURE_2D;
	//GLuint emptyTexture = createTexture(window, texture.target);

//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, emptyTexture, 0);

//	RenderScene(&geometry, &t, &shader);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//texture.textureID = emptyTexture;

/*
	GLuint linearTexture = create1DTexture(window, GL_TEXTURE_1D, &testVec);

	glUseProgram(shader.program);
	GLuint loc = glGetUniformLocation(shader.program, "tex");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(shader.program, "objects");
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(shader.program, "numOfObj");
	glUniform1i(loc, testVec.size());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, linearTexture);


	glActiveTexture(GL_TEXTURE0);*/
	
	/*GLuint create1DTexture(GLFWwindow* window, GLuint target = GL_TEXTURE_1D, vector<vec4> *v=NULL)
{
		//texture->target = target;
		GLuint emptyTexture;
		glGenTextures(1, &emptyTexture);
		glBindTexture(target, emptyTexture);
		GLuint format = GL_RGB;
		//cout << numComponents << endl;
		glTexImage1D(target, 0, format, v->size(), 0, format, GL_UNSIGNED_BYTE, v);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//texture->textureID = emptyTexture; 

		// Clean up
		glBindTexture(target, 0);

		return emptyTexture;
}*/

/*GLuint createEmptyTexture(GLFWwindow* window, GLuint target = GL_TEXTURE_2D)
{
		//texture->target = target;
		GLuint emptyTexture;
		glGenTextures(1, &emptyTexture);
		glBindTexture(target, emptyTexture);
		GLuint format = GL_RGB;
		//cout << numComponents << endl;
		glTexImage2D(target, 0, format, 1000, 1000, 0, format, GL_UNSIGNED_BYTE, NULL);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//texture->textureID = emptyTexture; 

		// Clean up
		glBindTexture(target, 0);

		return emptyTexture;
}*/
