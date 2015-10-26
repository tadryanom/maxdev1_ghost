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

#include <components/cursor.hpp>
#include <events/event_processor.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <input/input_receiver.hpp>
#include <interface/component_registry.hpp>

#include <windowserver.hpp>
#include <components/window.hpp>
#include <components/button.hpp>
#include <components/text/text_field.hpp>
#include <components/label.hpp>

#include <events/event.hpp>
#include <events/mouse_event.hpp>
#include <events/key_event.hpp>
#include <events/focus_event.hpp>

/**
 *
 */
event_processor_t::event_processor_t() {
	multiclickTimespan = DEFAULT_MULTICLICK_TIMESPAN;
}

/**
 *
 */
void event_processor_t::bufferKeyEvent(g_key_info keyInfo) {
	key_info_buffer.push_back(keyInfo);
}

/**
 *
 */
void event_processor_t::bufferCommandMessage(void* commandMessage) {
	command_message_buffer.push_back(commandMessage);
}

/**
 *
 */
void event_processor_t::process() {

	// process key events
	while (key_info_buffer.size() > 0) {
		translateKeyEvent(key_info_buffer.back());
		key_info_buffer.pop_back();
	}

	// process command messages
	while (command_message_buffer.size() > 0) {

		// take next message from buffer
		void* request_buffer = command_message_buffer.back();
		command_message_buffer.pop_back();

		g_message_header* message = (g_message_header*) request_buffer;

		// prepare response
		command_message_response_t buf_response;
		buf_response.target = message->sender;
		buf_response.transaction = message->transaction;
		buf_response.message = 0;

		// process the actual action
		process_command((g_ui_message_header*) G_MESSAGE_CONTENT(request_buffer), buf_response);

		// add generated response to queue
		if (buf_response.message != 0) {
			windowserver_t::instance()->responder_thread->send_response(buf_response);
		}

		// delete request buffer
		delete (g_message_header*) request_buffer;
	}
}

/**
 *
 */
void event_processor_t::process_command(g_ui_message_header* request_header, command_message_response_t& response_out) {

	if (request_header->id == G_UI_PROTOCOL_CREATE_COMPONENT) {
		component_t* component = 0;
		g_ui_component_id component_id = -1;

		// decide what to create
		g_ui_create_component_request* create_request = (g_ui_create_component_request*) request_header;

		switch (create_request->type) {
		case G_UI_COMPONENT_TYPE_WINDOW:
			component = new window_t();
			windowserver_t::instance()->screen->addChild(component);
			break;
		case G_UI_COMPONENT_TYPE_LABEL:
			component = new label_t();
			break;
		case G_UI_COMPONENT_TYPE_BUTTON:
			component = new button_t();
			break;
		case G_UI_COMPONENT_TYPE_TEXTFIELD:
			component = new text_field_t();
			break;

		default:
			klog("don't know how to create a component of type %i", create_request->type);
			break;
		}

		// register the component
		if (component != 0) {
			component_id = component_registry_t::add(component);
			// apply default properties
			component->setBounds(g_rectangle(100, 100, 200, 80));
		}

		// create response message
		g_ui_create_component_response* response = new g_ui_create_component_response;
		response->header.id = G_UI_PROTOCOL_CREATE_COMPONENT;
		response->id = component_id;
		response->status = (component != 0 ? G_UI_PROTOCOL_SUCCESS : G_UI_PROTOCOL_FAIL);

		response_out.message = response;
		response_out.length = sizeof(g_ui_create_component_response);

	} else if (request_header->id == G_UI_PROTOCOL_ADD_COMPONENT) {
		g_ui_component_add_child_request* request = (g_ui_component_add_child_request*) request_header;
		component_t* parent = component_registry_t::get(request->parent);
		component_t* child = component_registry_t::get(request->child);

		// create response message
		g_ui_component_add_child_response* response = new g_ui_component_add_child_response;
		if (parent == 0 || child == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
			klog("could not add %i (%x) to %i (%x)", request->child, child, request->parent, parent);
		} else {
			parent->addChild(child);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_add_child_response);

	} else if (request_header->id == G_UI_PROTOCOL_SET_BOUNDS) {
		g_ui_component_set_bounds_request* request = (g_ui_component_set_bounds_request*) request_header;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_bounds_response* response = new g_ui_component_set_bounds_response;
		if (component == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
		} else {
			component->setBounds(request->bounds);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_set_bounds_response);

	} else if (request_header->id == G_UI_PROTOCOL_SET_VISIBLE) {
		g_ui_component_set_visible_request* request = (g_ui_component_set_visible_request*) request_header;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_visible_response* response = new g_ui_component_set_visible_response;
		if (component == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
		} else {
			component->setVisible(request->visible);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_set_visible_response);

	} else if (request_header->id == G_UI_PROTOCOL_SET_ACTION_LISTENER) {
		g_ui_component_set_action_handler_request* request = (g_ui_component_set_action_handler_request*) request_header;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_action_handler_response* response = new g_ui_component_set_action_handler_response;
		if (component == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
		} else {
			action_component_t* action_component = dynamic_cast<action_component_t*>(component);
			if (action_component == 0) {
				response->status = G_UI_PROTOCOL_FAIL;
			} else {
				action_component->setActionListener(request->target_thread, request->id);
				klog("task %i registered as action listener for component %i!", request->target_thread, request->id);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_set_action_handler_response);

	} else if (request_header->id == G_UI_PROTOCOL_SET_TITLE) {
		g_ui_component_set_title_request* request = (g_ui_component_set_title_request*) request_header;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_title_response* response = new g_ui_component_set_title_response;
		if (component == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
		} else {
			titled_component_t* titled_component = dynamic_cast<titled_component_t*>(component);
			if (titled_component == 0) {
				response->status = G_UI_PROTOCOL_FAIL;
			} else {
				titled_component->setTitle(request->title);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_set_title_response);

	} else if (request_header->id == G_UI_PROTOCOL_GET_TITLE) {
		g_ui_component_get_title_request* request = (g_ui_component_get_title_request*) request_header;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_get_title_response* response = new g_ui_component_get_title_response;
		if (component == 0) {
			response->status = G_UI_PROTOCOL_FAIL;
		} else {
			titled_component_t* titled_component = dynamic_cast<titled_component_t*>(component);
			if (titled_component == 0) {
				response->status = G_UI_PROTOCOL_FAIL;
			} else {
				std::string title = titled_component->getTitle();

				// fill text (truncate if necessary)
				const char* title_str = title.c_str();
				size_t title_len;
				if (title.length() >= G_UI_COMPONENT_TITLE_MAXIMUM) {
					title_len = G_UI_COMPONENT_TITLE_MAXIMUM;
				} else {
					title_len = title.length();
				}
				memcpy(response->title, title.c_str(), title_len);
				response->title[title_len] = 0;

				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		response_out.message = response;
		response_out.length = sizeof(g_ui_component_get_title_response);

	}

}

/**
 *
 */
void event_processor_t::translateKeyEvent(g_key_info& info) {

	if (cursor_t::focusedComponent) {
		// process
		key_event_t k;
		k.info = info;
		windowserver_t::instance()->dispatch(cursor_t::focusedComponent, k);
	}
}

/**
 *
 */
void event_processor_t::processMouseState() {

	g_point previousPosition = cursor_t::position;
	mouse_button_t previousPressedButtons = cursor_t::pressedButtons;

	g_dimension resolution = windowserver_t::instance()->video_output->getResolution();
	windowserver_t* instance = windowserver_t::instance();
	screen_t* screen = instance->screen;

	// invalidate old location
	screen->markDirty(cursor_t::getArea());

	// set new cursor position
	cursor_t::position.x = cursor_t::nextPosition.x;
	cursor_t::position.y = cursor_t::nextPosition.y;

	// Invalidate new location
	screen->markDirty(cursor_t::getArea());

	// set pressed buttons
	cursor_t::pressedButtons = cursor_t::nextPressedButtons;

	mouse_event_t baseEvent;
	baseEvent.screenPosition = cursor_t::position;
	baseEvent.position = baseEvent.screenPosition;
	baseEvent.buttons = cursor_t::pressedButtons;

	// Press
	if ((!(previousPressedButtons & MOUSE_BUTTON_1) && (cursor_t::pressedButtons & MOUSE_BUTTON_1))
			|| (!(previousPressedButtons & MOUSE_BUTTON_2) && (cursor_t::pressedButtons & MOUSE_BUTTON_2))
			|| (!(previousPressedButtons & MOUSE_BUTTON_3) && (cursor_t::pressedButtons & MOUSE_BUTTON_3))) {

		// Prepare event
		mouse_event_t pressEvent = baseEvent;
		pressEvent.type = MOUSE_EVENT_PRESS;

		// Multiclicks
		static uint64_t lastClick = 0;
		static int clickCount = 0;
		uint64_t currentClick = g_millis();
		uint64_t diff = currentClick - lastClick;
		if (diff < multiclickTimespan) {
			++clickCount;
		} else {
			clickCount = 1;
		}
		lastClick = currentClick;
		pressEvent.clickCount = clickCount;

		// Send event
		instance->dispatch(screen, pressEvent);

		component_t* hitComponent = screen->getComponentAt(cursor_t::position);
		if (hitComponent != 0) {

			// Prepare drag
			if (hitComponent != screen) {
				cursor_t::draggedComponent = hitComponent;
			}

			// Switch focus
			if (hitComponent != cursor_t::focusedComponent) {
				// Old loses focus
				if (cursor_t::focusedComponent != 0) {
					focus_event_t focusLostEvent;
					focusLostEvent.type = FOCUS_EVENT_LOST;
					instance->dispatchUpwards(cursor_t::focusedComponent, focusLostEvent);
				}

				// Bring hit components window to front
				window_t* parentWindow = hitComponent->getWindow();
				if (parentWindow != 0) {
					parentWindow->bringToFront();
				}

				// New gains focus
				focus_event_t focusGainedEvent;
				focusGainedEvent.type = FOCUS_EVENT_GAINED;
				cursor_t::focusedComponent = instance->dispatchUpwards(hitComponent, focusGainedEvent);
			}
		}

		// Release
	} else if (((previousPressedButtons & MOUSE_BUTTON_1) && !(cursor_t::pressedButtons & MOUSE_BUTTON_1))
			|| ((previousPressedButtons & MOUSE_BUTTON_2) && !(cursor_t::pressedButtons & MOUSE_BUTTON_2))
			|| ((previousPressedButtons & MOUSE_BUTTON_3) && !(cursor_t::pressedButtons & MOUSE_BUTTON_3))) {

		if (cursor_t::draggedComponent) {
			mouse_event_t releaseDraggedEvent = baseEvent;
			releaseDraggedEvent.type = MOUSE_EVENT_DRAG_RELEASE;
			instance->dispatchUpwards(cursor_t::draggedComponent, releaseDraggedEvent);
			cursor_t::draggedComponent = 0;
		}

		mouse_event_t releaseEvent = baseEvent;
		releaseEvent.type = MOUSE_EVENT_RELEASE;
		instance->dispatch(screen, releaseEvent);

		// Move or drag
	} else if (cursor_t::position != previousPosition) {

		component_t* hovered = screen->getComponentAt(cursor_t::position);
		if (hovered != 0 && (hovered != cursor_t::hoveredComponent)) {

			// Leave
			if (cursor_t::hoveredComponent) {
				mouse_event_t leaveEvent = baseEvent;
				leaveEvent.type = MOUSE_EVENT_LEAVE;
				instance->dispatchUpwards(cursor_t::hoveredComponent, leaveEvent);
				cursor_t::hoveredComponent = 0;
			}

			// Enter
			mouse_event_t enterEvent = baseEvent;
			enterEvent.type = MOUSE_EVENT_ENTER;
			cursor_t::hoveredComponent = hovered;
			instance->dispatchUpwards(cursor_t::hoveredComponent, enterEvent);
		}

		if (cursor_t::draggedComponent != 0) { // Dragging
			mouse_event_t dragEvent = baseEvent;
			dragEvent.type = MOUSE_EVENT_DRAG;
			instance->dispatchUpwards(cursor_t::draggedComponent, dragEvent);

		} else { // Moving
			mouse_event_t moveEvent = baseEvent;
			moveEvent.type = MOUSE_EVENT_MOVE;
			instance->dispatch(screen, moveEvent);
		}
	}
}
