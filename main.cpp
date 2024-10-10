#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb/stb_image.h>

#include <glm/glm.hpp>

#include <iostream>
#include <random>

constexpr auto MOVING_SPEED = 0.008f;
constexpr auto MARGIN = 0.0f;
constexpr auto WINDOW_SIZE = 800;

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
    FragColor = 1 - texture(texture1, TexCoord);
}
)";

void checkCompileErrors(unsigned int shader, std::string type)
{
	int success;
	char infoLog[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
}
unsigned int createShaderProgram()
{
	unsigned int vertexShader, fragmentShader;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	checkCompileErrors(vertexShader, "VERTEX");

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	checkCompileErrors(fragmentShader, "FRAGMENT");

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	checkCompileErrors(shaderProgram, "PROGRAM");

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint VAO, VBO, texture, shaderProgram;
float direction;
float vertices[16];
inline void frame(GLFWwindow* window)
{
	glfwPollEvents();

	glm::vec2 directionVec = glm::normalize(glm::vec2(glm::cos(glm::radians(direction)), glm::sin(glm::radians(direction)))) * MOVING_SPEED;
	for (int i = 0; i < 16; i += 4)
	{
		vertices[i] += directionVec.x;
		vertices[i + 1] += directionVec.y;
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);

	glm::vec2 lowerLeftPoint = glm::vec2(vertices[8], vertices[9]);
	glm::vec2 upperRightPoint = glm::vec2(vertices[0], vertices[1]);

	bool collisionX = lowerLeftPoint.x <= -1.0f + MARGIN || upperRightPoint.x >= 1.0f - MARGIN;
	bool collisionY = lowerLeftPoint.y <= -1.0f + MARGIN || upperRightPoint.y >= 1.0f - MARGIN;

	if (collisionX)
	{
		direction = 180.0f - direction;
		if (direction < 0) direction += 360.0f;
	}
	if (collisionY)
	{
		direction = 360.0f - direction;
	}
}

int lastWidth{ WINDOW_SIZE }, lastHeight{ WINDOW_SIZE };
void resizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	float scaleX = float(lastWidth) / width;
	float scaleY = float(lastHeight) / height;

	for (int i = 0; i < 16; i += 4)
	{
		vertices[i] *= scaleX;
		vertices[i + 1] *= scaleY;
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	lastWidth = width;
	lastHeight = height;

	frame(window);
}


int main()
{
	std::srand(unsigned(time(nullptr)));
	direction = float((std::rand() * std::rand() / std::rand()) % 360);

	if (!glfwInit()) std::cout << "GLFW Init Error!\n";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "DvD", NULL, NULL);
	if (!window) std::cout << "Window Creation Failed!\n";
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, resizeCallback);
	if (!gladLoadGL()) std::cout << "glad loading failed!\n";

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("dvdlogo.png", &width, &height, &nrChannels, 0);
	if (data)
	{

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	float uniformScale = .5f;
	float halfWidth = 0.5f * aspectRatio * uniformScale;
	float newVertices[] = {
		// positions                     // texture coords
		 halfWidth,  0.5f * uniformScale, 1.0f, 1.0f,  // top right
		 halfWidth, -0.5f * uniformScale, 1.0f, 0.0f,  // bottom right
		-halfWidth, -0.5f * uniformScale, 0.0f, 0.0f,  // bottom left
		-halfWidth,  0.5f * uniformScale, 0.0f, 1.0f   // top left 
	};
	std::move(newVertices, newVertices + 16, vertices);

	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	shaderProgram = createShaderProgram();

	while (!glfwWindowShouldClose(window))
	{
		frame(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}