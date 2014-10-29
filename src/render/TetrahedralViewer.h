
#ifndef _TETRAHEDRAL_VIEWER_H
#define _TETRAHEDRAL_VIEWER_H

#include "Renderable.h"
#include <SFGUI/SFGUI.hpp>

class TetrahedralViewer {
    public:
        TetrahedralViewer(Renderable * renderable, sfg::Desktop * desktop);
        void init();
        void color_method_button_on_click();
        void display_outside_on_click();
        void display_inside_on_click();
        void display_interface_on_click();
    private:
        Renderable * renderable;
        sfg::Desktop * desktop;
        int per_vertex_coloring;
        int display_outside;
        int display_inside;
        int display_interface;
};

#endif
