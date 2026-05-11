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
#include "cmn/math/v2d.h"

#include "sokol/sokol_engine.h"

#include "sokol/render_utils.h"
#include "cmn/utils.h"

struct Raycast : public cmn::SokolEngine {
	const float Pi = 3.14159f;
	float cell_size = 20;
	int width = 0, height = 0;
	cmn::vf2d player_pos, player_dir;
	float player_rot;
	const float player_rad = 6;
	float player_fov = 60 * Pi / 180;

	cmn::vf2d light_pos;
	bool hover_light = true;
	bool show_grid = false;
	//flatten 2d index
	int ix(int i, int j) { return i + width * j; }

	bool* grid = nullptr;

	//grid helpers
	int ix(int i, int j) const {
		return i + width * j;
	}
	bool inRangeX(int i) const {
		return i >= 0 && i < width;
	}
	bool inRangeY(int j) const {
		return j >= 0 && j < height;
	}

	void setupSGL() {
		sgl_desc_t sgl_desc{};
		sgl_setup(sgl_desc);
	}

	bool user_create() override {
		app_title = "Raycaster base";

		player_pos.x = sapp_widthf() / 2;
		player_pos.y = sapp_heightf() / 2;

		light_pos.x = sapp_widthf() / 2;
		light_pos.y = sapp_heightf() / 2;


		setupSGL();

		return true;
	}

	void handleCollision(cmn::vf2d& pos, float rad)
	{
		int pi = pos.x / cell_size;
		int pj = pos.y / cell_size;
		if (inRangeX(pi) && inRangeY(pj) && grid[ix(pi, pj)]) return;

		//given player size, determine min area to check
		int si = (pos.x - rad) / cell_size;
		int ei = (pos.x + rad) / cell_size;
		int sj = (pos.y - rad) / cell_size;
		int ej = (pos.y + rad) / cell_size;

		//for each block
		for (int i = si; i <= ei; i++) {
			if (!inRangeX(i)) continue;

			for (int j = sj; j <= ej; j++) {
				if (!inRangeY(j)) continue;

				//only check solid
				if (!grid[ix(i, j)]) continue;

				//find close edge point
				cmn::vf2d close_pt(
					cmn::clamp(pos.x, cell_size * i, cell_size * (i + 1)),
					cmn::clamp(pos.y, cell_size * j, cell_size * (j + 1))
				);

				//is it too close?
				cmn::vf2d sub = pos - close_pt;
				float mag = sub.mag();
				if (mag < rad) {
					//resolve
					cmn::vf2d norm = sub / mag;
					pos += (rad - mag) * norm;
				}
			}
		}
		
	}

	void updateSizing() {
		//determine number of cells
		int new_width = 1 + sapp_widthf() / cell_size;
		int new_height = 1 + sapp_heightf() / cell_size;
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

	void handleUserInput(float dt)
	{
		
		if (GetKey(SAPP_KEYCODE_W).held) player_pos += 50 * dt * player_dir;
		if (GetKey(SAPP_KEYCODE_S).held) player_pos -= 50 * dt * player_dir;
		if (GetKey(SAPP_KEYCODE_A).held) player_rot -= 3 * dt;
		if (GetKey(SAPP_KEYCODE_D).held) player_rot += 3 * dt;
		

		int mi = GetMouseX() / cell_size;
		int mj = GetMouseY() / cell_size;
		if (mi >= 0 && mj >= 0 && mi < width && mj < height) {
			if (GetMouse(SAPP_MOUSEBUTTON_LEFT).held) grid[ix(mi, mj)] = true;
			if (GetMouse(SAPP_MOUSEBUTTON_RIGHT).held) grid[ix(mi, mj)] = false;
		}

		if (GetKey(SAPP_KEYCODE_G).pressed) show_grid ^= true;

	}

	bool user_update(float dt) override {
		//convert mouse coords to grid index

		handleUserInput(dt);


		

		player_dir = cmn::polar<cmn::vf2d>(1, player_rot);

		handleCollision(player_pos, player_rad);

		updateSizing();

		return true;
	}

	bool user_render() override {
		sg_pass pass{};
		pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
		pass.action.colors[0].clear_value = { 0, 0, 0, 0 };
		pass.swapchain = sglue_swapchain();
		sg_begin_pass(pass);

		sgl_defaults();

		sgl_matrix_mode_projection();
		sgl_ortho(0, sapp_widthf(), sapp_heightf(), 0, -1, 1);

		


		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				//skip "air" cells
				float x = cell_size * i;
				float y = cell_size * j;
				if (!grid[ix(i, j)])
				{
					cmn::fill_rect(
						x, y, cell_size, cell_size,
						{ 0, 1, 1}
					);
				}
				else
				{
					cmn::fill_rect(
						x, y, cell_size, cell_size,
						{ 0, 0, 0 }
					);
				}

				
				
			}
		}

		if (show_grid)
		{
			for (int i = 0; i < width; i++) {
				float x = cell_size * i;
				cmn::draw_line(x, 0, x, sapp_heightf(), { 0.0f, 1.0f, 1.0f });
			}

			for (int j = 0; j < height; j++) {
				float y = cell_size * j;
				cmn::draw_line(0, y, sapp_widthf(), y, { 0.0f, 1.0f, 1.0f });
			}
		}

		//draw player
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {

			}
		}
		cmn::fill_circle(player_pos.x, player_pos.y, player_rad, { 1,0,1,1 });
		cmn::draw_line(player_pos.x, player_pos.y, player_pos.x + player_rad * player_dir.x, player_pos.y + player_rad * player_dir.y, { 0,1,0,1 });

		sgl_draw();

		sg_end_pass();

		sg_commit();

		return true;
	}
};

#endif // !RAYCAST_H

