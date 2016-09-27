#include "scene.h"
#include "camera.h"
#include "bvh.h"
#include "maths.h"
#include "render.h"
#include "loader.h"
#include "util.h"

#if _WIN32

#include "freeglut/include/GL/glut.h"

#elif __APPLE__

#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 
#include <opengl/gl3.h>
#include <glut/glut.h>

#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLRenderers.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/OpenGL.h>

#endif

#include <iostream>

using namespace std;

Vec3 g_camPos;
Vec3 g_camAngle;
Mat44 g_camTransform;

float g_flySpeed = 0.5f;
bool g_flyMode = false;

// renderer
Renderer* g_renderer;

// the main scene
Scene g_scene;
Options g_options;
Camera g_camera;

// output buffers
Color* g_pixels;
Color* g_filtered;

// 
int g_iterationCount;


double GetSeconds();

void Render()
{
    Camera camera;

    if (g_flyMode)
    {
        camera.position = g_camPos;
        camera.rotation = Quat(Vec3(0.0f, 1.0f, 0.0f), g_camAngle.x)*Quat(Vec3(1.0f, 0.0f, 0.0f), g_camAngle.y);
        camera.fov = 35.f;

        g_camTransform = Transform(camera.position, camera.rotation);
    }
    else
    {
        camera = g_camera;
    }


    double startTime = GetSeconds();

	const int numSamples = 1;

    // take one more sample per-pixel each frame for progressive rendering
    g_renderer->Render(g_camera, g_options, g_pixels);

    double endTime = GetSeconds();

    //printf("g_pixels[0] = %f %f %f\n", g_pixels[0].x/g_pixels[0].w, g_pixels[0].y/g_pixels[0].w, g_pixels[0].z/g_pixels[0].w);
    //printf("g_pixels[0] = %f %f %f\n", g_pixels[0].x, g_pixels[0].y, g_pixels[0].z);

    printf("%d (%.4fms)\n", g_iterationCount, (endTime-startTime)*1000.0f);
    fflush(stdout);
                

    Color* presentMem = g_pixels;

    g_iterationCount += numSamples;

    if (g_options.mode == ePathTrace)
    {
        int numPixels = g_options.width*g_options.height;

        for (int i=0; i < numPixels; ++i)
        {            
            float s = g_options.exposure / g_pixels[i].w;

            g_filtered[i] = LinearToSrgb(g_pixels[i] * s);
        }

        presentMem = g_filtered;
    }

    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    //glPixelZoom(float(g_windowWidth)/g_options.width, float(g_windowHeight)/g_options.height);
    glDrawPixels(g_options.width, g_options.height, GL_RGBA, GL_FLOAT, presentMem);
}

void InitFrameBuffer()
{
    delete[] g_pixels;
    delete[] g_filtered;

    g_pixels = new Color[g_options.width*g_options.height];
    g_filtered = new Color[g_options.width*g_options.height];

    g_iterationCount = 0;

	printf("%d %d\n", g_options.width, g_options.height);

	g_renderer->Init(g_options.width, g_options.height);
}

#include "tests/testMaterials.h"
#include "tests/testMesh.h"
#include "tests/testMotionBlur.h"

void Init()
{
#if _WIN32
    // create renderer
    g_renderer = CreateGpuRenderer(&g_scene);
#else
    g_renderer = CreateCpuRenderer(&g_scene);
#endif

    InitFrameBuffer();
}

void GLUTUpdate()
{
    Render();

	// flip
	glutSwapBuffers();
}

void GLUTReshape(int width, int height)
{
    g_options.width = width;
    g_options.height = height;

    InitFrameBuffer();
}

void GLUTArrowKeys(int key, int x, int y)
{
}

void GLUTArrowKeysUp(int key, int x, int y)
{
}

void GLUTKeyboardDown(unsigned char key, int x, int y)
{
    bool resetFrame = false;

 	switch (key)
	{
    case 'w':
        g_camPos -= Vec3(g_camTransform.GetCol(2))*g_flySpeed; resetFrame = true;
		break;
    case 's':
        g_camPos += Vec3(g_camTransform.GetCol(2))*g_flySpeed; resetFrame = true; 
        break;
    case 'a':
        g_camPos -= Vec3(g_camTransform.GetCol(0))*g_flySpeed; resetFrame = true;
        break;
    case 'd':
        g_camPos += Vec3(g_camTransform.GetCol(0))*g_flySpeed; resetFrame = true;
        break;
	case '1':
		g_options.mode = eNormals;
		break;
	case '2':
		g_options.mode = eComplexity;
		break;
    case '3':
        g_options.mode = ePathTrace; resetFrame = true;
        break;
	case '[':
		g_options.exposure -= 0.01f;
		break;
	case ']':
		g_options.exposure += 0.01f;
		break;
	case 'q':
	case 27:
		exit(0);
		break;
	};

    // reset image if there are any camera changes
    if (resetFrame == true)
    {
        InitFrameBuffer();
    }
}

void GLUTKeyboardUp(unsigned char key, int x, int y)
{
// 	switch (key)
// 	{
// 	case 27:
// 		exit(0);
// 		break;
// 	};

}

static int lastx;
static int lasty;

void GLUTMouseFunc(int b, int state, int x, int y)
{
	switch (state)
	{
	case GLUT_UP:
		{
			lastx = x;
			lasty = y;			
		}
	case GLUT_DOWN:
		{
			lastx = x;
			lasty = y;
		}
	}
}

void GLUTMotionFunc(int x, int y)
{
    int dx = x-lastx;
    int dy = y-lasty;

    const float sensitivity = 0.01f;

    g_camAngle.x -= dx*sensitivity;
    g_camAngle.y -= dy*sensitivity;

	lastx = x;
	lasty = y;

    if (g_options.mode == ePathTrace)
    {
        InitFrameBuffer();
    }
}


/*
void Application::JoystickFunc(int x, int y, int z, unsigned long buttons)
{
g_app->JoystickFunc(x, y, z, buttons);
}
*/

void ProcessCommandLine(int argc, char* argv[])
{

}

int main(int argc, char* argv[])
{	
    // set up defaults
    g_options.width = 512;
    g_options.height = 256;
    g_options.filter = Filter(eFilterGaussian, 1.0f, 2.0f);
    g_options.numSamples = 1;
    g_options.mode = eNormals;
    g_options.exposure = 1.0f;

    g_camera.position = Vec3(0.0f, 1.0f, 5.0f);
    g_camera.rotation = Quat();
    g_camera.fov = DegToRad(35.0f);

    // the last argument should be the input file
    const char* filename = NULL;

    for (int i=1; i < argc; ++i)
    {
        filename = argv[i];
    }

    if (filename)
    {
        bool success = LoadTin(filename, &g_scene, &g_camera, &g_options);
        
        if (!success)
        {
            printf("Couldn't open %s for reading.\n", filename);
            exit(-1);
        }
    }
    else
    {
        // default test scene
        TestMaterials(&g_scene, &g_camera, &g_options);
    }

    // allow command line to override options
    ProcessCommandLine(argc, argv);

	// init gl
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	glutInitWindowSize(g_options.width, g_options.height);
	glutCreateWindow("Tinsel");
	glutPositionWindow(200, 200);

    Init();

    glutMouseFunc(GLUTMouseFunc);
	glutReshapeFunc(GLUTReshape);
	glutDisplayFunc(GLUTUpdate);
	glutKeyboardFunc(GLUTKeyboardDown);
	glutKeyboardUpFunc(GLUTKeyboardUp);
	glutIdleFunc(GLUTUpdate);	
	glutSpecialFunc(GLUTArrowKeys);
	glutSpecialUpFunc(GLUTArrowKeysUp);
	glutMotionFunc(GLUTMotionFunc);

	glutMainLoop();
}



 