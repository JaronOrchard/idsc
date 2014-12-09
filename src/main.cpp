
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <tetgen.h>

#include "model/IndexedFaceSet.h"
#include "render/Shader.h"
#include "render/Renderable.h"
#include "render/TetrahedralViewer.h"
#include "render/render_utils.h"
#include "tetmesh/tetmesh.h"
#include "tetmesh/TetMeshFactory.h"
#include "util/geometrySet.h"

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 810
#define FOV 45.0f
#define FRAME_RATE 60

int main(int argc, char* argv[]) {

    printf("Creating OpenGL context...\n");
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 2;
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "DSC Demo", sf::Style::Default, settings);

    printf("Initializing OpenGL...\n");
    glewExperimental = GL_TRUE;
    glewInit();

    printf("VENDOR = %s\n", glGetString(GL_VENDOR));
    printf("RENDERER = %s\n", glGetString(GL_RENDERER));
    printf("VERSION = %s\n", glGetString(GL_VERSION));

    GLuint vao;
    glGenVertexArrays(1, &vao);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load/generate tet mesh based on command line argument:
    TetMesh * tet_mesh;
    printf("Generating tet mesh...\n");
    /*
     * 1: Sphere
     * 2: Debug tetmesh
     * 3: Big debug tetmesh
     * 4: Collapsed tetmesh
     */
    std::string meshArg = "1";
    if (argc >= 2) { meshArg = argv[1]; }

    if (meshArg == "2") { // Debug tetmesh
        tet_mesh = TetMeshFactory::create_debug_tetmesh();
    } else if (meshArg == "3") { // Big debug tetmesh
        tet_mesh = TetMeshFactory::create_big_debug_tetmesh();
    } else if (meshArg == "4") { // Collapsed tetmesh
        tet_mesh = TetMeshFactory::create_collapsed_tetmesh();
    } else { // Default case (tet mesh #1)
        IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/sphere.obj");
        tet_mesh = TetMeshFactory::from_indexed_face_set(*mesh);
        delete mesh;
    }

    printf("Evolving tet mesh...\n");
    for (unsigned int i = 0; i < tet_mesh->vertices.size() / 3; i++) {
        if (tet_mesh->get_vertex_status(i) == INTERFACE) {
            // scale and translate x
            tet_mesh->vertex_targets[i * 3] = tet_mesh->vertices[i * 3] * 1.2;
        }
    }
    tet_mesh->evolve();

    printf("Displaying tet mesh...\n");

    printf("Initializing display...\n");
    Shader * shader = Shader::compile_from("shaders/dsc.vsh", "shaders/dsc.gsh", "shaders/dsc.fsh");
    glUseProgram(shader->get_id());
    glBindVertexArray(vao);
    Renderable renderable(shader, true, false, GL_TRIANGLES);
    tet_mesh->bind_attributes(renderable);
    check_gl_error();
    delete tet_mesh;

    tgui::Gui gui(window);
    gui.setGlobalFont("assets/fonts/DejaVuSans.ttf");
    TetrahedralViewer viewer(&renderable, &gui);
    viewer.init(WINDOW_WIDTH, WINDOW_HEIGHT, FOV);

    printf("Starting display...\n");
    sf::Event event;
    sf::Clock clock;
    tgui::Callback callback;
    while (window.isOpen()) {
        // float frame_length = clock.restart().asSeconds();

        glUseProgram(shader->get_id());
        glBindVertexArray(vao);
        while (gui.pollCallback(callback)) {
            viewer.handle_callback(callback);
        }

        viewer.update();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderable.render();
        check_gl_error();

        glUseProgram(0);
        glBindVertexArray(0);
        gui.draw();
        window.display();

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                printf("Closing window...\n");
                window.close();
            }
            viewer.handle_event(event);
        }

        sf::sleep(sf::seconds(1.0f / FRAME_RATE));
    }

    printf("Cleaning up...\n");
    delete shader;

    return 0;
}
