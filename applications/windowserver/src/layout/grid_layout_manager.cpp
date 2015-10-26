#include <components/component.hpp>
#include <vector>

#include <ghostuser/utils/Logger.hpp>
#include <layout/grid_layout_manager.hpp>

#include <typeinfo>

/**
 *
 */
grid_layout_manager_t::grid_layout_manager_t(int columns, int rows) :
		columns(columns), rows(rows) {
}

/**
 *
 */
void grid_layout_manager_t::layout() {

	if (component == 0) {
		return;
	}

	std::vector<component_t*>& children = component->getChildren();

	int x = 0;
	int y = 0;
	int lineHeight = 0;

	g_rectangle parentBounds = component->getBounds();

	int widthPerComponent = (columns > 0) ? (parentBounds.width / columns) : parentBounds.width;

	for (component_t* c : children) {

		int usedHeight = (rows > 0) ? (parentBounds.height / rows) : c->getPreferredSize().height;

		if (x + widthPerComponent > parentBounds.width) {
			x = 0;
			y += lineHeight;
			lineHeight = 0;
		}

		c->setBounds(g_rectangle(x, y, widthPerComponent, usedHeight));
		x += widthPerComponent;

		if (usedHeight > lineHeight) {
			lineHeight = usedHeight;
		}

	}
}
