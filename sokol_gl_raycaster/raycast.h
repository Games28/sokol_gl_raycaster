#pragma once
#ifndef RAYCAST_H
#define RAYCAST_H

#define SOKOL_IMPL
#ifdef __EMSCRIPTEN__
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE
#endif
#include "sokol/include/sokol_app.h"
#include "sokol/include/sokol_gfx.h"
#include "sokol/include/sokol_glue.h"

#include "sokol/sokol_engine.h"

#include "sokol/render_utils.h"

struct Raycast : public cmn::SokolEngine {
	float cell_sz = 20;
	int width = 0, height = 0;
	//flatten 2d index
	int ix(int i, int j) { return i + width * j; }

	bool* grid = nullptr;

	void setupSGL() {
		sgl_desc_t sgl_desc{};
		sgl_setup(sgl_desc);
	}

	bool user_create() override {
		app_title = "Raycaster base";

		setupSGL();

		return true;
	}

	void updateSizing() {
		//determine number of cells
		int new_width = 1 + sapp_widthf() / cell_sz;
		int new_height = 1 + sapp_heightf() / cell_sz;
		//avoid reallocation if no change
		if (new_width == width && new_height == height) return;

		//new allocation
		bool* new_grid = new bool[new_width * new_height];

		//copy overlapping region
		for (int i = 0; i < width && i < new_width; i++) {
			for (int j = 0; j < height && j < new_height; i++) {
				int new_k = i + new_width * j;
				new_grid[new_k] = grid[ix(i, j)];
			}
		}

		//free old
		delete[] grid;

		//transfer markers
		width = new_width;
		height = new_height;
		grid = new_grid;
	}

	bool user_update(float dt) override {
		//convert mouse coords to grid index
		int mi = GetMouseX() / cell_sz;
		int mj = GetMouseY() / cell_sz;
		if (mi >= 0 && mj >= 0 && mi < width && mj < height) {
			if (GetMouse(SAPP_MOUSEBUTTON_LEFT).held) grid[ix(mi, mj)] = true;
			if (GetMouse(SAPP_MOUSEBUTTON_RIGHT).held) grid[ix(mi, mj)] = false;
		}

		updateSizing();

		return true;
	}

	bool user_render() override {
		sg_pass pass{};
		pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
		pass.action.colors[0].clear_value = { 0, 0, 0, 1 };
		pass.swapchain = sglue_swapchain();
		sg_begin_pass(pass);

		sgl_defaults();

		sgl_matrix_mode_projection();
		sgl_ortho(0, sapp_widthf(), sapp_heightf(), 0, -1, 1);

		for (int i = 0; i < width; i++) {
			float x = cell_sz * i;
			cmn::draw_line(x, 0, x, sapp_heightf(), { .5f, .5f, .5f});
		}

		for (int j = 0; j < height; j++) {
			float y = cell_sz * j;
			cmn::draw_line(0, y, sapp_widthf(), y, { .5f, .5f, .5f});
		}

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				//skip "air" cells
				if (!grid[ix(i, j)]) continue;

				float x = cell_sz * i;
				float y = cell_sz * j;
				cmn::fill_rect(
					x, y, cell_sz, cell_sz,
					{ 1, 1, 1}
				);
			}
		}

		sgl_draw();

		sg_end_pass();

		sg_commit();

		return true;
	}
};

#endif // !RAYCAST_H

