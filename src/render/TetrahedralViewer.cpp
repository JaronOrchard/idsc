
#include "TetrahedralViewer.h"

#define PI 3.14159f
#define THEME_CONFIG_FILE "assets/widgets/Black.conf"
#define UINT_32_MAX ((unsigned int) 0xffffffff)

TetrahedralViewer::TetrahedralViewer(Renderable * r, tgui::Gui * g) {
    renderable = r;
    gui = g;
}

glm::vec3 TetrahedralViewer::set_eye_vector()
{
	glm::float32 y = radius * glm::sin(theta);
	glm::float32 x = radius * glm::cos(theta) * glm::cos(phi);
	glm::float32 z = radius * glm::cos(theta) * glm::sin(phi);
	return glm::vec3(x, y, z);
}

void TetrahedralViewer::init(int window_width, int window_height, float fov) {
    glm::vec2 screen_dimensions = glm::vec2(window_width, window_height);
    renderable->bind_uniform(&screen_dimensions[0], VEC2_FLOAT, 1, "screen_dimensions");
    model_transform = glm::mat4();
	current_pos = sf::Vector2i(0,0);
	radius = glm::sqrt(static_cast<glm::float32>(500));
	theta = 0.0F;
	phi = 0.0;
	eye = set_eye_vector();
	focus = glm::vec3(0, 0, 0);
    up = glm::vec3(0, 1, 0);
    view_transform = glm::lookAt(eye, focus, up);
    perspective_transform = glm::perspective(fov, ((float) window_width) / window_height, 0.1f, 100.0f);

    glUseProgram(0);
    glBindVertexArray(0);

    per_vertex = tgui::RadioButton::Ptr(*gui);
    per_vertex->load(THEME_CONFIG_FILE);
    per_vertex->setText("Per vertex colring");
    per_vertex->setPosition(20, 100);
    per_vertex->check();

    per_face = tgui::RadioButton::Ptr(*gui);
    per_face->load(THEME_CONFIG_FILE);
    per_face->setText("Per face coloring");
    per_face->setPosition(20, 140);

    outside_toggle = tgui::Checkbox::Ptr(*gui);
    outside_toggle->load(THEME_CONFIG_FILE);
    outside_toggle->setText("Display outside faces");
    outside_toggle->setPosition(20, 180);
    outside_toggle->check();

    inside_toggle = tgui::Checkbox::Ptr(*gui);
    inside_toggle->load(THEME_CONFIG_FILE);
    inside_toggle->setText("Display inside faces");
    inside_toggle->setPosition(20, 220);
    inside_toggle->check();

    interface_toggle = tgui::Checkbox::Ptr(*gui);
    interface_toggle->load(THEME_CONFIG_FILE);
    interface_toggle->setText("Display interface faces");
    interface_toggle->setPosition(20, 260);
    interface_toggle->check();

    error_toggle = tgui::Checkbox::Ptr(*gui);
    error_toggle->load(THEME_CONFIG_FILE);
    error_toggle->setText("Display boundary faces");
    error_toggle->setPosition(20, 300);
    error_toggle->check();

    opacity_slider = tgui::Slider::Ptr(*gui);
    opacity_slider->load(THEME_CONFIG_FILE);
    opacity_slider->setVerticalScroll(false);
    opacity_slider->setPosition(20, 340);
    opacity_slider->setSize(300, 25);
    opacity_slider->setValue(UINT_32_MAX * 0.2);
    opacity_slider->setMaximum(UINT_32_MAX);
}

void TetrahedralViewer::handle_event(sf::Event & event) {
    gui->handleEvent(event);
}

void TetrahedralViewer::handle_callback(tgui::Callback & callback) {

}

void TetrahedralViewer::update() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) 
	{
		phi += 5 * PI / 180;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
		phi -= 5 * PI / 180;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
		radius = .9F * radius;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
		radius = 1.1F * radius;
    }
	
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		sf::Vector2i new_pos = sf::Mouse::getPosition();
		sf::Vector2i diff = new_pos - current_pos;
		phi += PI * diff.x / 180.0F / 2.0F;
		theta += PI * diff.y / 180.0F / 2.0F;

		if (theta > PI/2)
			theta = PI/2;
		else if (theta < -PI/2)
			theta = -PI/2;
	}

	if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
		sf::Vector2i new_pos = sf::Mouse::getPosition();
		sf::Vector2i diff = new_pos - current_pos;
		radius += diff.y/6.0;
		if (radius >= 50)
			radius = 50;
		else if (radius < 1)
			radius = 1;
	}

	current_pos = sf::Mouse::getPosition();
	eye = set_eye_vector();
	view_transform = glm::lookAt(eye, focus, up);
    glm::mat4 MVP = perspective_transform * view_transform * model_transform;
    renderable->bind_uniform(&MVP[0][0], MAT4_FLOAT, 1, "MVP");

    int data;
    data = per_vertex->isChecked();
    renderable->bind_uniform(&data, SCALAR_INT, 1, "per_vertex_coloring");
    data = outside_toggle->isChecked();
    renderable->bind_uniform(&data, SCALAR_INT, 1, "display_outside");
    data = inside_toggle->isChecked();
    renderable->bind_uniform(&data, SCALAR_INT, 1, "display_inside");
    data = interface_toggle->isChecked();
    renderable->bind_uniform(&data, SCALAR_INT, 1, "display_interface");
    data = error_toggle->isChecked();
    renderable->bind_uniform(&data, SCALAR_INT, 1, "display_boundary");

    float opacity = ((float) opacity_slider->getValue()) / UINT_32_MAX;
    renderable->bind_uniform(&opacity, SCALAR_FLOAT, 1, "opacity");
}

void TetrahedralViewer::bind_attributes(TetMesh & tetmesh, Renderable & renderable) {
    // ensure vertices are not double precision
    float * verts = new float[tetmesh.vertices.size()];
    for (unsigned int i = 0; i < tetmesh.vertices.size(); i++) {
        verts[i] = tetmesh.vertices[i];
    }
    renderable.bind_attribute(verts, VEC3_FLOAT, tetmesh.vertices.size() / 3, "vertex_position");
    delete[] verts;

    int * vertex_statuses = new int[tetmesh.vertices.size() / 3];
    for (unsigned int i = 0; i < tetmesh.vertices.size() / 3; i++) {
        vertex_statuses[i] = (int)tetmesh.get_vertex_status(i);
    }
    renderable.bind_attribute(&vertex_statuses[0], SCALAR_INT, tetmesh.vertices.size() / 3, "vertex_status");
    delete[] vertex_statuses;

    unsigned int num_tets = tetmesh.tets.size() / 4;
    unsigned int num_faces = num_tets * 4;
    unsigned int num_indices = num_faces * 3;
    int * indices = new int[num_indices];

    unsigned int buffer_i = 0;
    for (unsigned int i = 0; i < num_tets; i++) {
        if (tetmesh.tet_gravestones[i] != DEAD) {
            for (unsigned int j = 0; j < 4; j++) {
                Face f = tetmesh.get_opposite_face(i, tetmesh.tets[i * 4 + j]);
                indices[buffer_i * 12 + j * 3] = f.getV1();
                indices[buffer_i * 12 + j * 3 + 1] = f.getV2();
                indices[buffer_i * 12 + j * 3 + 2] = f.getV3();
            }
            buffer_i++;
        }
    }

    renderable.bind_indices(indices, buffer_i * 12 * sizeof(int));
    delete[] indices;
}


