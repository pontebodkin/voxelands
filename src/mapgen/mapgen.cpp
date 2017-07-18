/************************************************************************
* Minetest-c55
* Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
*
* mapgen.cpp
* voxelands - 3d voxel world sandbox game
* Copyright (C) Lisa 'darkrose' Milne 2014 <lisa@ltmnet.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* License updated from GPLv2 or later to GPLv3 or later by Lisa Milne
* for Voxelands.
************************************************************************/

#include "mapgen.h"
#include "voxel.h"
#include "content_mapnode.h"
#include "noise.h"
#include "mapblock.h"
#include "map.h"
#include "mineral.h"
#include "content_sao.h"

namespace mapgen
{

void make_block(BlockMakeData *data)
{
	if (data->no_op)
		return;
	calc_biome(data);

	if (data->biome == BIOME_THEDEEP) {
		make_thedeep(data);
		return;
	}

	if (data->biome == BIOME_SPACE) {
		make_space(data);
		return;
	}

	if (data->biome == BIOME_SKY) {
		make_sky(data);
		return;
	}

	v3s16 blockpos = data->blockpos;

	ManualMapVoxelManipulator &vmanip = *(data->vmanip);
	// Area of center block
	v3s16 node_min = blockpos*MAP_BLOCKSIZE;
	v3s16 node_max = (blockpos+v3s16(1,1,1))*MAP_BLOCKSIZE-v3s16(1,1,1);
	// Full allocated area
	v3s16 full_node_min = (blockpos-1)*MAP_BLOCKSIZE;

	v2s16 p2d_center(node_min.X+MAP_BLOCKSIZE/2, node_min.Z+MAP_BLOCKSIZE/2);


	/*
		Get average ground level from noise
	*/

	s16 approx_groundlevel = (s16)get_sector_average_ground_level(data, v2s16(blockpos.X, blockpos.Z));

	s16 approx_ground_depth = approx_groundlevel - (node_min.Y+MAP_BLOCKSIZE/2);

	s16 minimum_groundlevel = (s16)get_sector_minimum_ground_level(data, v2s16(blockpos.X, blockpos.Z));
	// Minimum amount of ground above the top of the central block
	s16 minimum_ground_depth = minimum_groundlevel - node_max.Y;

	s16 maximum_groundlevel = (s16)get_sector_maximum_ground_level(data, v2s16(blockpos.X, blockpos.Z));
	// Maximum amount of ground above the bottom of the central block
	s16 maximum_ground_depth = maximum_groundlevel - node_min.Y;

	/*
		If block is deep underground, this is set to true and ground
		density noise is not generated, for speed optimization.
	*/
	bool all_is_ground_except_caves = (minimum_ground_depth > 40);

	/*
		Create a block-specific seed
	*/
	u32 blockseed = (u32)(data->seed%0x100000000ULL) + full_node_min.Z*38134234
			+ full_node_min.Y*42123 + full_node_min.X*23;

	/*
		Make some 3D noise
	*/

	//NoiseBuffer noisebuf1;
	//NoiseBuffer noisebuf2;
	NoiseBuffer noisebuf_cave;
	NoiseBuffer noisebuf_ground_crumbleness;
	NoiseBuffer noisebuf_ground_wetness;
	{
		v3f minpos_f(node_min.X, node_min.Y, node_min.Z);
		v3f maxpos_f(node_max.X, node_max.Y, node_max.Z);

		//TimeTaker timer("noisebuf.create");

		/*
			Cave noise
		*/
#if 1
		noisebuf_cave.create(get_cave_noise1_params(data->seed),
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y, maxpos_f.Z,
				2, 2, 2);
		noisebuf_cave.multiply(get_cave_noise2_params(data->seed));
#endif

		/*
			Ground noise
		*/

		// Sample length
		v3f sl = v3f(4.0, 4.0, 4.0);

		/*
			Ground property noise
		*/
		sl = v3f(2.5, 2.5, 2.5);
		noisebuf_ground_crumbleness.create(
				get_ground_crumbleness_params(data->seed),
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y+5, maxpos_f.Z,
				sl.X, sl.Y, sl.Z);
		noisebuf_ground_wetness.create(
				get_ground_wetness_params(data->seed),
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y+5, maxpos_f.Z,
				sl.X, sl.Y, sl.Z);
	}


	bool limestone = (noisebuf_ground_wetness.get(node_min.X+8,node_min.Y+8,node_min.Z+8) > 0.5);
	content_t base_content = CONTENT_STONE;
	if (limestone && data->biome != BIOME_WASTELANDS)
		base_content = CONTENT_LIMESTONE;

	/*
		Make base ground level
	*/

	for (s16 x=node_min.X; x<=node_max.X; x++)
	for (s16 z=node_min.Z; z<=node_max.Z; z++) {
		// Node position
		v2s16 p2d(x,z);
		{
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_min.Y, p2d.Y));
			int16_t h = get_ground_height(data->seed,p2d);
			for (s16 y=node_min.Y; y<=node_max.Y; y++) {
				// Only modify places that have no content
				if (vmanip.m_data[i].getContent() == CONTENT_IGNORE) {
					// First priority: make air and water.
					// This avoids caves inside water.
					if (
						all_is_ground_except_caves == false
						&& y>h
					) {
						if (y <= WATER_LEVEL) {
							vmanip.m_data[i] = MapNode(CONTENT_WATERSOURCE);
						}else if (y>=1024) {
							vmanip.m_data[i] = MapNode(CONTENT_VACUUM);
						}else{
							vmanip.m_data[i] = MapNode(CONTENT_AIR);
						}
					}else if (noisebuf_cave.get(x,y,z) > CAVE_NOISE_THRESHOLD) {
						vmanip.m_data[i] = MapNode(CONTENT_AIR);
					}else{
						vmanip.m_data[i] = MapNode(base_content);
					}
				}

				data->vmanip->m_area.add_y(em, i, 1);
			}
		}
	}

	/*
		Add minerals
	*/

	{
		PseudoRandom mineralrandom(blockseed);

		/*
			Add mithril blocks
		*/
		for (s16 i=0; i<approx_ground_depth/4; i++) {
			if (mineralrandom.next()%50 == 0) {
				s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
				s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
				s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
				// TODO: at random, some should be gems
				u8 type = mineralrandom.next()%12;
				switch (type) {
				case 0:
					type = MINERAL_RUBY;
					break;
				case 1:
					type = MINERAL_TURQUOISE;
					break;
				case 2:
					type = MINERAL_AMETHYST;
					break;
				case 3:
					type = MINERAL_SAPPHIRE;
					break;
				case 4:
					type = MINERAL_SUNSTONE;
					break;
				default:
					type = MINERAL_MITHRIL;
				}
				for (u16 i=0; i<27; i++) {
					v3s16 p = v3s16(x,y,z) + g_27dirs[i];
					u32 vi = vmanip.m_area.index(p);
					if (vmanip.m_data[vi].getContent() == base_content && mineralrandom.next()%8 == 0)
						vmanip.m_data[vi] = MapNode(base_content,type);
				}

			}
		}
		/*
			Add others
		*/
		if (limestone) {
			/*
				Add tin/quartz (and rare coal)
			*/
			u16 a = mineralrandom.range(0,15);
			a = a*a*a;
			u16 amount = 20;
			u16 rareness = 40 / amount;
			if (rareness == 0)
				rareness = 1;
			if (mineralrandom.next()%rareness == 0) {
				u16 a = mineralrandom.next() % 16;
				u16 amnt = amount * a*a*a / 1000;
				for (s16 i=0; i<amnt; i++) {
					s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
					s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
					s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
					u8 type = mineralrandom.next()%12;
					if (type == 0) {
						type = MINERAL_COAL;
					}else if (type > 8) {
						type =  MINERAL_QUARTZ;
					}else if (type > 4) {
						type =  MINERAL_COPPER;
					}else{
						type = MINERAL_TIN;
					}
					for (u16 i=0; i<27; i++) {
						v3s16 p = v3s16(x,y,z) + g_27dirs[i];
						u32 vi = vmanip.m_area.index(p);
						if (vmanip.m_data[vi].getContent() == CONTENT_LIMESTONE && mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(CONTENT_LIMESTONE, type);
					}
				}
			}
		}else{
			{
				u16 a = mineralrandom.range(0,15);
				a = a*a*a;
				u16 amount = 20 * a/1000;
				for (s16 i=0; i<amount; i++) {
					s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
					s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
					s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);

					u8 base_content = CONTENT_STONE;
					MapNode new_content(CONTENT_IGNORE);
					u32 sparseness = 6;

					if (noisebuf_ground_crumbleness.get(x,y+5,z) < -0.1) {
						new_content = MapNode(CONTENT_STONE, MINERAL_COAL);
					}else if (noisebuf_ground_wetness.get(x,y+5,z) > 0.1) {
						new_content = MapNode(CONTENT_STONE, MINERAL_IRON);
					}else if (noisebuf_ground_crumbleness.get(x,y,z) > 0.4) {
						new_content = MapNode(CONTENT_STONE, MINERAL_SILVER);
					}else if (noisebuf_ground_crumbleness.get(x,y,z) > 0.3) {
						new_content = MapNode(CONTENT_STONE, MINERAL_QUARTZ);
					}else if (noisebuf_ground_crumbleness.get(x,y,z) > 0.2) {
						new_content = MapNode(CONTENT_STONE, MINERAL_COPPER);
					}else if (noisebuf_ground_crumbleness.get(x,y,z) > 0.1) {
						new_content = MapNode(CONTENT_STONE, MINERAL_TIN);
					}else if (noisebuf_ground_wetness.get(x,y+5,z) > 0.0) {
						new_content = MapNode(CONTENT_STONE, MINERAL_GOLD);
					}

					if (new_content.getContent() != CONTENT_IGNORE) {
						for (u16 i=0; i<27; i++) {
							v3s16 p = v3s16(x,y,z) + g_27dirs[i];
							u32 vi = vmanip.m_area.index(p);
							if (vmanip.m_data[vi].getContent() == base_content && mineralrandom.next()%sparseness == 0)
								vmanip.m_data[vi] = new_content;
						}
					}
				}
			}
			/*
				Add coal
			*/
			u16 coal_amount = 30;
			u16 coal_rareness = 60 / coal_amount;
			if (coal_rareness == 0)
				coal_rareness = 1;
			if (mineralrandom.next()%coal_rareness == 0) {
				u16 a = mineralrandom.next() % 16;
				u16 amount = coal_amount * a*a*a / 1000;
				for (s16 i=0; i<amount; i++) {
					s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
					s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
					s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
					for (u16 i=0; i<27; i++) {
						v3s16 p = v3s16(x,y,z) + g_27dirs[i];
						u32 vi = vmanip.m_area.index(p);
						if (vmanip.m_data[vi].getContent() == CONTENT_STONE && mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(CONTENT_STONE, MINERAL_COAL);
					}
				}
			}
			/*
				Add iron
			*/
			u16 iron_amount = 8;
			u16 iron_rareness = 60 / iron_amount;
			if (iron_rareness == 0)
				iron_rareness = 1;
			if (mineralrandom.next()%iron_rareness == 0) {
				u16 a = mineralrandom.next() % 16;
				u16 amount = iron_amount * a*a*a / 1000;
				for (s16 i=0; i<amount; i++) {
					s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
					s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
					s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
					for (u16 i=0; i<27; i++) {
						v3s16 p = v3s16(x,y,z) + g_27dirs[i];
						u32 vi = vmanip.m_area.index(p);
						if (vmanip.m_data[vi].getContent() == CONTENT_STONE && mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(CONTENT_STONE, MINERAL_IRON);
					}
				}
			}
		}
	}

	/*
		Add mud and sand and others underground (in place of stone)
	*/
	content_t liquid_type = CONTENT_LAVASOURCE;
	if (limestone || blockpos.Y > -1 || ((blockpos.X + blockpos.Z)/blockpos.Y+1)%16 == 0)
		liquid_type = CONTENT_WATERSOURCE;

	for (s16 x=node_min.X; x<=node_max.X; x++)
	for (s16 z=node_min.Z; z<=node_max.Z; z++) {
		// Node position
		v2s16 p2d(x,z);
		{
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
			for (s16 y=node_max.Y; y>=node_min.Y; y--) {
				if (vmanip.m_data[i].getContent() == base_content) {
					if (noisebuf_ground_crumbleness.get(x,y,z) > 1.3) {
						if (noisebuf_ground_wetness.get(x,y,z) > 0.0) {
							vmanip.m_data[i] = MapNode(CONTENT_MUD);
						}else{
							vmanip.m_data[i] = MapNode(CONTENT_SAND);
						}
					}else if (noisebuf_ground_crumbleness.get(x,y,z) > 0.7) {
						if (noisebuf_ground_wetness.get(x,y,z) < -0.6)
							vmanip.m_data[i] = MapNode(CONTENT_GRAVEL);
					}else if (noisebuf_ground_crumbleness.get(x,y,z) < -3.0 + MYMIN(0.1 * sqrt((float)MYMAX(0, -y)), 1.5)) {
						vmanip.m_data[i] = MapNode(liquid_type);
						for (s16 x1=-1; x1<=1; x1++)
						for (s16 y1=-1; y1<=1; y1++)
						for (s16 z1=-1; z1<=1; z1++) {
							data->transforming_liquid.push_back(v3s16(p2d.X+x1, y+y1, p2d.Y+z1));
						}
					}
				}

				data->vmanip->m_area.add_y(em, i, -1);
			}
		}
	}

	/* Add dungeons */
	if (
		!limestone
		&& (data->biome == BIOME_WOODLANDS || data->biome == BIOME_JUNGLE || data->biome == BIOME_DESERT)
		&& ((noise3d(blockpos.X,blockpos.Y,blockpos.Z,data->seed)+1.0)/2.0) < 0.2
		&& node_min.Y < approx_groundlevel
	) {
		make_dungeon(data,blockseed);
	}

	/*
		Add top and bottom side of water to transforming_liquid queue
	*/

	for (s16 x=node_min.X; x<=node_max.X; x++)
	for (s16 z=node_min.Z; z<=node_max.Z; z++) {
		// Node position
		v2s16 p2d(x,z);
		{
			bool water_found = false;
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
			for (s16 y=node_max.Y; y>=node_min.Y; y--) {
				if (!water_found) {
					if (vmanip.m_data[i].getContent() == CONTENT_WATERSOURCE) {
						v3s16 p = v3s16(p2d.X, y, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = true;
					}
				}else{
					// This can be done because water_found can only
					// turn to true and end up here after going through
					// a single block.
					if (vmanip.m_data[i+1].getContent() != CONTENT_WATERSOURCE) {
						v3s16 p = v3s16(p2d.X, y+1, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = false;
					}
				}

				data->vmanip->m_area.add_y(em, i, -1);
			}
		}
	}

	/*
		If close to ground level
	*/

	if (minimum_ground_depth < 5 && maximum_ground_depth > -5) {
		/*
			Add grass and mud
		*/

		for (s16 x=node_min.X; x<=node_max.X; x++)
		for (s16 z=node_min.Z; z<=node_max.Z; z++) {
			// Node position
			v2s16 p2d(x,z);
			{
				u32 current_depth = 0;
				bool air_detected = false;
				bool water_detected = false;

				// Use fast index incrementing
				s16 start_y = node_max.Y+2;
				v3s16 em = vmanip.m_area.getExtent();
				u32 i = vmanip.m_area.index(v3s16(p2d.X, start_y, p2d.Y));
				uint8_t biome = get_block_biome(data,v3s16(x,0,z));
				for (s16 y=start_y; y>=node_min.Y-3; y--) {
					if (vmanip.m_data[i].getContent() == CONTENT_WATERSOURCE)
						water_detected = true;
					if (vmanip.m_data[i].getContent() == CONTENT_AIR)
						air_detected = true;

					if (
						(
							vmanip.m_data[i].getContent() == base_content
							|| vmanip.m_data[i].getContent() == CONTENT_MUD
							|| vmanip.m_data[i].getContent() == CONTENT_SAND
							|| vmanip.m_data[i].getContent() == CONTENT_GRAVEL
						) && (
							air_detected || water_detected
						)
					) {
						if (biome == BIOME_DESERT) {
							vmanip.m_data[i] = MapNode(CONTENT_DESERT_SAND);
						}else if (current_depth < 4) {
							if (biome == BIOME_BEACH || biome == BIOME_OCEAN) {
								vmanip.m_data[i] = MapNode(CONTENT_SAND);
							}else if (current_depth==0 && !water_detected && y >= WATER_LEVEL && air_detected) {
								if (biome == BIOME_SNOWCAP) {
									vmanip.m_data[i] = MapNode(CONTENT_MUD,0x04);
								}else if (biome == BIOME_WASTELANDS) {
									vmanip.m_data[i] = MapNode(CONTENT_MUD,0x06);
								}else if (biome == BIOME_JUNGLE) {
									if (noisebuf_ground_wetness.get(x,y,z) > 1.0) {
										vmanip.m_data[i] = MapNode(CONTENT_CLAY,0x08);
									}else{
										vmanip.m_data[i] = MapNode(CONTENT_MUD,0x08);
									}
								}else if (noisebuf_ground_wetness.get(x,y,z) > 1.0) {
									vmanip.m_data[i] = MapNode(CONTENT_CLAY,0x01);
								}else{
									vmanip.m_data[i] = MapNode(CONTENT_MUD,0x01);
								}
							}else{
								vmanip.m_data[i] = MapNode(CONTENT_MUD);
							}
						}else{
							if (vmanip.m_data[i].getContent() == CONTENT_MUD)
								vmanip.m_data[i] = MapNode(base_content);
						}

						current_depth++;

						if (current_depth >= 8)
							break;
					}else if (current_depth != 0) {
						break;
					}

					data->vmanip->m_area.add_y(em, i, -1);
				}
			}
		}

		/* add trees */
		u32 tree_count = get_tree_density(data, p2d_center);
			if (tree_count) {
			PseudoRandom treerandom(blockseed);
			// Put trees in random places on part of division
			for (u32 i=0; i<tree_count; i++) {
				s16 x = treerandom.range(node_min.X, node_max.X);
				s16 z = treerandom.range(node_min.Z, node_max.Z);
				s16 y = find_ground_level_from_noise(data, v2s16(x,z), 4);
				// Don't make a tree under water level
				if (y < WATER_LEVEL)
					continue;
				// Make sure tree fits (only trees whose starting point is
				// at this block are added)
				if (y < node_min.Y || y > node_max.Y)
					continue;
				/*
					Find exact ground level
				*/
				v3s16 p(x,y+6,z);
				bool found = false;
				for (; p.Y >= y-6; p.Y--) {
					u32 i = data->vmanip->m_area.index(p);
					MapNode *n = &data->vmanip->m_data[i];
					if (n->getContent() != CONTENT_AIR && n->getContent() != CONTENT_WATERSOURCE && n->getContent() != CONTENT_IGNORE) {
						found = true;
						break;
					}
				}
				// If not found, handle next one
				if (found == false)
					continue;

				{
					u32 i = data->vmanip->m_area.index(p);
					MapNode *n = &data->vmanip->m_data[i];

					if (n->getContent() == CONTENT_MUD) {
						// just stumps in wastelands
						if (data->biome == BIOME_WASTELANDS) {
							p.Y++;
							if (data->vmanip->m_area.contains(p)) {
								u32 ip = data->vmanip->m_area.index(p);
								vmanip.m_data[ip] = MapNode(CONTENT_TREE,0xE0);
							}
						// Papyrus grows only on mud and in water
						}else if (y <= WATER_LEVEL) {
							p.Y++;
							make_papyrus(vmanip, p);
						// Trees grow only on mud and grass, on land
						}else if (data->biome == BIOME_LAKE) {
							make_appletree(vmanip, p);
						}else if (y > (WATER_LEVEL+2)) {
							p.Y++;
							if (data->biome == BIOME_JUNGLE) {
								make_jungletree(vmanip, p);
							// connifers
							}else if (data->biome == BIOME_SNOWCAP) {
								make_conifertree(vmanip, p);
							}else if (data->biome == BIOME_PLAINS) {
								make_tree(vmanip, p);
							// regular trees
							}else if (myrand_range(0,10) != 0) {
								make_tree(vmanip, p);
							}else{
								make_largetree(vmanip, p);
							}
						}
					// Cactii grow only on sand, on land
					}else if (n->getContent() == CONTENT_DESERT_SAND) {
						if (y > (WATER_LEVEL+2)) {
							p.Y++;
							make_cactus(vmanip, p);
						}
					// bushes on clay
					}else if (n->getContent() == CONTENT_CLAY) {
						if (y > WATER_LEVEL+5) {
							p.Y++;
							if (vmanip.m_area.contains(p)) {
								if (data->biome == BIOME_JUNGLE) {
									vmanip.m_data[vmanip.m_area.index(p)] = MapNode(CONTENT_BUSH_RASPBERRY);
								}else if (data->biome == BIOME_FOREST || data->biome == BIOME_LAKE || data->biome == BIOME_WOODLANDS) {
									vmanip.m_data[vmanip.m_area.index(p)] = MapNode(CONTENT_BUSH_BLUEBERRY);
								}
							}
						}
					}
				}
			}
		}

		/* add grasses */
		u32 grass_count = get_grass_density(data, p2d_center);
		if (grass_count) {
			PseudoRandom grassrandom(blockseed);
			for (u32 i=0; i<grass_count; i++) {
				s16 x = grassrandom.range(node_min.X, node_max.X);
				s16 z = grassrandom.range(node_min.Z, node_max.Z);
				s16 y = get_ground_height(data->seed, v2s16(x,z));
				if (y < WATER_LEVEL)
					continue;
				if (y < node_min.Y || y > node_max.Y)
					continue;
				/*
					Find exact ground level
				*/
				v3s16 p(x,y+6,z);
				bool found = false;
				for (; p.Y >= y-6; p.Y--) {
					u32 i = data->vmanip->m_area.index(p);
					MapNode *n = &data->vmanip->m_data[i];
					if (content_features(*n).is_ground_content || n->getContent() == CONTENT_JUNGLETREE) {
						found = true;
						break;
					}
				}
				// If not found, handle next one
				if (found == false)
					continue;
				p.Y++;
				if (vmanip.m_area.contains(p) == false)
					continue;
				if (vmanip.m_data[vmanip.m_area.index(p)].getContent() != CONTENT_AIR)
					continue;
				if (vmanip.m_area.contains(p)) {
					if (data->biome != BIOME_JUNGLE) {
						vmanip.m_data[vmanip.m_area.index(p)] = CONTENT_WILDGRASS_LONG;
					}else if (y < 30 && myrand_range(0,20) == 0) {
						if (y > 20) {
							vmanip.m_data[vmanip.m_area.index(p)] = CONTENT_TEA;
						}else{
							vmanip.m_data[vmanip.m_area.index(p)] = CONTENT_COFFEE;
						}
					}else if (myrand_range(0,3) == 0) {
						vmanip.m_data[vmanip.m_area.index(p)] = CONTENT_JUNGLEFERN;
					}else{
						vmanip.m_data[vmanip.m_area.index(p)] = CONTENT_JUNGLEGRASS;
					}
				}
			}
		}

		if (data->biome == BIOME_WASTELANDS) {
			/* fallen meteor */
		}
	}
}

BlockMakeData::BlockMakeData():
	no_op(false),
	vmanip(NULL),
	seed(0),
	type(MGT_DEFAULT),
	biome(BIOME_UNKNOWN)
{
	int i;
	for (i=0; i<8; i++) {
		surrounding_biomes[i] = BIOME_UNKNOWN;
	}
}

BlockMakeData::~BlockMakeData()
{
	delete vmanip;
}

}; // namespace mapgen


