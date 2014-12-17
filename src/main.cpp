
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <tetgen.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

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
#define PI 3.14159f

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
     * 5: Sphere, rotated
     * 6: C-mesh, joining together
     * 7: Sphere, stretched in the x-direction
     */
    int meshArg = 1;
    if (argc >= 2) { meshArg = atoi(argv[1]); }

    if (meshArg == 2) { // Debug tetmesh
        tet_mesh = TetMeshFactory::create_debug_tetmesh();
        printf("Evolving tet mesh ...\n");
        tet_mesh->evolve();
    } else if (meshArg == 3) { // Big debug tetmesh
        tet_mesh = TetMeshFactory::create_big_debug_tetmesh();
        printf("Evolving tet mesh ...\n");
        tet_mesh->evolve();
    } else if (meshArg == 4) { // Collapsed tetmesh
        tet_mesh = TetMeshFactory::create_collapsed_tetmesh();
        printf("Evolving tet mesh ...\n");
        tet_mesh->evolve();
    } else if (meshArg == 5) { // Rotated sphere tetmesh
        IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/sphere.obj");
        tet_mesh = TetMeshFactory::from_indexed_face_set(*mesh);
        delete mesh;


        for (int deg = 1; deg <= 70; deg++) {
            REAL angle = PI / 180;
            for (unsigned int i = 0; i < tet_mesh->vertices.size() / 3; i++) {
                if (tet_mesh->get_vertex_status(i) == INTERFACE) {
                    glm::detail::tvec4<REAL, glm::precision::defaultp> v(tet_mesh->vertices[i*3], tet_mesh->vertices[i*3+1], tet_mesh->vertices[i*3+2], 1);
                    glm::detail::tvec4<REAL, glm::precision::defaultp> v2 = glm::rotateX(v, angle);
                    tet_mesh->vertex_targets[i*3] = v2[0];
                    tet_mesh->vertex_targets[i*3+1] = v2[1];
                    tet_mesh->vertex_targets[i*3+2] = v2[2];
                    tet_mesh->vertex_statuses[i] = MOVING;
                }
            }
            printf("Evolving tet mesh (%d deg)...\n", deg);
            tet_mesh->evolve();
        }
    } else if (meshArg == 6) { // C-mesh
        IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/c_mesh.obj");
        tet_mesh = TetMeshFactory::from_indexed_face_set(*mesh);
        delete mesh;

        for (unsigned int i = 0; i < tet_mesh->vertices.size() / 3; i++) {
			if (tet_mesh->get_vertex_status(i) == INTERFACE) 
			{
		        glm::detail::tvec4<REAL, glm::precision::defaultp> v(tet_mesh->vertices[i*3], tet_mesh->vertices[i*3+1], tet_mesh->vertices[i*3+2], 1);
                if (tet_mesh->vertices[i*3] == 0.64f && tet_mesh->vertices[i*3+1] == 0.10f)
				{
					tet_mesh->vertex_targets[i*3] = tet_mesh->vertices[i*3];		// Stays the same
					tet_mesh->vertex_targets[i*3+1] = -0.10f;	// Moves to create C Mesh case
					tet_mesh->vertex_targets[i*3+2] = tet_mesh->vertices[i*3+2];	// Stays the same
				}
				else
				{
					tet_mesh->vertex_targets[i*3] = tet_mesh->vertices[i*3];
					tet_mesh->vertex_targets[i*3+1] = tet_mesh->vertices[i*3+1];
					tet_mesh->vertex_targets[i*3+2] = tet_mesh->vertices[i*3+2];
				}
				tet_mesh->vertex_statuses[i] = MOVING;
            }
		}
        printf("Evolving tet mesh from C mesh...\n");
        tet_mesh->evolve();
		
    }  else if (meshArg == 7) { // Stretched sphere
        IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/sphere.obj");
        tet_mesh = TetMeshFactory::from_indexed_face_set(*mesh);
        for (unsigned int i = 0; i < tet_mesh->vertices.size() / 3; i++) {
            if (tet_mesh->get_vertex_status(i) == INTERFACE) {
                // scale x
                tet_mesh->vertex_targets[i * 3] = tet_mesh->vertices[i * 3] * 1.2;
                tet_mesh->vertex_targets[i * 3 + 1] = tet_mesh->vertices[i * 3 + 1];
                tet_mesh->vertex_targets[i * 3 + 2] = tet_mesh->vertices[i * 3 + 2];
                tet_mesh->vertex_statuses[i] = MOVING;
            }
        }
        printf("Evolving tet mesh ...\n");
        tet_mesh->evolve();
    } else { // Default case (tet mesh #1)
        IndexedFaceSet * mesh = IndexedFaceSet::load_from_obj("assets/models/sphere.obj");
        tet_mesh = TetMeshFactory::from_indexed_face_set(*mesh);
        printf("Evolving tet mesh ...\n");
        tet_mesh->evolve();
        delete mesh;
    }

    printf("Displaying tet mesh...\n");

    printf("Initializing display...\n");
    Shader * shader = Shader::compile_from("shaders/dsc.vsh", "shaders/dsc.gsh", "shaders/dsc.fsh");
    glUseProgram(shader->get_id());
    glBindVertexArray(vao);
    Renderable renderable(shader, true, false, GL_TRIANGLES);

    tgui::Gui gui(window);
    gui.setGlobalFont("assets/fonts/DejaVuSans.ttf");
    TetrahedralViewer viewer(&renderable, &gui);
    viewer.init(WINDOW_WIDTH, WINDOW_HEIGHT, FOV);
    viewer.bind_attributes(*tet_mesh, renderable);
    check_gl_error();
    delete tet_mesh;

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
