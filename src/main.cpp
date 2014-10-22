
#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <stdlib.h>
#include <stdio.h>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

#include "IndexedFaceSet.h"
#include "tetgen.h"
#include "Shader.h"
#include "Renderable.h"
#include "render_utils.h"
#include "tetmesh/tetmesh.h"

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 810
#define FOV 45.0f
#define FRAME_RATE 60
#define PI 3.14159f

int main() {

    printf("Creating OpenGL context...\n");
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 2;
    sf::Window window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "DSC Demo", sf::Style::Default, settings);

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
    // tetgen turns faces CW
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("Loading basic mesh...\n");
    IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/sphere.obj");

    printf("Generating tet mesh...\n");
    TetMesh * tet_mesh = TetMesh::from_indexed_face_set(*mesh);

    printf("Saving mesh to output/ ...\n");
    tet_mesh->save("output/sphere");

    printf("Evolving tet mesh...\n");
    for (int i = 0; i < tet_mesh->num_vertices; i++) {
        if (tet_mesh->vertex_statuses[i] == 2) {
            // scale and translate x
            tet_mesh->vertex_targets[i * 3] = tet_mesh->vertices[i * 3] * 1.001;
        }
    }
    tet_mesh->evolve();
    tet_mesh->save("output/evolved_sphere");
    delete tet_mesh;

    printf("Displaying original mesh...\n");

    printf("Initializing display...\n");
    Shader * shader = Shader::compile_from("shaders/wire.vsh", "shaders/wire.gsh", "shaders/wire.fsh");
    glUseProgram(shader->get_id());
    Renderable renderable(shader, true, false, GL_TRIANGLES);
    mesh->bind_attributes(renderable);
    check_gl_error();

    // glm::mat4 model_transform = glm::scale(glm::mat4(), glm::vec3(0.05f, 0.05f, 0.05f));
    glm::mat4 model_transform = glm::mat4();
    glm::vec3 eye = glm::vec3(2, 2, 6);
    glm::vec3 focus = glm::vec3(0, 1, 0);
    glm::vec3 up = glm::vec3(0, 1, 0);
    glm::mat4 view_transform = glm::lookAt(eye, focus, up);
    glm::mat4 perspective_transform = glm::perspective(FOV, ((float) WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 MVP = perspective_transform * view_transform * model_transform;
    renderable.bind_uniform(&MVP[0][0], MAT4_FLOAT, 1, "MVP");
    check_gl_error();

    printf("Starting display...\n");
    sf::Event event;
    sf::Clock clock;
    while (window.isOpen()) {
        // float frame_length = clock.restart().asSeconds();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            eye = glm::rotate(eye, 5 * PI / 180, up);
            view_transform = glm::lookAt(eye, focus, up);
            MVP = perspective_transform * view_transform * model_transform;
            renderable.bind_uniform(&MVP[0][0], MAT4_FLOAT, 1, "MVP");
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            eye = glm::rotate(eye, -5 * PI / 180, up);
            view_transform = glm::lookAt(eye, focus, up);
            MVP = perspective_transform * view_transform * model_transform;
            renderable.bind_uniform(&MVP[0][0], MAT4_FLOAT, 1, "MVP");
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (window.isOpen()) {
            renderable.render();
        }
        check_gl_error();

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                printf("Closing window...\n");
                window.close();
            }
        }

        if (window.isOpen()) {
            window.display();
            sf::sleep(sf::seconds(1.0f / FRAME_RATE));
        }
    }

    printf("Cleaning up...\n");
    delete shader;
    delete mesh;

    return 0;
}
