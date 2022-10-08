/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "components/desktop/background.hpp"

void background_t::load(const char* path)
{
	if(surface)
		cairo_surface_destroy(surface);

	surface = cairo_image_surface_create_from_png(path);
}

void background_t::paint()
{
	cairo_t* cr = graphics.getContext();
	auto bounds = getBounds();

	// Background pattern
	cairo_pattern_t* gradient = cairo_pattern_create_linear(bounds.width * 0.4, 0, bounds.width * 0.8, bounds.height);
	cairo_pattern_add_color_stop_rgb(gradient, 0.0, 105.0 / 255.0, 84.0 / 255.0, 161.0 / 255.0);
	cairo_pattern_add_color_stop_rgb(gradient, 1.0, 22.0 / 255.0, 50.0 / 255.0, 100.0 / 255.0);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_set_source(cr, gradient);
	cairo_fill(cr);
	cairo_pattern_destroy(gradient);

	// Background image
	if(surface)
	{
		int imgwidth = cairo_image_surface_get_width(surface);
		int imgheight = cairo_image_surface_get_height(surface);

		int bgx = bounds.x + (bounds.width / 2 - imgwidth / 2);
		int bgy = bounds.y + (bounds.height / 2 - imgheight / 2);
		cairo_set_source_surface(cr, surface, bgx, bgy);
		cairo_rectangle(cr, bgx, bgy, bounds.width, bounds.height);
		cairo_fill(cr);
	}

	// Selection
	if(selection.width != 0 || selection.height != 0)
	{
		cairo_set_source_rgba(cr, 1, 1, 1, 1);

		cairo_set_line_width(cr, 0.5);
		static const double dash[] = {2.0};
		cairo_set_dash(cr, dash, 1, 0);

		cairo_rectangle(cr, selection.x, selection.y, selection.width, selection.height);
		cairo_stroke(cr);
	}
}

void background_t::showSelection(g_rectangle& selection)
{
	this->selection = selection;
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

void background_t::hideSelection()
{
	this->selection.width = 0;
	this->selection.height = 0;
	markFor(COMPONENT_REQUIREMENT_PAINT);
}