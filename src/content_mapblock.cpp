/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "content_mapblock.h"
#include "content_mapnode.h"
#include "main.h" // For g_settings and g_texturesource
#include "mineral.h"
#include "mapblock_mesh.h" // For MapBlock_LightColor()
#include "settings.h"

#ifndef SERVER
// Create a cuboid.
// collector - the MeshCollector for the resulting polygons
// box - the position and size of the box
// tiles - the tiles (materials) to use (for all 6 faces)
// tilecount - number of entries in tiles, 1<=tilecount<=6
// c - vertex colour - used for all
// txc - texture coordinates - this is a list of texture coordinates
// for the opposite corners of each face - therefore, there
// should be (2+2)*6=24 values in the list. Alternatively, pass
// NULL to use the entire texture for each face. The order of
// the faces in the list is up-down-right-left-back-front
// (compatible with ContentFeatures). If you specified 0,0,1,1
// for each face, that would be the same as passing NULL.
void makeAngledCuboid(MeshCollector *collector, v3f pos, const aabb3f &box,
	TileSpec *tiles, int tilecount,
	video::SColor c[8], const f32* txc, s16 angle)
{
	assert(tilecount >= 1 && tilecount <= 6);

	v3f min = box.MinEdge;
	v3f max = box.MaxEdge;

	if (txc == NULL) {
		static const f32 txc_default[24] = {
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1
		};
		txc = txc_default;
	}

	video::S3DVertex vertices[24] = {
		// up
		video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, c[0], txc[0],txc[1]),
		video::S3DVertex(max.X,max.Y,max.Z, 0,1,0, c[1], txc[2],txc[1]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,1,0, c[2], txc[2],txc[3]),
		video::S3DVertex(min.X,max.Y,min.Z, 0,1,0, c[3], txc[0],txc[3]),
		// down
		video::S3DVertex(min.X,min.Y,min.Z, 0,-1,0, c[4], txc[4],txc[5]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,-1,0, c[5], txc[6],txc[5]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,-1,0, c[6], txc[6],txc[7]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,-1,0, c[7], txc[4],txc[7]),
		// right
		video::S3DVertex(max.X,max.Y,min.Z, 1,0,0, c[2], txc[ 8],txc[9]),
		video::S3DVertex(max.X,max.Y,max.Z, 1,0,0, c[1], txc[10],txc[9]),
		video::S3DVertex(max.X,min.Y,max.Z, 1,0,0, c[6], txc[10],txc[11]),
		video::S3DVertex(max.X,min.Y,min.Z, 1,0,0, c[5], txc[ 8],txc[11]),
		// left
		video::S3DVertex(min.X,max.Y,max.Z, -1,0,0, c[0], txc[12],txc[13]),
		video::S3DVertex(min.X,max.Y,min.Z, -1,0,0, c[3], txc[14],txc[13]),
		video::S3DVertex(min.X,min.Y,min.Z, -1,0,0, c[4], txc[14],txc[15]),
		video::S3DVertex(min.X,min.Y,max.Z, -1,0,0, c[7], txc[12],txc[15]),
		// back
		video::S3DVertex(max.X,max.Y,max.Z, 0,0,1, c[1], txc[16],txc[17]),
		video::S3DVertex(min.X,max.Y,max.Z, 0,0,1, c[0], txc[18],txc[17]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,0,1, c[7], txc[18],txc[19]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,0,1, c[6], txc[16],txc[19]),
		// front
		video::S3DVertex(min.X,max.Y,min.Z, 0,0,-1, c[3], txc[20],txc[21]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,0,-1, c[2], txc[22],txc[21]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,0,-1, c[5], txc[22],txc[23]),
		video::S3DVertex(min.X,min.Y,min.Z, 0,0,-1, c[4], txc[20],txc[23]),
	};


	for (s32 j=0; j<24; j++) {
		int tileindex = MYMIN(j/4, tilecount-1);
		if (angle)
			vertices[j].Pos.rotateXZBy(angle);
		vertices[j].Pos += pos;
		vertices[j].TCoords *= tiles[tileindex].texture.size;
		vertices[j].TCoords += tiles[tileindex].texture.pos;
	}
	u16 indices[] = {0,1,2,2,3,0};
	// Add to mesh collector
	for (s32 j=0; j<24; j+=4) {
		int tileindex = MYMIN(j/4, tilecount-1);
		collector->append(tiles[tileindex].getMaterial(), vertices+j, 4, indices, 6);
	}
}
void makeCuboid(MeshCollector *collector, const aabb3f &box,
	TileSpec *tiles, int tilecount,
	video::SColor c[8], const f32* txc)
{
	makeAngledCuboid(collector,v3f(0,0,0),box,tiles,tilecount,c,txc,0);
}

/*
 * get the light values for a node
 * smooth lighting gets per-vertex
 * standard lighting gets per-face
 * TODO: smooth lighting currently gets the light for each surrounding node up
 * to eight times, probably a better way to do this
 */
static void getLights(v3s16 pos, video::SColor *lights, MeshMakeData *data, bool smooth_lighting, u8 vertex_alpha)
{
	if (!smooth_lighting) {
		u8 l = 0;
		u32 lt = 0;
		u32 ltp;
		u8 ld = 1;
		for (s16 tx=-1; tx<2; tx++) {
		for (s16 ty=-1; ty<2; ty++) {
		for (s16 tz=-1; tz<2; tz++) {
			MapNode tn = data->m_vmanip.getNodeNoEx(pos + v3s16(tx,ty,tz));
			if (
				ty<1
				&& (
					tn.getContent() != CONTENT_AIR
					&& content_features(tn).light_source == 0
					&& content_features(tn).param_type != CPT_LIGHT
				)
			)
				continue;
			ltp = decode_light(tn.getLightBlend(data->m_daynight_ratio));
			if (!ltp)
				continue;
			lt += ltp;
			ld++;
		}
		}
		}
		if (ld)
			l = lt/ld;
		video::SColor c = MapBlock_LightColor(vertex_alpha, l);
		for (int i=0; i<8; i++) {
			lights[i] = c;
		}
		return;
	}
	static const v3s16 corners[8] = {
		v3s16(0,1,1),
		v3s16(1,1,1),
		v3s16(1,1,0),
		v3s16(0,1,0),
		v3s16(0,0,0),
		v3s16(1,0,0),
		v3s16(1,0,1),
		v3s16(0,0,1)
	};
	u8 l;
	u32 lt;
	u32 ltp;
	u8 ld;
	MapNode tn;
	for (int i=0; i<8; i++) {
		l = 0;
		lt = 0;
		ld = 1;
		for (s16 tx=-1; tx<1; tx++) {
		for (s16 ty=-1; ty<1; ty++) {
		for (s16 tz=-1; tz<1; tz++) {
			tn = data->m_vmanip.getNodeNoEx(pos + v3s16(tx,ty,tz) + corners[i]);
			if (
				ty<1
				&& (
					tn.getContent() != CONTENT_AIR
					&& content_features(tn).light_source == 0
					&& content_features(tn).param_type != CPT_LIGHT
				)
			)
				continue;
			ltp = decode_light(tn.getLightBlend(data->m_daynight_ratio));
			if (!ltp)
				continue;
			lt += ltp;
			ld++;
		}
		}
		}
		if (ld)
			l = lt/ld;
		lights[i] = MapBlock_LightColor(vertex_alpha, l);
	}
}
static void getLights(v3s16 pos, video::SColor *lights, MeshMakeData *data, bool smooth_lighting)
{
	getLights(pos,lights,data,smooth_lighting,255);
}
#endif

#ifndef SERVER
static void mapblock_mesh_check_walllike(MeshMakeData *data, v3s16 p, u8 d[8], u8 h[8], bool *post)
{
	for (s16 i=0; i<8; i++) {
		h[i] = 0;
		d[i] = 0;
	}
	// Now a section of fence, +X, if there's a post there
	v3s16 p2 = p;
	p2.X++;
	MapNode n2 = data->m_vmanip.getNodeNoEx(p2);
	const ContentFeatures *f2 = &content_features(n2);
	if (
		f2->draw_type == CDT_FENCELIKE
		|| f2->draw_type == CDT_WALLLIKE
		|| n2.getContent() == CONTENT_WOOD_GATE
		|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
		|| n2.getContent() == CONTENT_STEEL_GATE
		|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
	) {
		d[0] = 1;
	}

	// Now a section of fence, -X, if there's a post there
	p2 = p;
	p2.X--;
	n2 = data->m_vmanip.getNodeNoEx(p2);
	f2 = &content_features(n2);
	if (
		f2->draw_type == CDT_FENCELIKE
		|| f2->draw_type == CDT_WALLLIKE
		|| n2.getContent() == CONTENT_WOOD_GATE
		|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
		|| n2.getContent() == CONTENT_STEEL_GATE
		|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
	) {
		d[1] = 1;
	}

	// Now a section of fence, +Z, if there's a post there
	p2 = p;
	p2.Z++;
	n2 = data->m_vmanip.getNodeNoEx(p2);
	f2 = &content_features(n2);
	if (
		f2->draw_type == CDT_FENCELIKE
		|| f2->draw_type == CDT_WALLLIKE
		|| n2.getContent() == CONTENT_WOOD_GATE
		|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
		|| n2.getContent() == CONTENT_STEEL_GATE
		|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
	) {
		d[2] = 1;
	}

	// Now a section of fence, +Z, if there's a post there
	p2 = p;
	p2.Z--;
	n2 = data->m_vmanip.getNodeNoEx(p2);
	f2 = &content_features(n2);
	if (
		f2->draw_type == CDT_FENCELIKE
		|| f2->draw_type == CDT_WALLLIKE
		|| n2.getContent() == CONTENT_WOOD_GATE
		|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
		|| n2.getContent() == CONTENT_STEEL_GATE
		|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
	) {
		d[3] = 1;
	}
	if (!d[0] && !d[2]) {
		p2 = p;
		p2.X++;
		p2.Z++;
		n2 = data->m_vmanip.getNodeNoEx(p2);
		f2 = &content_features(n2);
		if (
			f2->draw_type == CDT_FENCELIKE
			|| f2->draw_type == CDT_WALLLIKE
		)
			d[4] = 1;
	}
	if (!d[0] && !d[3]) {
		p2 = p;
		p2.X++;
		p2.Z--;
		n2 = data->m_vmanip.getNodeNoEx(p2);
		f2 = &content_features(n2);
		if (
			f2->draw_type == CDT_FENCELIKE
			|| f2->draw_type == CDT_WALLLIKE
		)
			d[5] = 1;
	}
	if (!d[1] && !d[2]) {
		p2 = p;
		p2.X--;
		p2.Z++;
		n2 = data->m_vmanip.getNodeNoEx(p2);
		f2 = &content_features(n2);
		if (
			f2->draw_type == CDT_FENCELIKE
			|| f2->draw_type == CDT_WALLLIKE
		)
			d[6] = 1;
	}
	if (!d[1] && !d[3]) {
		p2 = p;
		p2.X--;
		p2.Z--;
		n2 = data->m_vmanip.getNodeNoEx(p2);
		f2 = &content_features(n2);
		if (
			f2->draw_type == CDT_FENCELIKE
			|| f2->draw_type == CDT_WALLLIKE
		)
			d[7] = 1;
	}
	u8 ps = d[0]+d[1]+d[2]+d[3]+d[4]+d[5]+d[6]+d[7];
	p2 = p;
	p2.Y++;
	n2 = data->m_vmanip.getNodeNoEx(p2);
	u8 ad[8];
	u8 ah[8];
	bool ap;
	if (content_features(n2).draw_type == CDT_WALLLIKE) {
		mapblock_mesh_check_walllike(data, p2,ad,ah,&ap);
		if (ad[0] && d[0])
			h[0] = 1;
		if (ad[1] && d[1])
			h[1] = 1;
		if (ad[2] && d[2])
			h[2] = 1;
		if (ad[3] && d[3])
			h[3] = 1;
		if (ad[4] && d[4])
			h[4] = 1;
		if (ad[5] && d[5])
			h[5] = 1;
		if (ad[6] && d[6])
			h[6] = 1;
		if (ad[7] && d[7])
			h[7] = 1;
	}
	if (ps == 2) {
		*post = false;
		if (d[0] && d[2]) {
			*post = true;
		}else if (d[1] && d[3]) {
			*post = true;
		}else if (d[0] && d[3]) {
			*post = true;
		}else if (d[1] && d[2]) {
			*post = true;
		}else if (d[4] || d[5] || d[6] || d[7]) {
			*post = true;
		}else if (content_features(n2).draw_type == CDT_WALLLIKE) {
			*post = ap;
		}else if (n2.getContent() != CONTENT_AIR && n2.getContent() != CONTENT_IGNORE) {
			*post = true;
		}
	}else{
		*post = true;
	}
}

void mapblock_mesh_generate_special(MeshMakeData *data,
		MeshCollector &collector)
{
	/*
		Some settings
	*/
	bool new_style_water = g_settings->getBool("new_style_water");
	bool smooth_lighting = g_settings->getBool("smooth_lighting");

	float node_liquid_level = 1.0;
	if(new_style_water)
		node_liquid_level = 0.85;

	v3s16 blockpos_nodes = data->m_blockpos*MAP_BLOCKSIZE;

	for(s16 z=0; z<MAP_BLOCKSIZE; z++)
	for(s16 y=0; y<MAP_BLOCKSIZE; y++)
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		v3s16 p(x,y,z);

		MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes+p);

		/*
			Add torches to mesh
		*/
		switch (content_features(n).draw_type) {
		case CDT_CUBELIKE:
		case CDT_AIRLIKE:
			break;
		case CDT_LIQUID:
		{
			bool top_is_same_liquid = false;
			MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			content_t c_flowing = content_features(n).liquid_alternative_flowing;
			content_t c_source = content_features(n).liquid_alternative_source;
			if(ntop.getContent() == c_flowing || ntop.getContent() == c_source)
				top_is_same_liquid = true;

			u8 l = 0;
			// Use the light of the node on top if possible
			if (content_features(ntop).param_type == CPT_LIGHT){
				l = decode_light(ntop.getLightBlend(data->m_daynight_ratio));
			// Otherwise use the light of this node (the liquid)
			}else{
				l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			}
			video::SColor c = MapBlock_LightColor(content_features(n).vertex_alpha, l);

			// Neighbor liquid levels (key = relative position)
			// Includes current node
			core::map<v3s16, f32> neighbor_levels;
			core::map<v3s16, content_t> neighbor_contents;
			core::map<v3s16, u8> neighbor_flags;
			const u8 neighborflag_top_is_same_liquid = 0x01;
			v3s16 neighbor_dirs[9] = {
				v3s16(0,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(1,0,1),
				v3s16(-1,0,-1),
				v3s16(1,0,-1),
				v3s16(-1,0,1),
			};
			for(u32 i=0; i<9; i++)
			{
				content_t content = CONTENT_AIR;
				float level = -0.5 * BS;
				u8 flags = 0;
				// Check neighbor
				v3s16 p2 = p + neighbor_dirs[i];
				MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				if(n2.getContent() != CONTENT_IGNORE) {
					content = n2.getContent();

					if (n2.getContent() == c_source) {
						p2.Y += 1;
						n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
						if (content_features(n2).liquid_type == LIQUID_NONE) {
							level = 0.5*BS;
						}else{
							level = (-0.5+node_liquid_level) * BS;
						}
						p2.Y -= 1;
					}else if(n2.getContent() == c_flowing) {
						level = (-0.5 + ((float)(n2.param2&LIQUID_LEVEL_MASK)
								+ 0.5) / 8.0 * node_liquid_level) * BS;
					}

					// Check node above neighbor.
					// NOTE: This doesn't get executed if neighbor
					//       doesn't exist
					p2.Y += 1;
					n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
					if(n2.getContent() == c_source ||
							n2.getContent() == c_flowing)
						flags |= neighborflag_top_is_same_liquid;
				}

				neighbor_levels.insert(neighbor_dirs[i], level);
				neighbor_contents.insert(neighbor_dirs[i], content);
				neighbor_flags.insert(neighbor_dirs[i], flags);
			}

			// Corner heights (average between four liquids)
			f32 corner_levels[4];

			v3s16 halfdirs[4] = {
				v3s16(0,0,0),
				v3s16(1,0,0),
				v3s16(1,0,1),
				v3s16(0,0,1),
			};
			for(u32 i=0; i<4; i++)
			{
				v3s16 cornerdir = halfdirs[i];
				float cornerlevel = 0;
				u32 valid_count = 0;
				u32 air_count = 0;
				for(u32 j=0; j<4; j++)
				{
					v3s16 neighbordir = cornerdir - halfdirs[j];
					content_t content = neighbor_contents[neighbordir];
					// If top is liquid, draw starting from top of node
					if(neighbor_flags[neighbordir] &
							neighborflag_top_is_same_liquid)
					{
						cornerlevel = 0.5*BS;
						valid_count = 1;
						break;
					}
					// Source is always the same height
					else if(content == c_source)
					{
						cornerlevel = (-0.5+node_liquid_level)*BS;
						valid_count = 1;
						break;
					}
					// Flowing liquid has level information
					else if(content == c_flowing)
					{
						cornerlevel += neighbor_levels[neighbordir];
						valid_count++;
					}
					else if(content == CONTENT_AIR)
					{
						air_count++;
					}
				}
				if(air_count >= 2)
					cornerlevel = -0.5*BS;
				else if(valid_count > 0)
					cornerlevel /= valid_count;
				corner_levels[i] = cornerlevel;
			}

			/*
				Generate sides
			*/

			v3s16 side_dirs[4] = {
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
			};
			s16 side_corners[4][2] = {
				{1, 2},
				{3, 0},
				{2, 3},
				{0, 1},
			};
			for(u32 i=0; i<4; i++)
			{
				v3s16 dir = side_dirs[i];

				/*
					If our topside is liquid and neighbor's topside
					is liquid, don't draw side face
				*/
				if(top_is_same_liquid &&
						neighbor_flags[dir] & neighborflag_top_is_same_liquid)
					continue;

				content_t neighbor_content = neighbor_contents[dir];
				ContentFeatures &n_feat = content_features(neighbor_content);

				// Don't draw face if neighbor is blocking the view
				if(n_feat.solidness == 2)
					continue;

				bool neighbor_is_same_liquid = (neighbor_content == c_source
						|| neighbor_content == c_flowing);

				// Don't draw any faces if neighbor same is liquid and top is
				// same liquid
				if(neighbor_is_same_liquid == true
						&& top_is_same_liquid == false)
					continue;

				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[i].texture.x0(), content_features(n).tiles[i].texture.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[i].texture.x1(), content_features(n).tiles[i].texture.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[i].texture.x1(), content_features(n).tiles[i].texture.y0()),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[i].texture.x0(), content_features(n).tiles[i].texture.y0()),
				};

				/*
					If our topside is liquid, set upper border of face
					at upper border of node
				*/
				if(top_is_same_liquid)
				{
					vertices[2].Pos.Y = 0.5*BS;
					vertices[3].Pos.Y = 0.5*BS;
				}
				/*
					Otherwise upper position of face is corner levels
				*/
				else
				{
					vertices[2].Pos.Y = corner_levels[side_corners[i][0]];
					vertices[3].Pos.Y = corner_levels[side_corners[i][1]];
				}

				/*
					If neighbor is liquid, lower border of face is corner
					liquid levels
				*/
				if(neighbor_is_same_liquid)
				{
					vertices[0].Pos.Y = corner_levels[side_corners[i][1]];
					vertices[1].Pos.Y = corner_levels[side_corners[i][0]];
				}
				/*
					If neighbor is not liquid, lower border of face is
					lower border of node
				*/
				else
				{
					vertices[0].Pos.Y = -0.5*BS;
					vertices[1].Pos.Y = -0.5*BS;
				}

				for(s32 j=0; j<4; j++)
				{
					if(dir == v3s16(0,0,1))
						vertices[j].Pos.rotateXZBy(0);
					if(dir == v3s16(0,0,-1))
						vertices[j].Pos.rotateXZBy(180);
					if(dir == v3s16(-1,0,0))
						vertices[j].Pos.rotateXZBy(90);
					if(dir == v3s16(1,0,-0))
						vertices[j].Pos.rotateXZBy(-90);

					vertices[j].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[i].getMaterial(), vertices, 4, indices, 6);
			}

			/*
				Generate top side, if appropriate
			*/

			if(top_is_same_liquid == false)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c,
							content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y0()),
					video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c,
							content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y0()),
				};

				// This fixes a strange bug
				s32 corner_resolve[4] = {3,2,1,0};

				for(s32 i=0; i<4; i++)
				{
					s32 j = corner_resolve[i];
					vertices[i].Pos.Y += corner_levels[j];
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		/*
			Add water sources to mesh if using new style
		*/
		break;
		case CDT_LIQUID_SOURCE:
		if (new_style_water)
		{
			static const u8 l[6][4] = {
				{0,1,6,7},
				{0,1,2,3},
				{1,2,5,6},
				{2,3,4,5},
				{4,5,6,7},
				{0,3,4,7}
			};
			video::SColor c[8];
			getLights(blockpos_nodes+p,c,data,smooth_lighting,content_features(n).vertex_alpha);

			for(u32 j=0; j<6; j++)
			{
				// Check this neighbor
				v3s16 n2p = blockpos_nodes + p + g_6dirs[j];
				MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
				if (content_features(n2).liquid_type != LIQUID_NONE) {
					if (n2.getContent() == content_features(n).liquid_alternative_flowing)
						continue;
					if (n2.getContent() == content_features(n).liquid_alternative_source)
						continue;
				}else if (g_6dirs[j].Y != 1 && n2.getContent() != CONTENT_AIR && content_features(n2).draw_type == CDT_CUBELIKE) {
					continue;
				}else if (n2.getContent() == CONTENT_IGNORE) {
					continue;
				}

				// The face at Z+
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c[l[j][0]],
						content_features(n).tiles[j].texture.x0(), content_features(n).tiles[j].texture.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c[l[j][1]],
						content_features(n).tiles[j].texture.x1(), content_features(n).tiles[j].texture.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c[l[j][2]],
						content_features(n).tiles[j].texture.x1(), content_features(n).tiles[j].texture.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c[l[j][3]],
						content_features(n).tiles[j].texture.x0(), content_features(n).tiles[j].texture.y0()),
				};

				// Rotations in the g_6dirs format
				switch (j) {
				case 0: // Z+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(0);
					}
					break;
				case 1: // Y+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateYZBy(-90);
					}
					break;
				case 2: // X+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(-90);
					}
					break;
				case 3: // Z-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(180);
					}
					break;
				case 4: // Y-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateYZBy(90);
					}
					break;
				case 5: // X-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(90);
					}
					break;
				default:;
				}

				for (u16 i=0; i<4; i++) {
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
					if (j == 1 || (j != 4 && i<2)) {
						vertices[i].Pos.Y -=0.15*BS;
					}
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[j].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_TORCHLIKE:
		{
			static const f32 txc[24] = {
				0.625,0.125,0.75,0.25,
				0.625,0.625,0.625,0.75,
				0,0,0.125,1,
				0,0,0.125,1,
				0,0,0.125,1,
				0,0,0.125,1
			};
			video::SColor c(255,255,255,255);
			v3f pos = intToFloat(p+blockpos_nodes, BS);
			v3s16 dir = unpackDir(n.param2);
			video::S3DVertex *v;
			video::S3DVertex vertices[3][24] = {
				{ // roof
					// up
					video::S3DVertex(-0.1*BS, 0.5*BS,0., 0,1,0, c, txc[4],txc[5]),
					video::S3DVertex(0.,0.5*BS,0., 0,1,0, c, txc[6],txc[5]),
					video::S3DVertex(0.,0.5*BS,-0.1*BS, 0,1,0, c, txc[6],txc[7]),
					video::S3DVertex(-0.1*BS, 0.5*BS,-0.1*BS, 0,1,0, c, txc[4],txc[7]),
					// down
					video::S3DVertex(0.,-0.1*BS,0., 0,-1,0, c, txc[0],txc[1]),
					video::S3DVertex(0.1*BS,-0.1*BS,0., 0,-1,0, c, txc[2],txc[1]),
					video::S3DVertex(0.1*BS,-0.1*BS,0.1*BS, 0,-1,0, c, txc[2],txc[3]),
					video::S3DVertex(0.,-0.1*BS,0.1*BS, 0,-1,0, c, txc[0],txc[3]),
					// right
					video::S3DVertex(0.,0.5*BS,-0.1*BS, 1,0,0, c, txc[ 8],txc[11]),
					video::S3DVertex(0.,0.5*BS,0., 1,0,0, c, txc[10],txc[11]),
					video::S3DVertex(0.1*BS,-0.1*BS,0.1*BS, 1,0,0, c, txc[10],txc[9]),
					video::S3DVertex(0.1*BS,-0.1*BS,0., 1,0,0, c, txc[ 8],txc[9]),
					// left
					video::S3DVertex(-0.1*BS,0.5*BS,0., -1,0,0, c, txc[12],txc[15]),
					video::S3DVertex(-0.1*BS,0.5*BS,-0.1*BS, -1,0,0, c, txc[14],txc[15]),
					video::S3DVertex(0.,-0.1*BS,0., -1,0,0, c, txc[14],txc[13]),
					video::S3DVertex(0.,-0.1*BS,0.1*BS, -1,0,0, c, txc[12],txc[13]),
					// back
					video::S3DVertex(0.,0.5*BS,0., 0,0,1, c, txc[16],txc[19]),
					video::S3DVertex(-0.1*BS,0.5*BS,0., 0,0,1, c, txc[18],txc[19]),
					video::S3DVertex(0.,-0.1*BS,0.1*BS, 0,0,1, c, txc[18],txc[17]),
					video::S3DVertex(0.1*BS,-0.1*BS,0.1*BS, 0,0,1, c, txc[16],txc[17]),
					// front
					video::S3DVertex(-0.1*BS,0.5*BS,-0.1*BS, 0,0,-1, c, txc[20],txc[23]),
					video::S3DVertex(0.,0.5*BS,-0.1*BS, 0,0,-1, c, txc[22],txc[23]),
					video::S3DVertex(0.1*BS,-0.1*BS,0., 0,0,-1, c, txc[22],txc[21]),
					video::S3DVertex(0.,-0.1*BS,0., 0,0,-1, c, txc[20],txc[21]),
				},{ // floor
					// up
					//video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, c, txc[0],txc[1]),
					video::S3DVertex(-0.05*BS,0.1*BS,0.05*BS, 0,1,0, c, txc[0],txc[1]),
					video::S3DVertex(0.05*BS,0.1*BS,0.05*BS, 0,1,0, c, txc[2],txc[1]),
					video::S3DVertex(0.05*BS,0.1*BS,-0.05*BS, 0,1,0, c, txc[2],txc[3]),
					video::S3DVertex(-0.05*BS,0.1*BS,-0.05*BS, 0,1,0, c, txc[0],txc[3]),
					// down
					video::S3DVertex(-0.05*BS,-0.5*BS,-0.05*BS, 0,-1,0, c, txc[4],txc[5]),
					video::S3DVertex(0.05*BS,-0.5*BS,-0.05*BS, 0,-1,0, c, txc[6],txc[5]),
					video::S3DVertex(0.05*BS,-0.5*BS,0.05*BS, 0,-1,0, c, txc[6],txc[7]),
					video::S3DVertex(-0.05*BS,-0.5*BS,0.05*BS, 0,-1,0, c, txc[4],txc[7]),
					// right
					video::S3DVertex(0.05*BS,0.1*BS,-0.05*BS, 1,0,0, c, txc[ 8],txc[9]),
					video::S3DVertex(0.05*BS,0.1*BS,0.05*BS, 1,0,0, c, txc[10],txc[9]),
					video::S3DVertex(0.05*BS,-0.5*BS,0.05*BS, 1,0,0, c, txc[10],txc[11]),
					video::S3DVertex(0.05*BS,-0.5*BS,-0.05*BS, 1,0,0, c, txc[ 8],txc[11]),
					// left
					video::S3DVertex(-0.05*BS,0.1*BS,0.05*BS, -1,0,0, c, txc[12],txc[13]),
					video::S3DVertex(-0.05*BS,0.1*BS,-0.05*BS, -1,0,0, c, txc[14],txc[13]),
					video::S3DVertex(-0.05*BS,-0.5*BS,-0.05*BS, -1,0,0, c, txc[14],txc[15]),
					video::S3DVertex(-0.05*BS,-0.5*BS,0.05*BS, -1,0,0, c, txc[12],txc[15]),
					// back
					video::S3DVertex(0.05*BS,0.1*BS,0.05*BS, 0,0,1, c, txc[16],txc[17]),
					video::S3DVertex(-0.05*BS,0.1*BS,0.05*BS, 0,0,1, c, txc[18],txc[17]),
					video::S3DVertex(-0.05*BS,-0.5*BS,0.05*BS, 0,0,1, c, txc[18],txc[19]),
					video::S3DVertex(0.05*BS,-0.5*BS,0.05*BS, 0,0,1, c, txc[16],txc[19]),
					// front
					video::S3DVertex(-0.05*BS,0.1*BS,-0.05*BS, 0,0,-1, c, txc[20],txc[21]),
					video::S3DVertex(0.05*BS,0.1*BS,-0.05*BS, 0,0,-1, c, txc[22],txc[21]),
					video::S3DVertex(0.05*BS,-0.5*BS,-0.05*BS, 0,0,-1, c, txc[22],txc[23]),
					video::S3DVertex(-0.05*BS,-0.5*BS,-0.05*BS, 0,0,-1, c, txc[20],txc[23]),
				},{ // wall
					// up
					//video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, c, txc[0],txc[1]),
					video::S3DVertex(-0.05*BS,0.3*BS,0.4*BS, 0,1,0, c, txc[0],txc[1]),
					video::S3DVertex(0.05*BS,0.3*BS,0.4*BS, 0,1,0, c, txc[2],txc[1]),
					video::S3DVertex(0.05*BS,0.3*BS,0.3*BS, 0,1,0, c, txc[2],txc[3]),
					video::S3DVertex(-0.05*BS,0.3*BS,0.3*BS, 0,1,0, c, txc[0],txc[3]),
					// down
					video::S3DVertex(-0.05*BS,-0.3*BS,0.4*BS, 0,-1,0, c, txc[4],txc[5]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.4*BS, 0,-1,0, c, txc[6],txc[5]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.5*BS, 0,-1,0, c, txc[6],txc[7]),
					video::S3DVertex(-0.05*BS,-0.3*BS,0.5*BS, 0,-1,0, c, txc[4],txc[7]),
					// right
					video::S3DVertex(0.05*BS,0.3*BS,0.3*BS, 1,0,0, c, txc[ 8],txc[9]),
					video::S3DVertex(0.05*BS,0.3*BS,0.4*BS, 1,0,0, c, txc[10],txc[9]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.5*BS, 1,0,0, c, txc[10],txc[11]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.4*BS, 1,0,0, c, txc[ 8],txc[11]),
					// left
					video::S3DVertex(-0.05*BS,0.3*BS,0.4*BS, -1,0,0, c, txc[12],txc[13]),
					video::S3DVertex(-0.05*BS,0.3*BS,0.3*BS, -1,0,0, c, txc[14],txc[13]),
					video::S3DVertex(-0.05*BS,-0.3*BS,0.4*BS, -1,0,0, c, txc[14],txc[15]),
					video::S3DVertex(-0.05*BS,-0.3*BS,0.5*BS, -1,0,0, c, txc[12],txc[15]),
					// back
					video::S3DVertex(0.05*BS,0.3*BS,0.4*BS, 0,0,1, c, txc[16],txc[17]),
					video::S3DVertex(-0.05*BS,0.3*BS,0.4*BS, 0,0,1, c, txc[18],txc[17]),
					video::S3DVertex(-0.05*BS,-0.3*BS,0.5*BS, 0,0,1, c, txc[18],txc[19]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.5*BS, 0,0,1, c, txc[16],txc[19]),
					// front
					video::S3DVertex(-0.05*BS,0.3*BS,0.3*BS, 0,0,-1, c, txc[20],txc[21]),
					video::S3DVertex(0.05*BS,0.3*BS,0.3*BS, 0,0,-1, c, txc[22],txc[21]),
					video::S3DVertex(0.05*BS,-0.3*BS,0.4*BS, 0,0,-1, c, txc[22],txc[23]),
					video::S3DVertex(-0.05*BS,-0.3*BS,0.4*BS, 0,0,-1, c, txc[20],txc[23]),
				}
			};

			if (dir.Y == 1) { // roof
				for (s32 i=0; i<24; i++) {
					vertices[0][i].Pos += intToFloat(p + blockpos_nodes, BS);
				}
				v = vertices[0];
			}else if (dir.Y == -1) { // floor
				for (s32 i=0; i<24; i++) {
					vertices[1][i].Pos += intToFloat(p + blockpos_nodes, BS);
				}
				v = vertices[1];
			}else{ // wall
				for (s32 i=0; i<24; i++) {
					if(dir == v3s16(1,0,0))
						vertices[2][i].Pos.rotateXZBy(-90);
					if(dir == v3s16(-1,0,0))
						vertices[2][i].Pos.rotateXZBy(90);
					if(dir == v3s16(0,0,1))
						vertices[2][i].Pos.rotateXZBy(0);
					if(dir == v3s16(0,0,-1))
						vertices[2][i].Pos.rotateXZBy(180);

					vertices[2][i].Pos += intToFloat(p + blockpos_nodes, BS);
				}
				v = vertices[2];
			}

			f32 sx = content_features(n.getContent()).tiles[0].texture.x1()-content_features(n.getContent()).tiles[0].texture.x0();
			f32 sy = content_features(n.getContent()).tiles[0].texture.y1()-content_features(n.getContent()).tiles[0].texture.y0();
			for (s32 j=0; j<24; j++) {
				v[j].TCoords *= v2f(sx,sy);
				v[j].TCoords += v2f(
					content_features(n.getContent()).tiles[0].texture.x0(),
					content_features(n.getContent()).tiles[0].texture.y0()
				);
			}

			u16 indices[] = {0,1,2,2,3,0};

			// Add to mesh collector
			for (s32 j=0; j<24; j+=4) {
				collector.append(content_features(n.getContent()).tiles[0].getMaterial(), &v[j], 4, indices, 6);
			}
		}
		/*
			Signs on walls
		*/
		break;
		case CDT_SIGNLIKE:
		{
			u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c = MapBlock_LightColor(255, l);

			static const f32 txc[24] = {
				0.1,0,0.9,0.1,
				0.1,0,0.9,0.1,
				0.1,0.3,0.9,0.7,
				0.1,0.3,0.9,0.7,
				0.1,0.3,0.9,0.7,
				0.1,0.3,0.9,0.7
			};
			video::S3DVertex vertices[24] = {
				// up
				video::S3DVertex(-0.4*BS,0.2*BS,0.5*BS, 0,1,0, c, txc[0],txc[1]),
				video::S3DVertex(0.4*BS,0.2*BS,0.5*BS, 0,1,0, c, txc[2],txc[1]),
				video::S3DVertex(0.4*BS,0.2*BS,0.45*BS, 0,1,0, c, txc[2],txc[3]),
				video::S3DVertex(-0.4*BS,0.2*BS,0.45*BS, 0,1,0, c, txc[0],txc[3]),
				// down
				video::S3DVertex(-0.4*BS,-0.2*BS,0.45*BS, 0,-1,0, c, txc[4],txc[5]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.45*BS, 0,-1,0, c, txc[6],txc[5]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.5*BS, 0,-1,0, c, txc[6],txc[7]),
				video::S3DVertex(-0.4*BS,-0.2*BS,0.5*BS, 0,-1,0, c, txc[4],txc[7]),
				// right
				video::S3DVertex(0.4*BS,0.2*BS,0.45*BS, 1,0,0, c, txc[ 8],txc[9]),
				video::S3DVertex(0.4*BS,0.2*BS,0.5*BS, 1,0,0, c, txc[10],txc[9]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.5*BS, 1,0,0, c, txc[10],txc[11]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.45*BS, 1,0,0, c, txc[ 8],txc[11]),
				// left
				video::S3DVertex(-0.4*BS,0.2*BS,0.5*BS, -1,0,0, c, txc[12],txc[13]),
				video::S3DVertex(-0.4*BS,0.2*BS,0.45*BS, -1,0,0, c, txc[14],txc[13]),
				video::S3DVertex(-0.4*BS,-0.2*BS,0.45*BS, -1,0,0, c, txc[14],txc[15]),
				video::S3DVertex(-0.4*BS,-0.2*BS,0.5*BS, -1,0,0, c, txc[12],txc[15]),
				// back
				video::S3DVertex(0.4*BS,0.2*BS,0.5*BS, 0,0,1, c, txc[16],txc[17]),
				video::S3DVertex(-0.4*BS,0.2*BS,0.5*BS, 0,0,1, c, txc[18],txc[17]),
				video::S3DVertex(-0.4*BS,-0.2*BS,0.5*BS, 0,0,1, c, txc[18],txc[19]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.5*BS, 0,0,1, c, txc[16],txc[19]),
				// front
				video::S3DVertex(-0.4*BS,0.2*BS,0.45*BS, 0,0,-1, c, txc[20],txc[21]),
				video::S3DVertex(0.4*BS,0.2*BS,0.45*BS, 0,0,-1, c, txc[22],txc[21]),
				video::S3DVertex(0.4*BS,-0.2*BS,0.45*BS, 0,0,-1, c, txc[22],txc[23]),
				video::S3DVertex(-0.4*BS,-0.2*BS,0.45*BS, 0,0,-1, c, txc[20],txc[23]),
			};

			f32 sx = content_features(n).tiles[0].texture.x1()-content_features(n).tiles[0].texture.x0();
			f32 sy = content_features(n).tiles[0].texture.y1()-content_features(n).tiles[0].texture.y0();
			for (s32 j=0; j<24; j++) {
				vertices[j].TCoords *= v2f(sx,sy);
				vertices[j].TCoords += v2f(content_features(n).tiles[0].texture.x0(),content_features(n).tiles[0].texture.y0());
			}
			v3s16 dir = unpackDir(n.param2);

			for (s32 i=0; i<24; i++) {
				if(dir == v3s16(1,0,0))
					vertices[i].Pos.rotateXZBy(-90);
				if(dir == v3s16(-1,0,0))
					vertices[i].Pos.rotateXZBy(90);
				if(dir == v3s16(0,0,1))
					vertices[i].Pos.rotateXZBy(0);
				if(dir == v3s16(0,0,-1))
					vertices[i].Pos.rotateXZBy(180);
				if(dir == v3s16(0,-1,0))
					vertices[i].Pos.rotateYZBy(90);
				if(dir == v3s16(0,1,0))
					vertices[i].Pos.rotateYZBy(-90);

				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			for (s32 j=0; j<24; j+=4) {
				collector.append(content_features(n).tiles[0].getMaterial(), &vertices[j], 4, indices, 6);
			}
		}
		/*
			Signs on walls
		*/
		break;
		case CDT_WALLMOUNT:
		{
			// Set material
			video::SMaterial material = content_features(n).tiles[0].getMaterial();
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, false);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_FOG_ENABLE, true);
			material.MaterialType
					= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			//material.setTexture(0, pa_current->atlas);

			u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c = MapBlock_LightColor(255, l);

			float d = (float)BS/16;
			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(BS/2-d,-BS/2,-BS/2, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
				video::S3DVertex(BS/2-d,-BS/2,BS/2, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
				video::S3DVertex(BS/2-d,BS/2,BS/2, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y0()),
				video::S3DVertex(BS/2-d,BS/2,-BS/2, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y0()),
			};

			v3s16 dir = unpackDir(n.param2);

			for(s32 i=0; i<4; i++)
			{
				if(dir == v3s16(1,0,0))
					vertices[i].Pos.rotateXZBy(0);
				if(dir == v3s16(-1,0,0))
					vertices[i].Pos.rotateXZBy(180);
				if(dir == v3s16(0,0,1))
					vertices[i].Pos.rotateXZBy(90);
				if(dir == v3s16(0,0,-1))
					vertices[i].Pos.rotateXZBy(-90);
				if(dir == v3s16(0,-1,0))
					vertices[i].Pos.rotateXYBy(-90);
				if(dir == v3s16(0,1,0))
					vertices[i].Pos.rotateXYBy(90);

				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
		}
		/*
			Add leaves if using new style
		*/
		/*
			Add glass
		*/
		break;
		case CDT_GLASSLIKE:
		{
			static const u8 l[6][4] = {
				{0,1,6,7},
				{0,1,2,3},
				{1,2,5,6},
				{2,3,4,5},
				{4,5,6,7},
				{0,3,4,7}
			};
			video::SColor c[8];
			getLights(blockpos_nodes+p,c,data,smooth_lighting);

			for(u32 j=0; j<6; j++)
			{
				// Check this neighbor
				v3s16 n2p = blockpos_nodes + p + g_6dirs[j];
				MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);

				// The face at Z+
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c[l[j][0]],
						content_features(n).tiles[j].texture.x0(), content_features(n).tiles[j].texture.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c[l[j][1]],
						content_features(n).tiles[j].texture.x1(), content_features(n).tiles[j].texture.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c[l[j][2]],
						content_features(n).tiles[j].texture.x1(), content_features(n).tiles[j].texture.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c[l[j][3]],
						content_features(n).tiles[j].texture.x0(), content_features(n).tiles[j].texture.y0()),
				};

				// Rotations in the g_6dirs format
				switch (j) {
				case 0: // Z+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(0);
					}
					break;
				case 1: // Y+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateYZBy(-90);
					}
					break;
				case 2: // X+
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(-90);
					}
					break;
				case 3: // Z-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(180);
					}
					break;
				case 4: // Y-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateYZBy(90);
					}
					break;
				case 5: // X-
					for(u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(90);
					}
					break;
				default:;
				}

				for (u16 i=0; i<4; i++) {
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[j].getMaterial(), vertices, 4, indices, 6);
			}
		}
		/*
			Add wall
		*/
		break;
		case CDT_WALLLIKE:
		{
			video::SColor c[8];
			getLights(blockpos_nodes+p,c,data,smooth_lighting);

			const f32 post_rad=(f32)0.2*BS;
			const f32 wall_rad=(f32)0.15*BS;
			u8 d[8];
			u8 h[8];
			bool post;
			float height;
			mapblock_mesh_check_walllike(data, blockpos_nodes+p,d,h,&post);

			v3f pos = intToFloat(p+blockpos_nodes, BS);
			if (d[0]) {
				height = h[0] ? 0.501 : 0.301;
				aabb3f bar(
					0.,-(0.499*BS),-wall_rad,
					0.5*BS,height*BS,wall_rad
				);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 xrailuv[24]={
					0.,0.35,0.35,0.65,
					0.,0.35,0.35,0.65,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1,
					0.,0.2,0.5,1,
					0.,0.2,0.5,1
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
			}

			// Now a section of fence, -X, if there's a post there
			if (d[1]) {
				height = h[1] ? 0.501 : 0.301;
				aabb3f bar(
					-0.5*BS,-(0.499*BS),-wall_rad,
					0.,height*BS,wall_rad
				);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 xrailuv[24]={
					0.65,0.35,1,0.65,
					0.65,0.35,1,0.65,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1,
					0.5,0.2,1,1,
					0.5,0.2,1,1
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			if (d[2]) {
				height = h[2] ? 0.501 : 0.301;
				aabb3f bar(
					-wall_rad,-(0.499*BS),0.,
					wall_rad,height*BS,0.5*BS
				);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 zrailuv[24]={
					0.5,0.35,1,0.65,
					0.5,0.35,1,0.65,
					0.5,0.2,1,1,
					0.5,0.2,1,1,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			if (d[3]) {
				height = h[3] ? 0.501 : 0.301;
				aabb3f bar(
					-wall_rad,-(0.499*BS),-0.5*BS,
					wall_rad,height*BS,0.
				);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 zrailuv[24]={
					0,0.35,0.5,0.65,
					0,0.35,0.5,0.65,
					0,0.2,0.5,1,
					0,0.2,0.5,1,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
			}
			if (d[4]) {
				height = h[4] ? 0.501 : 0.301;
				aabb3f bar(
					0.,-(0.499*BS),-wall_rad,
					0.707*BS,height*BS,wall_rad
				);
				f32 xrailuv[24]={
					0.,0.35,0.35,0.65,
					0.,0.35,0.35,0.65,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1,
					0.,0.2,0.5,1,
					0.,0.2,0.5,1
				};
				makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, xrailuv, 45);
			}
			if (d[5]) {
				height = h[5] ? 0.501 : 0.301;
				aabb3f bar(
					-0.707*BS,-(0.499*BS),-wall_rad,
					0.,height*BS,wall_rad
				);
				f32 xrailuv[24]={
					0.65,0.35,1,0.65,
					0.65,0.35,1,0.65,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1,
					0.5,0.2,1,1,
					0.5,0.2,1,1
				};
				makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, xrailuv, 135);
			}
			if (d[6]) {
				height = h[6] ? 0.501 : 0.301;
				aabb3f bar(
					-wall_rad,-(0.499*BS),0.,
					wall_rad,height*BS,0.707*BS
				);
				f32 zrailuv[24]={
					0.5,0.35,1,0.65,
					0.5,0.35,1,0.65,
					0.5,0.2,1,1,
					0.5,0.2,1,1,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1
				};
				makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 45);
			}
			if (d[7]) {
				height = h[7] ? 0.501 : 0.301;
				aabb3f bar(
					-wall_rad,-(0.499*BS),-0.707*BS,
					wall_rad,height*BS,0.
				);
				f32 zrailuv[24]={
					0,0.35,0.5,0.65,
					0,0.35,0.5,0.65,
					0,0.2,0.5,1,
					0,0.2,0.5,1,
					0.35,0.2,0.65,1,
					0.35,0.2,0.65,1
				};
				makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 315);
			}
			if (post) {
				aabb3f post_v(-post_rad,-BS/2,-post_rad,post_rad,BS/2,post_rad);
				post_v.MinEdge += pos;
				post_v.MaxEdge += pos;
				f32 postuv[24]={
					0.3,0.3,0.7,0.7,
					0.3,0.3,0.7,0.7,
					0.3,0,0.7,1,
					0.3,0,0.7,1,
					0.3,0,0.7,1,
					0.3,0,0.7,1
				};
				makeCuboid(&collector, post_v, content_features(n).tiles, 6,  c, postuv);
			}
		}
		/*
			Add fence
		*/
		break;
		case CDT_FENCELIKE:
		{
			video::SColor c[8];
			getLights(blockpos_nodes+p,c,data,smooth_lighting);

			const f32 post_rad=(f32)BS/10;
			const f32 bar_rad=(f32)BS/20;

			// The post - always present
			v3f pos = intToFloat(p+blockpos_nodes, BS);

			// The post - always present
			aabb3f post(-post_rad,-BS/2,-post_rad,post_rad,BS/2,post_rad);
			post.MinEdge += pos;
			post.MaxEdge += pos;
			f32 postuv[24]={
				0.4,0.4,0.6,0.6,
				0.4,0.4,0.6,0.6,
				0.35,0,0.65,1,
				0.35,0,0.65,1,
				0.35,0,0.65,1,
				0.35,0,0.65,1
			};
			makeCuboid(&collector, post, content_features(n).tiles, 6,  c, postuv);

			bool fence_plus_x = false;
			bool fence_minus_x = false;
			bool fence_plus_z = false;
			bool fence_minus_z = false;

			// Now a section of fence, +X, if there's a post there
			v3s16 p2 = p;
			p2.X++;
			MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			const ContentFeatures *f2 = &content_features(n2);
			if (
				f2->draw_type == CDT_FENCELIKE
				|| f2->draw_type == CDT_WALLLIKE
				|| n2.getContent() == CONTENT_WOOD_GATE
				|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
				|| n2.getContent() == CONTENT_STEEL_GATE
				|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
			) {
				fence_plus_x = true;
				aabb3f bar(post_rad,-bar_rad+BS/4,-bar_rad,
						0.5*BS,bar_rad+BS/4,bar_rad);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 xrailuv[24]={
					0.5,0.4,1,0.6,
					0.5,0.4,1,0.6,
					0.5,0.4,1,0.6,
					0.5,0.4,1,0.6,
					0.5,0.4,1,0.6,
					0.5,0.4,1,0.6
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			p2 = p;
			p2.X--;
			n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			f2 = &content_features(n2);
			if (
				f2->draw_type == CDT_FENCELIKE
				|| f2->draw_type == CDT_WALLLIKE
				|| n2.getContent() == CONTENT_WOOD_GATE
				|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
				|| n2.getContent() == CONTENT_STEEL_GATE
				|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
			) {
				fence_minus_x = true;
				aabb3f bar(-0.5*BS,-bar_rad+BS/4,-bar_rad,
						-post_rad,bar_rad+BS/4,bar_rad);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 xrailuv[24]={
					0,0.4,0.5,0.6,
					0,0.4,0.5,0.6,
					0,0.4,0.5,0.6,
					0,0.4,0.5,0.6,
					0,0.4,0.5,0.6,
					0,0.4,0.5,0.6
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, xrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			p2 = p;
			p2.Z++;
			n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			f2 = &content_features(n2);
			if (
				f2->draw_type == CDT_FENCELIKE
				|| f2->draw_type == CDT_WALLLIKE
				|| n2.getContent() == CONTENT_WOOD_GATE
				|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
				|| n2.getContent() == CONTENT_STEEL_GATE
				|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
			) {
				fence_plus_z = true;
				aabb3f bar(-bar_rad,-bar_rad+BS/4,post_rad,
						bar_rad,bar_rad+BS/4,0.5*BS);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 zrailuv[24]={
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			p2 = p;
			p2.Z--;
			n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			f2 = &content_features(n2);
			if (
				f2->draw_type == CDT_FENCELIKE
				|| f2->draw_type == CDT_WALLLIKE
				|| n2.getContent() == CONTENT_WOOD_GATE
				|| n2.getContent() == CONTENT_WOOD_GATE_OPEN
				|| n2.getContent() == CONTENT_STEEL_GATE
				|| n2.getContent() == CONTENT_STEEL_GATE_OPEN
			) {
				fence_minus_z = true;
				aabb3f bar(-bar_rad,-bar_rad+BS/4,-0.5*BS,
						bar_rad,bar_rad+BS/4,-post_rad);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 zrailuv[24]={
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6
				};
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, content_features(n).tiles, 6,  c, zrailuv);
			}
			if (!fence_plus_x && !fence_plus_z) {
				p2 = p;
				p2.X++;
				p2.Z++;
				n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				f2 = &content_features(n2);
				if (
					f2->draw_type == CDT_FENCELIKE
					|| f2->draw_type == CDT_WALLLIKE
				) {
					aabb3f bar(-bar_rad,-bar_rad+BS/4,-0.707*BS,
							bar_rad,bar_rad+BS/4,-post_rad);
					f32 zrailuv[24]={
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6
					};
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 135);
					bar.MinEdge.Y -= BS/2;
					bar.MaxEdge.Y -= BS/2;
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 135);
				}
			}
			if (!fence_plus_x && !fence_minus_z) {
				p2 = p;
				p2.X++;
				p2.Z--;
				n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				f2 = &content_features(n2);
				if (
					f2->draw_type == CDT_FENCELIKE
					|| f2->draw_type == CDT_WALLLIKE
				) {
					aabb3f bar(-bar_rad,-bar_rad+BS/4,-0.707*BS,
							bar_rad,bar_rad+BS/4,-post_rad);
					f32 zrailuv[24]={
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6
					};
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 45);
					bar.MinEdge.Y -= BS/2;
					bar.MaxEdge.Y -= BS/2;
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 45);
				}
			}
			if (!fence_minus_x && !fence_plus_z) {
				p2 = p;
				p2.X--;
				p2.Z++;
				n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				f2 = &content_features(n2);
				if (
					f2->draw_type == CDT_FENCELIKE
					|| f2->draw_type == CDT_WALLLIKE
				) {
					aabb3f bar(-bar_rad,-bar_rad+BS/4,-0.707*BS,
							bar_rad,bar_rad+BS/4,-post_rad);
					f32 zrailuv[24]={
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6
					};
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 225);
					bar.MinEdge.Y -= BS/2;
					bar.MaxEdge.Y -= BS/2;
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 225);
				}
			}
			if (!fence_minus_x && !fence_minus_z) {
				p2 = p;
				p2.X--;
				p2.Z--;
				n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				f2 = &content_features(n2);
				if (
					f2->draw_type == CDT_FENCELIKE
					|| f2->draw_type == CDT_WALLLIKE
				) {
					aabb3f bar(-bar_rad,-bar_rad+BS/4,-0.707*BS,
							bar_rad,bar_rad+BS/4,-post_rad);
					f32 zrailuv[24]={
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6,
						0,0.4,1,0.6
					};
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 315);
					bar.MinEdge.Y -= BS/2;
					bar.MaxEdge.Y -= BS/2;
					makeAngledCuboid(&collector, pos, bar, content_features(n).tiles, 6,  c, zrailuv, 315);
				}
			}

		}
		break;
		case CDT_RAILLIKE:
		{
			bool is_rail_x [] = { false, false };  /* x-1, x+1 */
			bool is_rail_z [] = { false, false };  /* z-1, z+1 */

			bool is_rail_z_minus_y [] = { false, false };  /* z-1, z+1; y-1 */
			bool is_rail_x_minus_y [] = { false, false };  /* x-1, z+1; y-1 */
			bool is_rail_z_plus_y [] = { false, false };  /* z-1, z+1; y+1 */
			bool is_rail_x_plus_y [] = { false, false };  /* x-1, x+1; y+1 */

			MapNode n_minus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1,y,z));
			MapNode n_plus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1,y,z));
			MapNode n_minus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z-1));
			MapNode n_plus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z+1));
			MapNode n_plus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y+1, z));
			MapNode n_plus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y-1, z));
			MapNode n_minus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y+1, z));
			MapNode n_minus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y-1, z));
			MapNode n_plus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z+1));
			MapNode n_minus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z-1));
			MapNode n_plus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z+1));
			MapNode n_minus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z-1));

			content_t thiscontent = n.getContent();

			if (n_minus_x.getContent() == thiscontent)
				is_rail_x[0] = true;
			if (n_minus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[0] = true;
			if (n_minus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[0] = true;
			if (n_plus_x.getContent() == thiscontent)
				is_rail_x[1] = true;
			if (n_plus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[1] = true;
			if (n_plus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[1] = true;
			if (n_minus_z.getContent() == thiscontent)
				is_rail_z[0] = true;
			if (n_minus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[0] = true;
			if (n_minus_z_plus_y.getContent() == thiscontent)
				is_rail_z_plus_y[0] = true;
			if (n_plus_z.getContent() == thiscontent)
				is_rail_z[1] = true;
			if (n_plus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[1] = true;
			if (n_plus_z_plus_y.getContent() == thiscontent)
				is_rail_z_plus_y[1] = true;

			bool is_rail_x_all[] = {false, false};
			bool is_rail_z_all[] = {false, false};
			is_rail_x_all[0] = is_rail_x[0] || is_rail_x_minus_y[0] || is_rail_x_plus_y[0];
			is_rail_x_all[1] = is_rail_x[1] || is_rail_x_minus_y[1] || is_rail_x_plus_y[1];
			is_rail_z_all[0] = is_rail_z[0] || is_rail_z_minus_y[0] || is_rail_z_plus_y[0];
			is_rail_z_all[1] = is_rail_z[1] || is_rail_z_minus_y[1] || is_rail_z_plus_y[1];

			// reasonable default, flat straight unrotated rail
			bool is_straight = true;
			int adjacencies = 0;
			int angle = 0;
			u8 tileindex = 0;

			// check for sloped rail
			if (is_rail_x_plus_y[0] || is_rail_x_plus_y[1] || is_rail_z_plus_y[0] || is_rail_z_plus_y[1]) {
				adjacencies = 5; //5 means sloped
				is_straight = true; // sloped is always straight
			}else{
				// is really straight, rails on both sides
				is_straight = (is_rail_x_all[0] && is_rail_x_all[1]) || (is_rail_z_all[0] && is_rail_z_all[1]);
				adjacencies = is_rail_x_all[0] + is_rail_x_all[1] + is_rail_z_all[0] + is_rail_z_all[1];
			}

			switch (adjacencies) {
			case 1:
				if(is_rail_x_all[0] || is_rail_x_all[1])
					angle = 90;
				break;
			case 2:
				if(!is_straight)
					tileindex = 1; // curved
				if(is_rail_x_all[0] && is_rail_x_all[1])
					angle = 90;
				if(is_rail_z_all[0] && is_rail_z_all[1]){
					if (is_rail_z_plus_y[0])
						angle = 180;
				}
				else if(is_rail_x_all[0] && is_rail_z_all[0])
					angle = 270;
				else if(is_rail_x_all[0] && is_rail_z_all[1])
					angle = 180;
				else if(is_rail_x_all[1] && is_rail_z_all[1])
					angle = 90;
				break;
			case 3:
				// here is where the potential to 'switch' a junction is, but not implemented at present
				tileindex = 2; // t-junction
				if(!is_rail_x_all[1])
					angle=180;
				if(!is_rail_z_all[0])
					angle=90;
				if(!is_rail_z_all[1])
					angle=270;
				break;
			case 4:
				tileindex = 3; // crossing
				break;
			case 5: //sloped
				if(is_rail_z_plus_y[0])
					angle = 180;
				if(is_rail_x_plus_y[0])
					angle = 90;
				if(is_rail_x_plus_y[1])
					angle = -90;
				break;
			default:
				break;
			}

			TileSpec tile = content_features(thiscontent).tiles[tileindex];

			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);

			float d = (float)BS/32;

			char g=-1;
			if (is_rail_x_plus_y[0] || is_rail_x_plus_y[1] || is_rail_z_plus_y[0] || is_rail_z_plus_y[1])
				g=1; //Object is at a slope

			video::S3DVertex vertices[4] = {
				video::S3DVertex(-BS/2,-BS/2+d,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
				video::S3DVertex(BS/2,-BS/2+d,-BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
				video::S3DVertex(BS/2,g*BS/2+d,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
				video::S3DVertex(-BS/2,g*BS/2+d,BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
			};

			for (s32 i=0; i<4; i++) {
				if (angle != 0)
					vertices[i].Pos.rotateXZBy(angle);
				vertices[i].Pos += intToFloat(blockpos_nodes + p, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			collector.append(tile.getMaterial(), vertices, 4, indices, 6);
		}
		break;
		case CDT_ROOFLIKE:
		{
			bool is_roof_x [] = { false, false };  /* x-1, x+1 */
			bool is_roof_z [] = { false, false };  /* z-1, z+1 */

			bool is_roof_z_minus_y [] = { false, false };  /* z-1, z+1; y-1 */
			bool is_roof_x_minus_y [] = { false, false };  /* x-1, z+1; y-1 */
			bool is_roof_z_plus_y [] = { false, false };  /* z-1, z+1; y+1 */
			bool is_roof_x_plus_y [] = { false, false };  /* x-1, x+1; y+1 */

			MapNode n_minus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1,y,z));
			MapNode n_plus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1,y,z));
			MapNode n_minus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z-1));
			MapNode n_plus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z+1));
			MapNode n_plus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y+1, z));
			MapNode n_plus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y-1, z));
			MapNode n_minus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y+1, z));
			MapNode n_minus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y-1, z));
			MapNode n_plus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z+1));
			MapNode n_minus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z-1));
			MapNode n_plus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z+1));
			MapNode n_minus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z-1));

			content_t thiscontent = n.getContent();

			if (n_minus_x.getContent() == thiscontent)
				is_roof_x[0] = true;
			if (n_minus_x_minus_y.getContent() == thiscontent)
				is_roof_x_minus_y[0] = true;
			if (n_minus_x_plus_y.getContent() == thiscontent)
				is_roof_x_plus_y[0] = true;
			if (n_plus_x.getContent() == thiscontent)
				is_roof_x[1] = true;
			if (n_plus_x_minus_y.getContent() == thiscontent)
				is_roof_x_minus_y[1] = true;
			if (n_plus_x_plus_y.getContent() == thiscontent)
				is_roof_x_plus_y[1] = true;
			if (n_minus_z.getContent() == thiscontent)
				is_roof_z[0] = true;
			if (n_minus_z_minus_y.getContent() == thiscontent)
				is_roof_z_minus_y[0] = true;
			if (n_minus_z_plus_y.getContent() == thiscontent)
				is_roof_z_plus_y[0] = true;
			if (n_plus_z.getContent() == thiscontent)
				is_roof_z[1] = true;
			if (n_plus_z_minus_y.getContent() == thiscontent)
				is_roof_z_minus_y[1] = true;
			if (n_plus_z_plus_y.getContent() == thiscontent)
				is_roof_z_plus_y[1] = true;

			bool is_roof_x_all[] = {false, false};
			bool is_roof_z_all[] = {false, false};
			is_roof_x_all[0] = is_roof_x[0] || is_roof_x_minus_y[0] || is_roof_x_plus_y[0];
			is_roof_x_all[1] = is_roof_x[1] || is_roof_x_minus_y[1] || is_roof_x_plus_y[1];
			is_roof_z_all[0] = is_roof_z[0] || is_roof_z_minus_y[0] || is_roof_z_plus_y[0];
			is_roof_z_all[1] = is_roof_z[1] || is_roof_z_minus_y[1] || is_roof_z_plus_y[1];

			u8 adjacencies = is_roof_x[0] + is_roof_x[1] + is_roof_z[0] + is_roof_z[1];

			TileSpec tile = content_features(thiscontent).tiles[0];
			u8 l = decode_light(
				undiminish_light(
					data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z)).getLightBlend(data->m_daynight_ratio)
				)
			);
			video::SColor c = MapBlock_LightColor(255, l);

			u8 type = 0;
			s16 angle = 0;

			MapNode abv;

			if (adjacencies == 1) {
				// cross X
				if (is_roof_x[0] || is_roof_x[1]) {
					if (is_roof_z_plus_y[0]) {
						type = 0;
						angle = 180;
					}else if (is_roof_z_plus_y[1]) {
						type = 0;
						angle = 0;
					}else{
						type = 1;
						angle = 0;
					}
				}
				// cross Z
				else if (is_roof_z[0] || is_roof_z[1]) {
					if (is_roof_x_plus_y[1]) {
						type = 0;
						angle = 270;
					}else if (is_roof_x_plus_y[0]) {
						type = 0;
						angle = 90;
					}else{
						type = 1;
						angle = 90;
					}
				}
			}else if (adjacencies == 2) {
				// cross X
				if (is_roof_x[0] && is_roof_x[1]) {
					if (is_roof_z_plus_y[0]) {
						type = 0;
						angle = 180;
					}else if (is_roof_z_plus_y[1]) {
						type = 0;
						angle = 0;
					}else{
						type = 1;
						angle = 0;
					}
				}
				// cross Z
				else if (is_roof_z[0] && is_roof_z[1]) {
					if (is_roof_x_plus_y[1]) {
						type = 0;
						angle = 270;
					}else if (is_roof_x_plus_y[0]) {
						type = 0;
						angle = 90;
					}else{
						type = 1;
						angle = 90;
					}
				}else if (is_roof_x[0] && is_roof_z[0]) {
					if (is_roof_x_plus_y[1] && is_roof_z_plus_y[1]) {
						type = 7;
						angle = 90;
					}else if (is_roof_x_plus_y[1]) {
						type = 2;
						angle = 0;
					}else if (is_roof_z_plus_y[1]) {
						type = 2;
						angle = 90;
					}else{
						abv = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y+1, z-1));
						if (abv.getContent() == thiscontent) {
							type = 4;
							angle = 90;
						}else{
							type = 3;
							angle = 0;
						}
					}
				}else if (is_roof_x[0] && is_roof_z[1]) {
					if (is_roof_x_plus_y[1] && is_roof_z_plus_y[0]) {
						type = 7;
						angle = 0;
					}else if (is_roof_x_plus_y[1]) {
						type = 2;
						angle = 0;
					}else if (is_roof_z_plus_y[0]) {
						type = 2;
						angle = 270;
					}else{
						abv = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y+1, z+1));
						if (abv.getContent() == thiscontent) {
							type = 4;
							angle = 0;
						}else{
							type = 3;
							angle = 270;
						}
					}
				}else if (is_roof_x[1] && is_roof_z[0]) {
					if (is_roof_x_plus_y[0] && is_roof_z_plus_y[1]) {
						type = 7;
						angle = 180;
					}else if (is_roof_x_plus_y[0]) {
						type = 2;
						angle = 180;
					}else if (is_roof_z_plus_y[1]) {
						type = 2;
						angle = 90;
					}else{
						abv = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y+1, z-1));
						if (abv.getContent() == thiscontent) {
							type = 4;
							angle = 180;
						}else{
							type = 3;
							angle = 90;
						}
					}
				}else if (is_roof_x[1] && is_roof_z[1]) {
					if (is_roof_x_plus_y[0] && is_roof_z_plus_y[0]) {
						type = 7;
						angle = 270;
					}else if (is_roof_x_plus_y[0]) {
						type = 2;
						angle = 180;
					}else if (is_roof_z_plus_y[0]) {
						type = 2;
						angle = 270;
					}else{
						abv = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y+1, z+1));
						if (abv.getContent() == thiscontent) {
							type = 4;
							angle = 270;
						}else{
							type = 3;
							angle = 180;
						}
					}
				}
			}else if (adjacencies == 3) {
				if (is_roof_x[0] && is_roof_x[1] && is_roof_z[0]) {
					if (is_roof_z_plus_y[1]) {
						type = 2;
						angle = 90;
					}else{
						type = 6;
						angle = 0;
					}
				}else if (is_roof_x[0] && is_roof_x[1] && is_roof_z[1]) {
					if (is_roof_z_plus_y[0]) {
						type = 2;
						angle = 270;
					}else{
						type = 6;
						angle = 180;
					}
				}else if (is_roof_x[0] && is_roof_z[0] && is_roof_z[1]) {
					if (is_roof_x_plus_y[1]) {
						type = 2;
						angle = 0;
					}else{
						type = 6;
						angle = 270;
					}
				}else if (is_roof_x[1] && is_roof_z[0] && is_roof_z[1]) {
					if (is_roof_x_plus_y[0]) {
						type = 2;
						angle = 180;
					}else{
						type = 6;
						angle = 90;
					}
				}
			}else if (adjacencies == 4) {
				type = 5;
				angle = 0;
			}else{
				if (is_roof_z_plus_y[0]) {
					type = 0;
					angle = 180;
				}else if (is_roof_z_plus_y[1]) {
					type = 0;
				}else if (is_roof_x_plus_y[1]) {
					type = 0;
					angle = 270;
				}else if (is_roof_x_plus_y[0]) {
					type = 0;
					angle = 90;
				}else{
					type = 0;
					angle = 0;
				}
			}
			/*
				0: slope
				1: top
				2: top butting to slope
				3: top corner
				4: outer corner
				5: top X
				6: top T
				7: inner corner
			*/
			switch (type) {
			case 0:
			{
				video::S3DVertex slope_v[4] = {
					video::S3DVertex(-BS/2,-BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					video::S3DVertex(BS/2,-BS/2,-BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
				};
				for (s32 i=0; i<4; i++) {
					if (angle != 0)
						slope_v[i].Pos.rotateXZBy(angle);
					slope_v[i].Pos += intToFloat(blockpos_nodes + p, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				collector.append(tile.getMaterial(), slope_v, 4, indices, 6);
			}
			break;
			case 1:
			{
				// TODO: tex coords for half height
				video::S3DVertex top_v[2][4] = {
					{
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, -BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					},{
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2,  0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					}
				};
				for (s32 s=0; s<2; s++) {
					for (s32 i=0; i<4; i++) {
						if (angle != 0)
							top_v[s][i].Pos.rotateXZBy(angle);
						top_v[s][i].Pos += intToFloat(blockpos_nodes + p, BS);
					}

					u16 indices[] = {0,1,2,2,3,0};
					collector.append(tile.getMaterial(), top_v[s], 4, indices, 6);
				}
			}
			break;
			case 2:
			{
				// TODO: tex coords for half height
				video::S3DVertex butt_v[3][4] = {
					{
						video::S3DVertex(-BS/2,-BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(BS/2,BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
					}
				};
				s16 k = 6;
				for (s32 s=0; s<3; s++) {
					for (s32 i=0; i<4; i++) {
						if (angle != 0)
							butt_v[s][i].Pos.rotateXZBy(angle);
						butt_v[s][i].Pos += intToFloat(blockpos_nodes + p, BS);
					}

					u16 indices[] = {0,1,2,2,3,0};
					collector.append(tile.getMaterial(),butt_v[s], 4, indices, k);
					k = 3;
				}
			}
			break;
			case 3:
			{
				// TODO: tex coords are totally screwed
				video::S3DVertex topc_v[4][4] = {
					{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(0,     0,     -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(0,     0,     -BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(BS/2,  -BS/2, -BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
					}
				};
				u16 indices[4][6] = {
					{0,1,2,0,3,1},
					{0,1,2,0,3,1},
					{0,1,2,0,3,1},
					{0,1,2,0,2,3}
				};
				s16 k = 3;
				for (s32 s=0; s<4; s++) {
					k = s%2 ? 6 : 3;
					for (s32 i=0; i<4; i++) {
						if (angle != 0)
							topc_v[s][i].Pos.rotateXZBy(angle);
						topc_v[s][i].Pos += intToFloat(blockpos_nodes + p, BS);
					}

					collector.append(tile.getMaterial(),topc_v[s], 4, indices[s], k);
				}
			}
			break;
			case 4:
			{
				video::S3DVertex outer_v[4] = {
					video::S3DVertex(-BS/2,-BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					video::S3DVertex(BS/2,-BS/2,-BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
				};
				for (s32 i=0; i<4; i++) {
					if (angle != 0)
						outer_v[i].Pos.rotateXZBy(angle);
					outer_v[i].Pos += intToFloat(blockpos_nodes + p, BS);
				}

				u16 indices[] = {0,1,2,2,1,3};
				collector.append(tile.getMaterial(), outer_v, 4, indices, 6);
			}
			break;
			case 5:
			{
				// TODO: tex coords are totally screwed
				video::S3DVertex topx_v[4][4] = {
					{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(0,     0,     -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					},{
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(0,     0,     BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
					}
				};
				for (s32 s=0; s<4; s++) {
					for (s32 i=0; i<4; i++) {
						topx_v[s][i].Pos += intToFloat(blockpos_nodes + p, BS);
					}

					u16 indices[] = {0,1,2,1,3,0};
					collector.append(tile.getMaterial(),topx_v[s], 4, indices, 6);
				}
			}
			break;
			case 6:
			{
				// TODO: tex coords for half height
				video::S3DVertex topt_v[4][4] = {
					{
						video::S3DVertex(-BS/2, -BS/2, BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					},{
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(BS/2,  -BS/2, -BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
						video::S3DVertex(BS/2,  0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(-BS/2, 0,     0, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					},{
						video::S3DVertex(0,     0,     -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
						video::S3DVertex(-BS/2, -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
						video::S3DVertex(0,     0,     0, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
						video::S3DVertex(BS/2,  -BS/2, -BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y1()),
					}
				};
				for (s32 s=0; s<3; s++) {
					for (s32 i=0; i<4; i++) {
						if (angle != 0)
							topt_v[s][i].Pos.rotateXZBy(angle);
						topt_v[s][i].Pos += intToFloat(blockpos_nodes + p, BS);
					}

					u16 indices[] = {0,1,2,2,3,0};
					collector.append(tile.getMaterial(),topt_v[s], 4, indices, 6);
				}
				break;
			}
			break;
			case 7:
			{
				video::S3DVertex inner_v[6] = {
					video::S3DVertex(BS/2,BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					video::S3DVertex(-BS/2,BS/2,-BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
					video::S3DVertex(BS/2,BS/2,-BS/2, 0,0,0, c, tile.texture.x0(), tile.texture.y0()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y0()),
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, tile.texture.x1(), tile.texture.y1()),
				};
				for (s32 i=0; i<6; i++) {
					if (angle != 0)
						inner_v[i].Pos.rotateXZBy(angle);
					inner_v[i].Pos += intToFloat(blockpos_nodes + p, BS);
				}

				u16 indices[] = {0,1,2,3,4,5};
				collector.append(tile.getMaterial(), inner_v, 6, indices, 6);
			}
			break;
			default:
				break;
			}
		}
		break;
		case CDT_PLANTLIKE_LGE:
		{
			MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + v3s16(0,1,0));
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);
			f32 tuv[4] = {
				content_features(n).tiles[0].texture.x0(),
				content_features(n).tiles[0].texture.x1(),
				content_features(n).tiles[0].texture.y0(),
				content_features(n).tiles[0].texture.y1()
			};
			s32 h = 1;
			f32 s = 1.3;
			if (
				content_features(n2).draw_type == CDT_PLANTLIKE_LGE
				|| content_features(n2).draw_type == CDT_PLANTLIKE
				|| content_features(n2).draw_type == CDT_PLANTLIKE_SML
			) {
				tuv[2] = (0.333*content_features(n).tiles[0].texture.size.Y)+content_features(n).tiles[0].texture.y0();
				h = 2;
				s = 1.0;
			}
			video::S3DVertex base_vertices[4] = {
				base_vertices[0] = video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,tuv[0], tuv[3]),
				base_vertices[1] = video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,tuv[1], tuv[3]),
				base_vertices[2] = video::S3DVertex(BS/2,BS/h,0, 0,0,0, c,tuv[1], tuv[2]),
				base_vertices[3] = video::S3DVertex(-BS/2,BS/h,0, 0,0,0, c,tuv[0], tuv[2])
			};
			for (u32 j=0; j<2; j++) {
				video::S3DVertex vertices[4] = {
					base_vertices[0],
					base_vertices[1],
					base_vertices[2],
					base_vertices[3]
				};

				switch (j) {
				case 0:
					for (u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(45);
					}
					break;
				case 1:
					for (u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(-45);
					}
					break;
				case 2:
					for (u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(135);
					}
					break;
				case 3:
					for (u16 i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(-135);
					}
					break;
				default:;
				}

				for (u16 i=0; i<4; i++) {
					vertices[i].Pos.X *= 1.3;
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_PLANTLIKE:
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);

			for(u32 j=0; j<2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y0()),
					video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y0()),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_PLANTLIKE_SML:
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);

			for(u32 j=0; j<2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y0()),
					video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y0()),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos *= 0.8;
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_PLANTGROWTH_1:
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);
			f32 h = (0.75*content_features(n).tiles[0].texture.size.Y)+content_features(n).tiles[0].texture.y0();

			for(u32 j=0; j<2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-0.5*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-0.5*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-0.25*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), h),
					video::S3DVertex(-BS/2,-0.25*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), h),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_PLANTGROWTH_2:
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);
			f32 h = (0.5*content_features(n).tiles[0].texture.size.Y)+content_features(n).tiles[0].texture.y0();

			for(u32 j=0; j<2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,0,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), h),
					video::S3DVertex(-BS/2,0,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), h),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_PLANTGROWTH_3:
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c = MapBlock_LightColor(255, l);
			f32 h = (0.25*content_features(n).tiles[0].texture.size.Y)+content_features(n).tiles[0].texture.y0();

			for(u32 j=0; j<2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-0.5*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,-0.5*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), content_features(n).tiles[0].texture.y1()),
					video::S3DVertex(BS/2,0.25*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x1(), h),
					video::S3DVertex(-BS/2,0.25*BS,0, 0,0,0, c,
						content_features(n).tiles[0].texture.x0(), h),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(content_features(n).tiles[0].getMaterial(), vertices, 4, indices, 6);
			}
		}
		break;
		case CDT_NODEBOX:
		{
			static const v3s16 tile_dirs[6] = {
				v3s16(0, 1, 0),
				v3s16(0, -1, 0),
				v3s16(1, 0, 0),
				v3s16(-1, 0, 0),
				v3s16(0, 0, 1),
				v3s16(0, 0, -1)
			};

			TileSpec tiles[6];
			for (int i = 0; i < 6; i++) {
				// Handles facedir rotation for textures
				tiles[i] = n.getTile(tile_dirs[i]);
			}
			video::SColor c[8];
			getLights(blockpos_nodes+p,c,data,smooth_lighting);

			v3f pos = intToFloat(p+blockpos_nodes, BS);
			std::vector<aabb3f> boxes = content_features(n).getNodeBoxes(n);
			for (std::vector<aabb3f>::iterator i = boxes.begin(); i != boxes.end(); i++) {
				aabb3f box = *i;
				box.MinEdge += pos;
				box.MaxEdge += pos;

				// Compute texture coords
				f32 tx1 = (i->MinEdge.X/BS)+0.5;
				f32 ty1 = (i->MinEdge.Y/BS)+0.5;
				f32 tz1 = (i->MinEdge.Z/BS)+0.5;
				f32 tx2 = (i->MaxEdge.X/BS)+0.5;
				f32 ty2 = (i->MaxEdge.Y/BS)+0.5;
				f32 tz2 = (i->MaxEdge.Z/BS)+0.5;
				f32 txc[24] = {
					// up
					tx1, 1-tz2, tx2, 1-tz1,
					// down
					tx1, tz1, tx2, tz2,
					// right
					tz1, 1-ty2, tz2, 1-ty1,
					// left
					1-tz2, 1-ty2, 1-tz1, 1-ty1,
					// back
					1-tx2, 1-ty2, 1-tx1, 1-ty1,
					// front
					tx1, 1-ty2, tx2, 1-ty1,
				};
				makeCuboid(&collector, box, tiles, 6,  c, txc);
			}
		}
		break;
		}
	}
}
#endif

