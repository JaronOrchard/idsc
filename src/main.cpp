
#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <stdlib.h>
#include <stdio.h>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "IndexedFaceSet.h"
#include "tetgen.h"
#include "Shader.h"
#include "Renderable.h"
#include "render_utils.h"

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 810
#define FOV 45.0f
#define FRAME_RATE 60

int main() {

    printf("Creating OpenGL context...\n");
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 2;
    sf::Window window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Cloth Simulation", sf::Style::Default, settings);

    printf("Initializing OpenGL...\n");
    glewExperimental = GL_TRUE;
    glewInit();

    printf("VENDOR = %s\n", glGetString(GL_VENDOR));
    printf("RENDERER = %s\n", glGetString(GL_RENDERER));
    printf("VERSION = %s\n", glGetString(GL_VERSION));

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glClearColor(0.0f, 0.2f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("Loading basic mesh...\n");
    // IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/cube.obj");
    tetgenio in;
    in.load_poly("assets/models/example");

    printf("Generating tet mesh...\n");
    // tetgenio * in = IndexedFaceSet::to_tetgenio(*mesh);
    // delete mesh;
    tetgenio out;
    tetgenbehavior switches;
    switches.parse_commandline("");
    tetrahedralize(&switches, &in, &out);
    IndexedFaceSet * tetmesh = IndexedFaceSet::from_tetgenio(out);

    printf("Initializing display...\n");
    Shader * shader = Shader::compile_from("shaders/wire.vsh", "shaders/wire.gsh", "shaders/wire.fsh");
    glUseProgram(shader->get_id());
    Renderable renderable(shader, true, false, GL_TRIANGLES);
    tetmesh->bind_attributes(renderable);
    check_gl_error();

    glm::mat4 model_transform = glm::mat4();
    glm::mat4 view_transform = glm::lookAt(glm::vec3(10, 30, 30), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 perspective_transform = glm::perspective(FOV, ((float) WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 MVP = perspective_transform * view_transform * model_transform;
    renderable.bind_uniform(&MVP[0][0], MAT4_FLOAT, 1, "MVP");
    check_gl_error();

    printf("Starting display...\n");
    sf::Event event;
    sf::Clock clock;
    while (window.isOpen()) {
        window.display();

        sf::sleep(sf::seconds(1.0f / FRAME_RATE));

        // float frame_length = clock.restart().asSeconds();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        check_gl_error();

        renderable.render();
        check_gl_error();

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                printf("Closing window...\n");
                window.close();
            }
        }
    }

    printf("Cleaning up...\n");
    delete shader;
    delete tetmesh;

    return 0;
}
