/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <components/checkbox.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <events/mouse_event.hpp>

/**
 *
 */
checkbox_t::checkbox_t() :
		component_t(true), checked(false), boxSize(DEFAULT_BOX_SIZE), boxTextGap(DEFAULT_BOX_TEXT_GAP), hovered(false), pressed(false) {
	addChild(&label);
}

/**
 *
 */
void checkbox_t::layout() {
	g_dimension preferredSize = label.getPreferredSize();
	if (preferredSize.height < boxSize + boxTextGap) {
		preferredSize.height = boxSize + boxTextGap;
	}
	preferredSize.width += preferredSize.height;
	setPreferredSize(preferredSize);
}

/**
 *
 */
void checkbox_t::paint() {
	g_rectangle bounds = getBounds();
	g_painter p(graphics);
	p.setColor(pressed ? RGB(240, 240, 240) : (hovered ? RGB(245, 245, 255) : RGB(255, 255, 255)));
	p.fill(g_rectangle(0, 0, boxSize, boxSize));
	p.setColor((hovered || pressed) ? RGB(140, 140, 150) : RGB(160, 160, 170));
	p.draw(g_rectangle(0, 0, boxSize - 1, boxSize - 1));

	if (checked) {
		p.setColor(RGB(70, 180, 255));
		p.fill(g_rectangle(4, 4, boxSize - 8, boxSize - 8));
	}
}

/**
 *
 */
bool checkbox_t::handle(event_t& e) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			hovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_LEAVE) {
			hovered = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_PRESS) {
			pressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_RELEASE || me->type == MOUSE_EVENT_DRAG_RELEASE) {
			pressed = false;

			g_rectangle minbounds = getBounds();
			minbounds.x = 0;
			minbounds.y = 0;
			klog("clicked at: %i, %i IN %i %i %i %i", me->position.x, me->position.y, minbounds.x, minbounds.y, minbounds.width, minbounds.height);
			if (minbounds.contains(me->position)) {
				klog("clicked in bounds!");
				checked = !checked;
			}

			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		return true;
	}

	return false;
}

/**
 *
 */
void checkbox_t::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle unpositioned = getBounds();
	unpositioned.x = boxSize + boxTextGap;
	unpositioned.y = 0;
	this->label.setBounds(unpositioned);
}
