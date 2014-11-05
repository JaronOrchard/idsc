
#ifndef _TETRAHEDRAL_VIEWER_H
#define _TETRAHEDRAL_VIEWER_H

#include "TetrahedralViewer.h"
#include "Renderable.h"
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

class TetrahedralViewer {
    public:
        TetrahedralViewer(Renderable * renderable, tgui::Gui * gui);
        void init(int window_width, int window_height, float fov);
        void update();
        void handle_event(sf::Event & event);
        void handle_callback(tgui::Callback & callback);
    private:
        Renderable * renderable;
        tgui::Gui * gui;

        tgui::RadioButton::Ptr per_vertex;
        tgui::RadioButton::Ptr per_face;
        tgui::Checkbox::Ptr outside_toggle;
        tgui::Checkbox::Ptr inside_toggle;
        tgui::Checkbox::Ptr interface_toggle;
        tgui::Slider::Ptr opacity_slider;

        glm::mat4 model_transform;
        glm::vec3 eye;
        glm::vec3 focus;
        glm::vec3 up;
        glm::mat4 view_transform;
        glm::mat4 perspective_transform;

};

#endif
