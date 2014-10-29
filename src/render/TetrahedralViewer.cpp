
#include "TetrahedralViewer.h"

TetrahedralViewer::TetrahedralViewer(Renderable * r, sfg::Desktop * d) {
    renderable = r;
    desktop = d;
    per_vertex_coloring = true;
    display_outside = true;
    display_inside = true;
    display_interface = true;
}

void TetrahedralViewer::init() {
    renderable->bind_uniform(&per_vertex_coloring, SCALAR_INT, 1, "per_vertex_coloring");
    renderable->bind_uniform(&display_outside, SCALAR_INT, 1, "display_outside");
    renderable->bind_uniform(&display_inside, SCALAR_INT, 1, "display_inside");
    renderable->bind_uniform(&display_interface, SCALAR_INT, 1, "display_interface");

    glBindVertexArray(0);
    auto color_method_button = sfg::Button::Create("Color Method");
    color_method_button->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&TetrahedralViewer::color_method_button_on_click, this));

    auto outside_toggle = sfg::Button::Create("Outside");
    outside_toggle->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&TetrahedralViewer::display_outside_on_click, this));
    outside_toggle->SetId("outside_toggle");
    desktop->SetProperty("Button#outside_toggle", "BackgroundColor", "#00ff00ff");

    auto inside_toggle = sfg::Button::Create("Inside");
    inside_toggle->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&TetrahedralViewer::display_inside_on_click, this));
    inside_toggle->SetId("inside_toggle");
    desktop->SetProperty("Button#inside_toggle", "BackgroundColor", "#ff0000ff");

    auto interface_toggle = sfg::Button::Create("Interface");
    interface_toggle->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&TetrahedralViewer::display_interface_on_click, this));
    interface_toggle->SetId("interface_toggle");
    desktop->SetProperty("Button#interface_toggle", "BackgroundColor", "#0000ffff");

    auto box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5.0f);
    box->Pack(color_method_button);
    box->Pack(outside_toggle);
    box->Pack(inside_toggle);
    box->Pack(interface_toggle);

    auto gui_window = sfg::Window::Create();
    gui_window->SetTitle("Display Toggles");
    gui_window->Add(box);

    desktop->Add(gui_window);
}

void TetrahedralViewer::color_method_button_on_click() {
    per_vertex_coloring = !per_vertex_coloring;
    renderable->bind_uniform(&per_vertex_coloring, SCALAR_INT, 1, "per_vertex_coloring");
}

void TetrahedralViewer::display_outside_on_click() {
    display_outside = !display_outside;
    renderable->bind_uniform(&display_outside, SCALAR_INT, 1, "display_outside");
}

void TetrahedralViewer::display_inside_on_click() {
    display_inside = !display_inside;
    renderable->bind_uniform(&display_inside, SCALAR_INT, 1, "display_inside");
}

void TetrahedralViewer::display_interface_on_click() {
    display_interface = !display_interface;
    renderable->bind_uniform(&display_interface, SCALAR_INT, 1, "display_interface");
}
