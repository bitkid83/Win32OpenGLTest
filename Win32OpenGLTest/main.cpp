#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#ifdef _WIN32
	#define APIENTRY __stdcall
#endif

#include <glad/glad.h>
#include <glad/glad_wgl.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glad.lib")

#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

 static const GLfloat gVertices[] = {
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,

	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,

	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f
};
 
 GLfloat gNormals[12] = { 0.0 };

static const int32_t gAttribs[] =
{
	WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
	WGL_CONTEXT_MINOR_VERSION_ARB, 5,
	WGL_CONTEXT_FLAGS_ARB, 0,
	0
};

const float gFov = 80.0f;
bool gRunning = true;

void calculate_normals()
{
	glm::vec3 vector1, vector2;
	glm::vec3 norm;
	int count = 0;
	int nindex = 0;
			
	for (int j = 0; j < (j + 9) && (j < 36); j += 9) {
		vector1.x = gVertices[j] - gVertices[j + 3];
		vector1.y = gVertices[j + 1] - gVertices[j + 4];
		vector1.z = gVertices[j + 2] - gVertices[j + 5];
		
		vector2.x = gVertices[j + 3] - gVertices[j + 6];
		vector2.y = gVertices[j + 4] - gVertices[j + 7];
		vector2.z = gVertices[j + 5] - gVertices[j + 8];
		norm = glm::cross(vector1, vector2);

		gNormals[nindex] = norm.x;
		gNormals[nindex + 1] = norm.y;
		gNormals[nindex + 2] = norm.z;
		nindex += 3;
	}
}

LONG WINAPI WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct window_info {
	int32_t width, height;
	int32_t xpos, ypos;
	const char *title;
};

static const char *window_class = "Win32OpenGLTest";
static const char *window_name = "OpenGL Test App";

struct gl_platform_ctx {
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
};

struct gl_workingdata {
	GLuint VBO;
	GLuint VAO;
	GLuint NORM;
	GLuint shaderProg;
};


float rot = 0.0f;

static inline window_info *init_window_info(int32_t width, int32_t height, const char *title)
{
	window_info *info = (window_info *)malloc(sizeof(window_info));
	info->title = title;
	info->width = width;
	info->height = height;
	info->xpos = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	info->ypos = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	return info;
}

static inline bool register_window_class()
{
	WNDCLASSA wc = { 0 };	
	
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_GLOBALCLASS;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = window_class;

	if (!RegisterClassA(&wc)) {
		OutputDebugStringA("Error registering window class!\n");
		return false;
	}

	return true;
}

static inline bool create_gl_window(struct window_info *info, struct gl_platform_ctx *plat)
{
	if (info && plat) {
		const char *window_title = info->title;
		int32_t w = info->width;
		int32_t h = info->height;
		int32_t x = info->xpos;
		int32_t y = info->ypos;

		plat->hwnd = CreateWindowExA(	0, window_class, window_title, 
										WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
										x, y, w, h, 
										NULL, 
										NULL, 
										GetModuleHandleW(NULL),
										NULL );
		if (!plat->hwnd) {
			OutputDebugStringA("Error creating window!\n"); //GetLastError()
			return false;
		}
	}
	else {
		OutputDebugStringA("Bad window info or platform context passed to create window!\n");
		return false;
	}

	plat->hdc = GetDC(plat->hwnd);
	if (!plat->hdc) {
		OutputDebugStringA("Error getting DC from window!\n");
		return false;
	}

	return true;
}

static inline bool init_gl_context(struct gl_platform_ctx *plat)
{
	if (!plat) return false;

	plat->hglrc = wglCreateContext(plat->hdc);
	if (!plat->hglrc) {
		OutputDebugStringA("Error creating OpenGL context!\n");
		return false;
	}

	if (!wglMakeCurrent(plat->hdc, plat->hglrc)) {
		wglDeleteContext(plat->hglrc);
		return false;
	}

	return true;
}

static inline bool set_pixel_format_and_swap_chain(HDC hdc)
{
	// pixel format desc
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; // | PFD_DOUBLEBUFFER;

	int format_index = ChoosePixelFormat(hdc, &pfd);
	if (!format_index) {
		return false;
	}

	if (!SetPixelFormat(hdc, format_index, &pfd)) {
		return false;
	}

	DescribePixelFormat(hdc, format_index, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	return true;
}

GLuint compile_shader_from_file(const char *path, GLenum shaderType)
{
	std::string contents;
	std::ifstream shaderStream(path, std::ios::in);
	if (!shaderStream.is_open()) {
		// error opening file
		return 0;
	}

	std::string line;
	while (!shaderStream.eof()) {
		std::getline(shaderStream, line);
		contents.append(line + "\r\n");
	}
	shaderStream.close();

	const GLchar *shaderString = (GLchar *)contents.c_str();
	GLuint shaderBytes = glCreateShader(shaderType);
	glShaderSource(shaderBytes, 1, &shaderString, NULL);
	glCompileShader(shaderBytes);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(shaderBytes, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shaderBytes, 512, NULL, infoLog);
		OutputDebugStringA(infoLog);
		return 0;
	}	

	return shaderBytes;
}

GLuint create_shader_program(GLuint *vertexShader, GLuint *fragmentShader)
{
	GLuint shaderProgram;
	GLint success;
	GLchar infoLog[512];
	
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, *vertexShader);
	glAttachShader(shaderProgram, *fragmentShader);
	glLinkProgram(shaderProgram);
	
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		OutputDebugStringA(infoLog);
		return 0;
	}
	
	glDeleteShader(*vertexShader);
	glDeleteShader(*fragmentShader);
	
	return shaderProgram;
}

void setup_scene(gl_workingdata *gldata)
{	
	if (!gldata) { return; }

	// vertex buffer object	
	glGenVertexArrays(1, &gldata->VAO);
	glGenBuffers(1, &gldata->VBO);
	glGenBuffers(1, &gldata->NORM);

	glBindVertexArray(gldata->VAO);
	{		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, gldata->VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(gVertices), gVertices, GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, gldata->NORM);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(gNormals), gNormals, GL_STATIC_DRAW);
	}
	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	calculate_normals();
}



void display_scene(gl_platform_ctx *plat, gl_workingdata *gldata)
{	
	POINT mouse;
	float mxRatio = 0.0f;
	float myRatio = 0.0f;
	GetCursorPos(&mouse);
	if (ScreenToClient(plat->hwnd, &mouse)) {
		mxRatio = (float)((float)mouse.x / (float)WINDOW_WIDTH);
		myRatio = (float)((float)mouse.y / (float)WINDOW_HEIGHT);
	}
	
	if (!gldata) { return; } // oops! D:
	GLuint prog = gldata->shaderProg;
	GLuint vao = gldata->VAO;
	
	glUseProgram(prog);
		
	// transform matrices
	glm::mat4 model, view, proj, mvp, mv;

	// camera
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// light
	//glm::vec3 lightPosition = glm::vec3(mxRatio, myRatio, mxRatio / myRatio * 1.5f);
	glm::vec3 lightPosition = glm::vec3(0.f, 0.f, 2.5f);
	
	// shader locations
	//GLint modelLoc = glGetUniformLocation(prog, "model");
	//GLint viewLoc = glGetUniformLocation(prog, "view");
	//GLint projLoc = glGetUniformLocation(prog, "projection");
	GLint mvpLoc = glGetUniformLocation(prog, "mvp");
	GLint mvLoc = glGetUniformLocation(prog, "mv");
	GLint colorLoc = glGetUniformLocation(prog, "input_color");
	GLint lightLoc = glGetUniformLocation(prog, "light_pos");

	
	// matrix transforms
	rot += 0.0001f;
	if (rot >= 360.0f) { rot = 0.0f; }
	model = glm::rotate(model, rot, glm::vec3(0.0f, 1.0f, 0.f)); // rotate the model
	model = glm::rotate(model, mxRatio * 4.f, glm::vec3(0.f, 0.f, 1.f)); // rotate the model
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // camera looks down -z axis, oriented on y axis
	//view = glm::rotate(view, -rot, glm::vec3(0.0f, 0.0f, 1.0f)); // rotate the view
	//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f)); // translate the view on -z axis
	proj = glm::perspective(gFov, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
	mv = view * model;
	mvp = proj * view * model;

	// set shader uniforms
	//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	//glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform3fv(lightLoc, 1, glm::value_ptr(lightPosition));
	glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
	
	// clear screen
	glClearColor(0, 0, 0, 255);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	// draw!
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 12);	
	glBindVertexArray(0);

	SwapBuffers(plat->hdc); // glFlush();
}

void check_gl_version(int32_t *maj, int32_t *min)
{
	int32_t majv = GLVersion.major;
	int32_t minv = GLVersion.minor;
	char gl_ver_string[32];
	wsprintfA(gl_ver_string, "OpenGL Version: %d.%d", majv, minv);
	OutputDebugStringA(gl_ver_string);

	*maj = majv;
	*min = minv;
}




int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{		
	window_info *wi = init_window_info(WINDOW_WIDTH, WINDOW_HEIGHT, window_name);
	gl_platform_ctx *plat = (gl_platform_ctx *)malloc(sizeof(gl_platform_ctx));
	gl_workingdata wrk = { 0 };

	if (!register_window_class()) {
		return 1;
	}
	if (!create_gl_window(wi, plat)) {
		return 1;
	}
	if (!set_pixel_format_and_swap_chain(plat->hdc)) {
		return 1;
	}
	if (!init_gl_context(plat)) {
		return 1;
	}
	if (!gladLoadGL()) {
		// glad failed despite context
		return 1;
	}
	ShowWindow(plat->hwnd, nShowCmd);
	SetForegroundWindow(plat->hwnd);
	SetFocus(plat->hwnd);

	GLuint vertexShader = compile_shader_from_file("test.vs", GL_VERTEX_SHADER);
	GLuint fragmentShader = compile_shader_from_file("test.fs", GL_FRAGMENT_SHADER);
	wrk.shaderProg = create_shader_program(&vertexShader, &fragmentShader);
	setup_scene(&wrk);


	int32_t maj, min;
	check_gl_version(&maj, &min);

	MSG msg;
	while (gRunning) {		
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			display_scene(plat, &wrk);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// shutdown stuff
	glDeleteVertexArrays(1, &wrk.VAO);
	glDeleteBuffers(1, &wrk.VBO);

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(plat->hwnd, plat->hdc);
	wglDeleteContext(plat->hglrc);
	DestroyWindow(plat->hwnd);
	if (plat) { free(plat); plat = NULL; }	
	if (wi) { free(wi);	wi = NULL; }
	
	return msg.wParam;
}

LONG WINAPI WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;

	switch (message) {	
	case WM_PAINT:
		//BeginPaint(hWnd, &ps);
		
		//EndPaint(hWnd, &ps);
		return 0;
	case WM_SIZE:
		glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
		PostMessage(hWnd, WM_PAINT, 0, 0);
		return 0;
	case WM_CHAR:
		// escape pressed
		switch (wParam) {
		case 27:
			gRunning = false;
			PostQuitMessage(0);
			break;
		}
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	/*default:
		return DefWindowProc(hWnd, message, wParam, lParam);*/
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}