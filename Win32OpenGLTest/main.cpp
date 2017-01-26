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
 
static const int32_t gAttribs[] =
{
	WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
	WGL_CONTEXT_MINOR_VERSION_ARB, 5,
	WGL_CONTEXT_FLAGS_ARB, 0,
	0
};

const float gFov = 90.0f;
bool gRunning = true;

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

GLuint VBO;
GLuint VAO;
GLuint shaderProgram;

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

void compile_shader_from_file(const char *path, GLenum shaderType, GLuint *data)
{
	std::string contents;
	std::ifstream shaderStream(path, std::ios::in);
	if (!shaderStream.is_open()) {
		// error opening file
		return;
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
	}
	
	*data = shaderBytes;

	return;
}

void setup_scene()
{	
	GLuint vertexShader;
	GLuint fragmentShader;
	compile_shader_from_file("test.vs", GL_VERTEX_SHADER, &vertexShader);
	compile_shader_from_file("test.fs", GL_FRAGMENT_SHADER, &fragmentShader);

	// link shader programs
	GLint success;
	GLchar infoLog[512];	
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		OutputDebugStringA(infoLog);
	}
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	
	// vertex buffer object	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);	
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVertices), gVertices, GL_STATIC_DRAW);

	glBindVertexArray(VAO);
	{
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);		
		glEnableVertexAttribArray(0);
	}
	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
}

void display_scene(HDC hdc)
{	
	POINT p;
	GetCursorPos(&p);
	glUseProgram(shaderProgram);
	
	GLint uniformColor = glGetUniformLocation(shaderProgram, "input_color");
	
	// camera
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// transform matrices
	glm::mat4 model, view, proj;
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

	rot += 0.0001f;
	if (rot >= 360.0f) { rot = 0.0f; }
	
	model = glm::rotate(model, rot, glm::vec3(1.0f, 0.0f, 0.0f)); // rotate the model
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // camera looks down -z axis, oriented on y axis
	//view = glm::rotate(view, -rot, glm::vec3(0.0f, 0.0f, 1.0f)); // rotate the view
	//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f)); // translate the view on -z axis

	proj = glm::perspective(gFov, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

	//glUniform3f(uniformColor, (float)(fabs(p.x / 2.0f) / 10000.f) * 2, (float)(p.y / 10000.f) * 2, 0.0f);
	glUniform3f(uniformColor, 1.0f, 0.0f, 0.0f);
	glClearColor(0, 0, 0, 255);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 12);	
	glBindVertexArray(0);

	SwapBuffers(hdc); //glFlush();
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
	window_info *wi = init_window_info(WINDOW_WIDTH, WINDOW_HEIGHT, window_name); // free me
	gl_platform_ctx *plat = (gl_platform_ctx *)malloc(sizeof(gl_platform_ctx)); // free me

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

	setup_scene();

	int32_t maj, min;
	check_gl_version(&maj, &min);

	MSG msg;
	while (gRunning) {		
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			display_scene(plat->hdc);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(plat->hwnd, plat->hdc);
	wglDeleteContext(plat->hglrc);
	DestroyWindow(plat->hwnd);
	free(plat);
	plat = NULL;

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