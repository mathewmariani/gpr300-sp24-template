#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ew/camera.h"
#include "ew/cameraController.h"
#include "ew/shader.h"
#include "ew/model.h"
#include "ew/texture.h"
#include "ew/transform.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();


static float quad_vertices[] = {
	// pos (x, y) texcoord (u, v)
	-1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 1.0f, 0.0f,

	-1.0f,  1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f,  1.0f, 1.0f, 1.0f,
};

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

// camera
ew::Camera camera;
ew::CameraController cameraController;
ew::Transform transform;
GLuint texture;

struct FullscreenQuad {
	GLuint vao;
	GLuint vbo;
} fullscreen_quad;
// FullscreenQuad fullscreen_quad;

struct Material {
	float Ka = 1.0; 
	float Kd = 0.5; 
	float Ks = 0.5;
	float Shininess = 128;
} material;


struct Framebuffer {
	GLuint fbo;
	GLuint color0;
	GLuint color1;
	GLuint depth;
} framebuffer;

void render(ew::Shader& shader, ew::Model& model)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	// clear framebuffer 
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// describe pipeline
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	//RENDER
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set bindings
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	shader.use();

	// material properties
	shader.setInt("texture0",0);
	shader.setFloat("material.Ka", material.Ka);
	shader.setFloat("material.Kd", material.Kd);
	shader.setFloat("material.Ks", material.Ks);
	shader.setFloat("material.Shininess", material.Shininess);

	// camera
	shader.setVec3("_EyePos", camera.position);

	// scene matrices
	auto viewproj = camera.projectionMatrix() * camera.viewMatrix();
	shader.setMat4("model", transform.modelMatrix());
	shader.setMat4("view_proj", viewproj);

	// draw
	model.draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#define DEFAULT_FB (0)

int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// initialize resources
	ew::Shader blinnphong = ew::Shader("assets/blinnphong.vert", "assets/blinnphong.frag");
	ew::Model suzanne = ew::Model("assets/Suzanne.obj");
	texture = ew::loadTexture("assets/brick_color.jpg");

	ew::Shader fullscreen_shader = ew::Shader("assets/fullscreen.vert", "assets/fullscreen.frag");
	ew::Shader inverse_shader = ew::Shader("assets/inverse.vert", "assets/inverse.frag");
	ew::Shader grayscale_shader = ew::Shader("assets/fullscreen.vert", "assets/grayscale.frag");
	ew::Shader blur_shader = ew::Shader("assets/blur.vert", "assets/blur.frag");
	ew::Shader chromatic_shader = ew::Shader("assets/blur.vert", "assets/chromatic.frag");

	// initialize camera
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.aspectRatio = (float)screenWidth/screenHeight;
	camera.fov = 60.0f;

	// initialize fullscreen quad, buffer object
	glGenVertexArrays(1, &fullscreen_quad.vao);
	glGenBuffers(1, &fullscreen_quad.vbo);

    // bind vao, and vbo
	glBindVertexArray(fullscreen_quad.vao);
	glBindBuffer(GL_ARRAY_BUFFER, fullscreen_quad.vbo);

	// buffer data to vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0); // positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1); // texcoords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (sizeof(float) * 2));

	glBindVertexArray(0);

	// initialize framebuffer
	glGenFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	// color attachment
	glGenTextures(1, &framebuffer.color0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.color0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.color0, 0);

	// check completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Not so victorious\n");
		return 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		// update the camera
		cameraController.move(window, &camera, deltaTime);

		// rotate suzanne
		transform.rotation = glm::rotate(transform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		// render the scene
		render(blinnphong, suzanne);

		// fullscreen quad pipeline:
		glDisable(GL_DEPTH_TEST);

		// clear default buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// >> render fullscreen quad
		// inverse_shader.use();
		// inverse_shader.setInt("texture0", 0);

		// grayscale_shader.use();
		// grayscale_shader.setInt("texture0", 0);

		blur_shader.use();
		blur_shader.setInt("texture0", 0);



		glBindVertexArray(fullscreen_quad.vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer.color0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0,0,5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");

	if (ImGui::Button("Reset Camera")){
		resetCamera(&camera, &cameraController);
	}

	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}

	ImGui::Image((ImTextureID)(intptr_t)framebuffer.color0, ImVec2(800, 600));

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

