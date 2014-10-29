
#include "TetrahedralViewer.h"

#define PI 3.14159f

enum callback_ids {
    PER_VERTEX_COLORING,
    PER_FACE_COLORING,
    OUTSIDE_TOGGLE,
    INSIDE_TOGGLE,
    INTERFACE_TOGGLE
};

TetrahedralViewer::TetrahedralViewer(Renderable * r, tgui::Gui * g) {
    renderable = r;
    gui = g;
}

void TetrahedralViewer::init(int window_width, int window_height, float fov) {
    glm::vec2 screen_dimensions = glm::vec2(window_width, window_height);
    renderable->bind_uniform(&screen_dimensions[0], VEC2_FLOAT, 1, "screen_dimensions");
    model_transform = glm::mat4();
    eye = glm::vec3(12, 12, 20);
    focus = glm::vec3(0, 0, 0);
    up = glm::vec3(0, 1, 0);
    view_transform = glm::lookAt(eye, focus, up);
    perspective_transform = glm::perspective(fov, ((float) window_width) / window_height, 0.1f, 100.0f);

    glUseProgram(0);
    glBindVertexArray(0);

    per_vertex = tgui::RadioButton::Ptr(*gui);
    per_vertex->load("assets/widgets/Black.conf");
    per_vertex->setText("Per vertex colring");
    per_vertex->setPosition(20, 100);
    per_vertex->check();
    per_vertex->bindCallback(tgui::Button::LeftMouseClicked);
    per_vertex->setCallbackId(PER_VERTEX_COLORING);

    per_face = tgui::RadioButton::Ptr(*gui);
    per_face->load("assets/widgets/Black.conf");
    per_face->setText("Per face coloring");
    per_face->setPosition(20, 140);
    per_face->bindCallback(tgui::Button::LeftMouseClicked);
    per_face->setCallbackId(PER_FACE_COLORING);

    outside_toggle = tgui::Checkbox::Ptr(*gui);
    outside_toggle->load("assets/widgets/Black.conf");
    outside_toggle->setText("Display outside faces");
    outside_toggle->setPosition(20, 180);
    outside_toggle->check();
    outside_toggle->bindCallback(tgui::Button::LeftMouseClicked);
    outside_toggle->setCallbackId(OUTSIDE_TOGGLE);

    inside_toggle = tgui::Checkbox::Ptr(*gui);
    inside_toggle->load("assets/widgets/Black.conf");
    inside_toggle->setText("Display inside faces");
    inside_toggle->setPosition(20, 220);
    inside_toggle->check();
    inside_toggle->bindCallback(tgui::Button::LeftMouseClicked);
    inside_toggle->setCallbackId(INSIDE_TOGGLE);

    interface_toggle = tgui::Checkbox::Ptr(*gui);
    interface_toggle->load("assets/widgets/Black.conf");
    interface_toggle->setText("Display interface faces");
    interface_toggle->setPosition(20, 260);
    interface_toggle->check();
    interface_toggle->bindCallback(tgui::Button::LeftMouseClicked);
    interface_toggle->setCallbackId(INTERFACE_TOGGLE);
}

void TetrahedralViewer::handle_event(sf::Event & event) {
    gui->handleEvent(event);
}

void TetrahedralViewer::handle_callback(tgui::Callback & callback) {

}

void TetrahedralViewer::update() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        eye = glm::rotate(eye, 5 * PI / 180, up);
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        eye = glm::rotate(eye, -5 * PI / 180, up);
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        eye = 0.9f * eye;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        eye = 1.1f * eye;
    }
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
}

