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

	void Initialize()
	{
		float quad_vertices[] = {
			// pos (x, y) texcoord (u, v)
			-1.0f,  1.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f,
			1.0f, -1.0f, 1.0f, 0.0f,

			-1.0f,  1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 0.0f,
			1.0f,  1.0f, 1.0f, 1.0f,
		};

		// initialize fullscreen quad, buffer object
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		// bind vao, and vbo
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// buffer data to vbo
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
		
		// positions and texcoords
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (sizeof(float) * 2));

		glBindVertexArray(0);
	}
} fullscreen_quad;

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

	void Initialize()
	{
		// initialize framebuffer
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// color attachment
		glGenTextures(1, &color0);
		glBindTexture(GL_TEXTURE_2D, color0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Create depth texture
		glGenTextures(1, &depth);
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 800, 600, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color0, 0);

		// check completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			printf("Not so victorious\n");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
} framebuffer;


enum {
	EFFECT_NONE = 0,
	EFFECT_GREYSCALE = 1,
	EFFECT_BLUR = 2,
	EFFECT_INVERSE = 3,
	EFFECT_ABERRATION = 4,
};

static std::vector<std::string> post_processing_effects = {
    "None",
    "Grayscale",
    "Kernel Blur",
    "Inverse",
    "Chromatic Aberration",
    "CRT",
};

struct {
	int index = 0;

	struct {
		float strength = 16.0f;
	} blur;
	
	struct {
		glm::vec3 offset = glm::vec3(0.009f, 0.006f, -0.006f);
		glm::vec2 direction = glm::vec2(1.0f);
	} chromatic;
} effect;

void render(ew::Shader& shader, ew::Model& model)
{
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

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	// clear framebuffer 
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// describe pipeline
	glEnable(GL_DEPTH_TEST);

	// set bindings
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	// draw
	model.draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void post_process(ew::Shader& shader)
{
	shader.use();
	shader.setInt("texture0", 0);

	// what other uniforms should we send ?
	switch (effect.index)
	{
		case EFFECT_GREYSCALE:
			break;
		case EFFECT_BLUR:
			shader.setFloat("strength", effect.blur.strength);
			break;
		case EFFECT_INVERSE:
			break;
		case EFFECT_ABERRATION:
			shader.setVec3("offset", effect.chromatic.offset);
			shader.setVec2("direction", effect.chromatic.direction);
			break;
		default:
			break;
	}

	// fullscreen quad pipeline:
	glDisable(GL_DEPTH_TEST);

	// clear default buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);

	// draw fullscreen quad
	glBindVertexArray(fullscreen_quad.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.color0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// initialize resources
	ew::Shader blinnphong = ew::Shader("assets/blinnphong.vert", "assets/blinnphong.frag");
	ew::Model suzanne = ew::Model("assets/Suzanne.obj");
	texture = ew::loadTexture("assets/brick_color.jpg");

	std::vector<ew::Shader> effects = {
		ew::Shader("assets/fullscreen.vert", "assets/noprocess.frag"),
		ew::Shader("assets/fullscreen.vert", "assets/grayscale.frag"),
		ew::Shader("assets/fullscreen.vert", "assets/blur.frag"),
		ew::Shader("assets/fullscreen.vert", "assets/inverse.frag"),
		ew::Shader("assets/fullscreen.vert", "assets/chromatic.frag"),
	};

	// initialize camera
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.aspectRatio = (float)screenWidth/screenHeight;
	camera.fov = 60.0f;

	fullscreen_quad.Initialize();
	framebuffer.Initialize();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		// update the camera
		cameraController.move(window, &camera, deltaTime);

		// rotate suzanne
		transform.rotation = glm::rotate(transform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		// render the scene using blinnphong shader
		render(blinnphong, suzanne);

		// render fullscreen quad
		post_process(effects[effect.index]);

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

	if (ImGui::BeginCombo("Effect", post_processing_effects[effect.index].c_str()))
	{
		for (auto n = 0; n < post_processing_effects.size(); ++n)
		{
			auto is_selected = (post_processing_effects[effect.index] == post_processing_effects[n]);
			if (ImGui::Selectable(post_processing_effects[n].c_str(), is_selected))
			{
				effect.index = n;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Post Process Controls:");
	ImGui::Separator();
	if (effect.index == EFFECT_BLUR) {
		ImGui::SliderFloat("Strength", &effect.blur.strength, 0.0f, 32.0f);
	} else if (effect.index == EFFECT_ABERRATION) {
		ImGui::SliderFloat3("Offset", &effect.chromatic.offset[0], -1.0f, +1.0f);
		ImGui::SliderFloat2("Direction", &effect.chromatic.direction[0], -1.0f, +1.0f);
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

