/*
 * Tibia
 *
 * Copyright (C) 2024 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo
 */

typedef struct {
	void *			widget;
} plugin_ui;

static void plugin_ui_get_default_size(uint32_t *width, uint32_t *height) {
	*width = 0;
	*height = 0;
}

static plugin_ui *plugin_ui_create(char has_parent, void *parent, plugin_ui_callbacks *cbs) {
	return NULL;
}

static void plugin_ui_free(plugin_ui *instance) {
}

static void plugin_ui_idle(plugin_ui *instance) {
}

static void plugin_ui_set_parameter(plugin_ui *instance, size_t index, float value) {
}
