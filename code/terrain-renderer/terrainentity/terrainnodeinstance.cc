//------------------------------------------------------------------------------
//  billboardnodeinstance.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainnodeinstance.h"
#include "terrainnode.h"
#include "models/modelnode.h"
#include "models/nodes/transformnode.h"
#include "models/nodes/statenodeinstance.h"
#include "models/modelnodeinstance.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "resources/resourceloader.h"
#include "coregraphics/renderdevice.h"
#include "graphics/cameraentity.h"
#include "coregraphics/base/vertexcomponentbase.h"
#include "coregraphics/transformdevice.h"

using namespace CoreGraphics;
using namespace Util;
using namespace Resources;
using namespace Models;
namespace Terrain
{
__ImplementClass(Terrain::TerrainNodeInstance, 'TNIE', Models::StateNodeInstance);

//------------------------------------------------------------------------------
/**
*/
TerrainNodeInstance::TerrainNodeInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TerrainNodeInstance::~TerrainNodeInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNodeInstance::Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst)
{
	// up to parent class
	StateNodeInstance::Setup(inst, node, parentNodeInst);

	this->terrain_node = this->modelNode.downcast<TerrainNode>();
	this->geoclipmap_shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:geoclipmaps");
	this->SetupUniformBuffer();
	level_offsets.resize(terrain_node->levels);
	for (unsigned int i = 0; i < 256; i++){
		this->id_offset_list.Append((float)i);
	}
	//list with offsets memcpy
	memcpy(this->terrain_node->mapped_offsets, this->id_offset_list.Begin(), this->id_offset_list.Size() * sizeof(float));
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNodeInstance::Discard()
{
	StateNodeInstance::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNodeInstance::Render()
{
	StateNodeInstance::Render();
	RenderDevice* renderDevice = RenderDevice::Instance();

	Math::float4 camera_position = CoreGraphics::TransformDevice::Instance()->GetInvViewTransform().get_position();
	//float2 camera_pos(camera_position.x(), camera_position.z());
	float2 camera_pos(0.f, 0.f);

	this->culled_counter = 0;
	//get cameras position
	this->update_level_offsets(camera_pos);
	this->update_draw_list();

	n_printf("Culled Blocks %d\n", this->culled_counter);

	//bind vao
	renderDevice->SetStreamVertexBuffer(0, terrain_node->vbo, 0);
	renderDevice->SetStreamVertexBuffer(1, terrain_node->offset_buffer, 0);
	renderDevice->SetVertexLayout(terrain_node->vertexLayout); //own vertexlayout
	renderDevice->SetIndexBuffer(terrain_node->ibo);

	//bind uniform buffer
	this->instance_data_blockvar = this->geoclipmap_shader->GetVariableByName("InstanceData");
	this->instance_data_blockvar->SetBufferHandle(this->uniform_buffer->GetHandle());

	this->render_draw_list();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNodeInstance::SetupUniformBuffer()
{
	this->uniform_buffer = CoreGraphics::ConstantBuffer::Create();
	this->uniform_buffer->SetupFromBlockInShader(this->geoclipmap_shader, "InstanceData", 3);
	this->offset_shdvar = this->uniform_buffer->GetVariableByName("offset");
	this->scale_shdvar = this->uniform_buffer->GetVariableByName("scale");
	this->level_shdvar = this->uniform_buffer->GetVariableByName("level");
}

//! [Snapping clipmap level to a grid]
// The clipmap levels only move in steps of texture coordinates.
// Computes top-left world position for the levels.
float2 TerrainNodeInstance::get_offset_level(const float2& camera_pos, unsigned int level)
{
	if (level == 0) // Must follow level 1 as trim region is fixed.
	{
		int temp_size = terrain_node->size << 1;
		return get_offset_level(camera_pos, 1) + float2((float)temp_size, (float)temp_size);
	}
	else
	{

		float clipmap_scale = terrain_node->clipmap_scale;
		float2 scaled_pos;
		scaled_pos.x() = camera_pos.x() / clipmap_scale; // Snap to grid in the appropriate space.
		scaled_pos.y() = camera_pos.y() / clipmap_scale;


		float2 temp_value((float)(1 << (level + 1)), (float)(1 << (level + 1)));
		float2 vec_floor;
		vec_floor.x() = scaled_pos.x() / temp_value.x();
		vec_floor.y() = scaled_pos.y() / temp_value.y();

		// rounds the value downwards to an integer that is not larger than the original
		vec_floor.x() = floor(vec_floor.x());
		vec_floor.y() = floor(vec_floor.y());

		// Snap to grid of next level. I.e. we move the clipmap level in steps of two. bitwise multi
		float2 snapped_pos = float2::multiply(vec_floor, float2((float)(1 << (level + 1)), (float)(1 << (level + 1))));

		// Apply offset so all levels align up neatly.
		// If snapped_pos is equal for all levels,
		// this causes top-left vertex of level N to always align up perfectly with top-left interior corner of level N + 1.
		// This gives us a bottom-right trim region.

		// Due to the flooring, snapped_pos might not always be equal for all levels.
		// The flooring has the property that snapped_pos for level N + 1 is less-or-equal snapped_pos for level N.
		// If less, the final position of level N + 1 will be offset by -2 ^ N, which can be compensated for with changing trim-region to top-left.
		float2 pos = snapped_pos - float2((float)((2 * (terrain_node->size - 1)) << level), (float)((2 * (terrain_node->size - 1)) << level));
		return pos;
	}
}
//! [Snapping clipmap level to a grid]

void TerrainNodeInstance::update_level_offsets(const float2& camera_pos)
{
	//level_offsets.resize(terrain_node->levels);
	for (int i = 0; i < terrain_node->levels; i++){
		level_offsets[i] = get_offset_level(camera_pos, i);
	}
}

// Since we use instanced drawing, all the different instances of various block types
// can be grouped together to form one draw call per block type.
//
// For the get_draw_info* calls, we look through all possible places where blocks can be rendered
// and push this information to a draw list and a uniform buffer.
//
// The draw list struct (DrawInfo) contains information such as the number of instances for a block type,
// and from where in the uniform buffer to get per-instance data. The per-instance data contains information
// of offset and scale values required to render the blocks at correct positions and at correct scale.
//
// The get_draw_info_* calls are sort of repetitive so comments are only introduced when
// something different is done.
//
// It is important to note that instance.offset is a pre-scaled offset which denotes the
// world-space X/Z position of the top-left vertex in the block.
// instance.scale is used to scale vertex data in a block (which are just integers).
//
// World space X/Z coordinates are computed as instance.offset + vertex_coord * instance.scale.

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_horiz_fixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	InstanceData instance;
	//instance.debug_color = Vector3(0, 1, 0);

	// Horizontal
	info.index_buffer_offset = terrain_node->horizontal.offset;
	info.indices = terrain_node->horizontal.count;
	info.instances = 0;

	// We don't have any fixup regions for the lowest clipmap level.
	for (int i = 1; i < terrain_node->levels; i++)
	{
		// Left side horizontal fixup region.
		// Texel coordinates are derived by just dividing the world space offset with texture size.
		// The 0.5 texel offset required to sample exactly at the texel center is done in vertex shader.
		instance.scale = terrain_node->clipmap_scale * float(1 << i);
		instance.level = (float)i;

		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(0.f, (float)(2 * (terrain_node->size - 1))), float2((float)(1 << i), (float)(1 << i)));
		// Avoid texture coordinates which are very large as this can be difficult for the texture sampler
		// to handle (float precision). Since we use GL_REPEAT, fract() does not change the result.
		// Scale the offset down by 2^level first to get the appropriate texel.
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

		// Only add the instance if it's visible.
		if (this->IntersectsFrustum(instance.offset, terrain_node->horizontal.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}

		// Right side horizontal fixup region.
		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(3.f * (terrain_node->size - 1.f) + 2, 2.f * (terrain_node->size - 1)), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

		// Only add the instance if it's visible.
		if (this->IntersectsFrustum(instance.offset, terrain_node->horizontal.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}
	}

	return info;
}

// Same as horizontal, just different vertex data and offsets.
TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_vert_fixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	InstanceData instance;
	//instance.debug_color = Vector3(0, 1, 0);

	// Vertical
	info.index_buffer_offset = terrain_node->vertical.offset;
	info.indices = terrain_node->vertical.count;
	info.instances = 0;

	for (int i = 1; i < terrain_node->levels; i++)
	{
		// Top region
		instance.scale = terrain_node->clipmap_scale * float(1 << i);
		instance.level = (float)i;

		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(2.f * (terrain_node->size - 1), 0), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

		if (this->IntersectsFrustum(instance.offset, terrain_node->vertical.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}

		// Bottom region
		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(2.f * (terrain_node->size - 1), 3.f * (terrain_node->size - 1) + 2), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

		if (this->IntersectsFrustum(instance.offset, terrain_node->vertical.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}
	}

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_degenerate(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, const float2& offset, const float2& ring_offset)
{
	DrawInfo info;
	info.instances = 0;
	info.index_buffer_offset = block.offset;
	info.indices = block.count;

	InstanceData instance;
	//instance.debug_color = Vector3(0, 0, 1);

	// No need to connect the last clipmap level to next level (there is none).
	for (int i = 0; i < terrain_node->levels - 1; i++)
	{
		instance.level = (float)i;
		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2::multiply(offset, float2((float)(1 << i), (float)(1 << i)));

		// This is required to differentiate between level 0 and the other levels.
		// In clipmap level 0, we only have tightly packed N-by-N blocks.
		// In other levels however, there are horizontal and vertical fixup regions, therefore a different
		// offset (2 extra texels) is required.
		if (i > 0){
			instance.offset = instance.offset + float2::multiply(ring_offset, float2((float)(1 << i), (float)(1 << i)));
		}
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));
		instance.scale = terrain_node->clipmap_scale * float(1 << i);

		if (this->IntersectsFrustum(instance.offset, terrain_node->block.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}
	}

	return info;
}

// Use the generalized get_draw_info_degenerate().
TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_degenerate_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_degenerate(offset_list, scale_list, level_list, terrain_node->degenerate_left, float2(0.0f, 0.0f), float2(0.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_degenerate_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_degenerate(offset_list, scale_list, level_list, terrain_node->degenerate_right, float2(4.f * (terrain_node->size - 1), 0.0f), float2(2.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_degenerate_top(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_degenerate(offset_list, scale_list, level_list, terrain_node->degenerate_top, float2(0.0f, 0.0f), float2(0.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_degenerate_bottom(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_degenerate(offset_list, scale_list, level_list, terrain_node->degenerate_bottom, float2(0.0f, 4.f * (terrain_node->size - 1)), float2(0.0f, 2.0f));
}

// Only used for cliplevel 1 to encapsulate cliplevel 0.
TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim_full(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	info.index_buffer_offset = terrain_node->trim_full.offset;
	info.indices = terrain_node->trim_full.count;
	info.instances = 0;

	InstanceData instance;
	//instance.debug_color = Vector3(1, 1, 0);

	instance.level = 1;
	instance.offset = level_offsets[1];
	instance.offset = instance.offset + float2((float)((terrain_node->size - 1) << 1), (float)((terrain_node->size - 1) << 1));
	instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));
	instance.scale = terrain_node->clipmap_scale * float(1 << 1);

	if (this->IntersectsFrustum(instance.offset, terrain_node->trim_full.range, 1))
	{
		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;
	}

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, TrimConditional cond)
{
	DrawInfo info;
	info.index_buffer_offset = block.offset;
	info.indices = block.count;
	info.instances = 0;

	// Level 1 always fills in the gap to level 0 using get_draw_info_trim_full().
	// From level 2 and out, we only need a single L-shaped trim region as levels 1 and up
	// use horizontal/vertical trim regions as well, which increases the size slightly (get_draw_info_blocks()).
	for (int i = 2; i < terrain_node->levels; i++)
	{
		float2 offset_prev_level = level_offsets[i - 1];
		float2 offset_current_level = level_offsets[i] + float2((float)((terrain_node->size - 1) << i), (float)((terrain_node->size - 1) << i));

		// There are four different ways (top-right, bottom-right, top-left, bottom-left)
		// to apply a trim region depending on how camera snapping is done in get_offset_level().
		// A function pointer is introduced here so we can check if a particular trim type
		// should be used for this level. Only one conditional will return true for a given level.
		if (!cond(offset_prev_level - offset_current_level))
			continue;

		InstanceData instance;
		instance.level = (float)i;
		instance.offset = level_offsets[i];
		instance.offset = instance.offset + float2((float)((terrain_node->size - 1) << i), (float)((terrain_node->size - 1) << i));
		instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));
		instance.scale = terrain_node->clipmap_scale * float(1 << i);

		if (this->IntersectsFrustum(instance.offset, terrain_node->block.range, i))
		{
			offset_list.Append(instance.offset);
			scale_list.Append(instance.scale);
			level_list.Append(instance.level);
			info.instances++;
		}
	}

	return info;
}

// offset.x and offset.y are either 0 or at least 1.
// Using 0.5f as threshold is a safe way to check for this difference.
static inline bool trim_top_right_cond(const float2& offset)
{
	return offset.x() < 0.5f && offset.y() > 0.5f;
}

static inline bool trim_top_left_cond(const float2& offset)
{
	return offset.x() > 0.5f && offset.y() > 0.5f;
}

static inline bool trim_bottom_right_cond(const float2& offset)
{
	return offset.x() < 0.5f && offset.y() < 0.5f;
}

static inline bool trim_bottom_left_cond(const float2& offset)
{
	return offset.x() > 0.5f && offset.y() < 0.5f;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim_top_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_trim(offset_list, scale_list, level_list, terrain_node->trim_top_right, trim_top_right_cond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim_top_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_trim(offset_list, scale_list, level_list, terrain_node->trim_top_left, trim_top_left_cond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim_bottom_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_trim(offset_list, scale_list, level_list, terrain_node->trim_bottom_right, trim_bottom_right_cond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_trim_bottom_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return get_draw_info_trim(offset_list, scale_list, level_list, terrain_node->trim_bottom_left, trim_bottom_left_cond);
}

// These are the basic N-by-N tesselated quads.
TerrainNodeInstance::DrawInfo TerrainNodeInstance::get_draw_info_blocks(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	// Special case for level 0, here we draw the base quad in a tight 4x4 grid. This needs to be padded with a full trim (get_draw_info_trim_full()).
	DrawInfo info;
	info.instances = 0;
	info.index_buffer_offset = terrain_node->block.offset;
	info.indices = terrain_node->block.count;

	InstanceData instance;
	//instance.debug_color = Vector3(1, 0, 0);
	instance.scale = terrain_node->clipmap_scale;

	for (int z = 0; z < 4; z++)
	{
		for (int x = 0; x < 4; x++)
		{
			instance.level = 0;
			instance.offset = level_offsets[0];
			instance.offset = instance.offset + float2::multiply(float2((float)x, (float)z), float2((float)(terrain_node->size - 1), (float)(terrain_node->size - 1)));
			instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

			if (this->IntersectsFrustum(instance.offset, terrain_node->block.range, 0))
			{
				offset_list.Append(instance.offset);
				scale_list.Append(instance.scale);
				level_list.Append(instance.level);
				info.instances++;
			}
		}
	}

	// From level 1 and out, the four center blocks are already filled with the lower clipmap level, so
	// skip these.
	for (int i = 1; i < terrain_node->levels; i++)
	{
		for (int z = 0; z < 4; z++)
		{
			for (int x = 0; x < 4; x++)
			{
				if (z != 0 && z != 3 && x != 0 && x != 3)
				{
					// Already occupied, skip.
					continue;
				}

				instance.scale = terrain_node->clipmap_scale * float(1 << i);
				instance.level = (float)i;
				instance.offset = level_offsets[i];
				instance.offset = instance.offset + float2::multiply(float2((float)x, (float)z), float2((float)((terrain_node->size - 1) << i), (float)((terrain_node->size - 1) << i)));

				// Skip 2 texels horizontally and vertically at the middle to get a symmetric structure.
				// These regions are filled with horizontal and vertical fixup regions.
				if (x >= 2)
					instance.offset.x() += 2 << i;
				if (z >= 2)
					instance.offset.y() += 2 << i;

				instance.offset = float2::multiply(instance.offset, float2(terrain_node->clipmap_scale, terrain_node->clipmap_scale));

				if (this->IntersectsFrustum(instance.offset, terrain_node->block.range, i))
				{
					offset_list.Append(instance.offset);
					scale_list.Append(instance.scale);
					level_list.Append(instance.level);
					info.instances++;
				}
			}
		}
	}

	return info;
}

void TerrainNodeInstance::update_draw_list(DrawInfo& info, size_t& uniform_buffer_offset)
{
	info.uniform_buffer_offset = uniform_buffer_offset;
	draw_list.Append(info);

	// Have to ensure that the uniform buffer is always bound at aligned offsets.
	uniform_buffer_offset = uniform_buffer_offset + info.instances; //remove the sizeof instancedata
}

void TerrainNodeInstance::update_draw_list()
{
	draw_list.Clear();

	DrawInfo info;
	size_t uniform_buffer_offset = 0;

	Util::Array<float2> offset_list;
	Util::Array<float> scale_list;
	Util::Array<float> level_list;

	// Create a draw list. The number of draw calls is equal to the different types
	// of blocks. The blocks are instanced as necessary in the get_draw_info* calls.

	// Main blocks
	info = get_draw_info_blocks(offset_list, scale_list, level_list);
	//info.debug_color = float4(1, 0, 0, 1);
	update_draw_list(info, uniform_buffer_offset);

	//// Vertical ring fixups
	//info = get_draw_info_vert_fixup(offset_list, scale_list, level_list);
	////info.debug_color = float4(0, 1, 0, 1);
	//update_draw_list(info, uniform_buffer_offset);

	//// Horizontal ring fixups
	//info = get_draw_info_horiz_fixup(offset_list, scale_list, level_list);
	//info.debug_color = float4(0, 1, 0, 1);
	//update_draw_list(info, uniform_buffer_offset);

	///************************************************************************/
	///*                                                                      */
	///************************************************************************/
	//// Left-side degenerates
	//info = get_draw_info_degenerate_left(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Right-side degenerates
	//info = get_draw_info_degenerate_right(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Top-side degenerates
	//info = get_draw_info_degenerate_top(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Bottom-side degenerates
	//info = get_draw_info_degenerate_bottom(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Full trim
	//info = get_draw_info_trim_full(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Top-right trim
	//info = get_draw_info_trim_top_right(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Top-left trim
	//info = get_draw_info_trim_top_left(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Bottom-right trim
	//info = get_draw_info_trim_bottom_right(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	//// Bottom-left trim
	//info = get_draw_info_trim_bottom_left(offset_list, scale_list, level_list);
	//update_draw_list(info, uniform_buffer_offset);

	if (!offset_list.IsEmpty() && !scale_list.IsEmpty() && !level_list.IsEmpty())
	{
		this->offset_shdvar->SetFloat2Array(offset_list.Begin(), offset_list.Size());
		this->scale_shdvar->SetFloatArray(scale_list.Begin(), scale_list.Size());
		this->level_shdvar->SetFloatArray(level_list.Begin(), level_list.Size());
	}
	this->uniform_buffer->CycleBuffers();
}

void TerrainNodeInstance::render_draw_list()
{
	RenderDevice* renderDevice = RenderDevice::Instance();


	for (int i = 0; i < draw_list.Size(); i++)
	{
		if (draw_list[i].instances){
			CoreGraphics::PrimitiveGroup primGroup;
			// setup the primitive group
			primGroup.SetBaseVertex(0);
			primGroup.SetBaseIndex(draw_list[i].index_buffer_offset);
			primGroup.SetNumIndices(draw_list[i].indices);
			primGroup.SetPrimitiveTopology(PrimitiveTopology::TriangleStrip);

			renderDevice->SetPrimitiveGroup(primGroup);
			renderDevice->DrawIndexedInstanced(draw_list[i].instances, draw_list[i].uniform_buffer_offset); //offset the gl_instanceID
		}
	}
}

void TerrainNodeInstance::OnVisibilityResolve(IndexT resolveIndex, float distToViewer)
{
	// check if node is inside lod distances or if no lod is used
	const Ptr<TransformNode>& transformNode = this->modelNode.downcast<TransformNode>();
	if (transformNode->CheckLodDistance(distToViewer))
	{
		this->modelNode->AddVisibleNodeInstance(resolveIndex, this->surfaceInstance->GetCode(), this);
		ModelNodeInstance::OnVisibilityResolve(resolveIndex, distToViewer);
	}
}

void TerrainNodeInstance::UpdateShaderHandles()
{
	//this->terrain_node = this->modelNode.downcast<TerrainNode>();
	this->geoclipmap_shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:geoclipmaps");
	this->SetupUniformBuffer();
	//list with offsets memcpy
	memcpy(this->terrain_node->mapped_offsets, this->id_offset_list.Begin(), this->id_offset_list.Size() * sizeof(float));
}

bool TerrainNodeInstance::IntersectsFrustum(const float2& offset, const float2& range, unsigned int level)
{
	// These depend on the heightmap itself. These should be as small as possible to be able to cull more blocks.
	// We know the range of the block in the XZ-plane, but not in Y as it depends on the heightmap texture.
	// In the vertex shader, we enforce a min/max height, so it is safe to assume a range for Y.
	float y_min = -20.0f;
	float y_max = 20.0f;

	// Create an axis-aligned bounding box.
	// Add a twiddle factor to account for potential precision issues.
	point center = point(offset.x(), y_min, offset.y()) + point(-0.01f, -0.01f, -0.01f);
	vector original_extent, power_of_two_scale, clipmap_scale;
	original_extent = vector(range.x(), 0.0f, range.y());
	power_of_two_scale = vector(float(1 << level));
	clipmap_scale = vector(terrain_node->clipmap_scale);
	
	vector extent = vector::multiply(vector::multiply(original_extent, power_of_two_scale), clipmap_scale);// +vector(0.f, y_max - y_min, 0.f) + vector(0.02f);
	extent.y() = 0.f;
	//vector extent = point::multiply(point::multiply(point(range.x(), 0.0f, range.y()), point(float(1 << level), float(1 << level), float(1 << level))), point(terrain_node->clipmap_scale, terrain_node->clipmap_scale, terrain_node->clipmap_scale)) + point(0, y_max - y_min, 0) + point(0.02f, 0.02f, 0.02f);
	Math::bbox aabb(center, extent);
	//Math::bbox aabb(Math::point(0.f, 0.f, 0.f), Math::vector(0.f, 0.f, 0.f));

	const Ptr<Graphics::CameraEntity>& camera = Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCameraEntity();
	//const Math::frustum viewFrustum = camera->GetViewFrustum();
	
	if (aabb.clipstatus(camera->GetViewProjTransform()) == ClipStatus::Outside){
		this->culled_counter++;
		return false;
	}
	return true;

	//for (unsigned int c = 0; c < 8; c++)
	//{
	//	// Require 4-dimensional coordinates for plane equations.
	//	if (!viewFrustum.inside(aabb.corner_point(c))){
	//		return false;
	//	}
	//}
	//return true;

	//if (viewFrustum.clipstatus(aabb) == ClipStatus::Inside){
	//	return true;
	//}
	//else{
	//	return false;
	//}

	//return view_proj_frustum.intersects_aabb(aabb);
}

} // namespace Models