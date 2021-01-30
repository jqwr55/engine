#include <Types.h>

void ModKeyCallBack(GLFWwindow* window , int key, int scancode, int action, int mods);
void TextCallBack(GLFWwindow* window , uint codePoint);
void MouseCallBack(GLFWwindow* w, double x ,double y);
void WindowCloseCallback(GLFWwindow* window);
void WindowResizeCallBack(GLFWwindow* window , int w , int h);
void RenderLoop(Renderer* r);
