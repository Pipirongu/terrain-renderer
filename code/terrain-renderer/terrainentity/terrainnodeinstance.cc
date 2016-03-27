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
#include "materials/surfaceinstance.h"

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
	this->level_offsets.resize(this->terrain_node->levels);
	this->SetupInstanceOffsets();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNodeInstance::Discard()
{
	this->uniform_buffer->Discard();
	this->uniform_buffer = 0;

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

	// get camera pos
	Math::float4 camera_position = CoreGraphics::TransformDevice::Instance()->GetInvViewTransform().get_position();
	float2 camera_pos(camera_position.x(), camera_position.z());
	//float2 camera_pos(0.f, 0.f);

	this->UpdateCameraOffsets(camera_pos);
	this->UpdateDrawList();

	//bind vao, ibo
	renderDevice->SetStreamVertexBuffer(0, this->terrain_node->vbo, 0);
	renderDevice->SetStreamVertexBuffer(1, this->terrain_node->instance_offset_buffer, 0);
	renderDevice->SetVertexLayout(this->terrain_node->vertexLayout); //own vertexlayout
	renderDevice->SetIndexBuffer(this->terrain_node->ibo);

	//bind uniform buffer
	this->instance_data_blockvar = this->geoclipmap_shader->GetVariableByName("InstanceData");
	this->instance_data_blockvar->SetBufferHandle(this->uniform_buffer->GetHandle());

	this->RenderDrawList();
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

float2 TerrainNodeInstance::GetCameraOffset(const float2& camera_pos, unsigned int level)
{
	if (level == 0) // first level has a full trim shape. so it follows the second level
	{
		int temp_size = this->terrain_node->size << 1;
		return this->GetCameraOffset(camera_pos, 1) + float2((float)temp_size, (float)temp_size);
	}
	else
	{

		float clipmap_scale = this->terrain_node->clipmap_scale;
		float2 scaled_pos;
		scaled_pos.x() = camera_pos.x() / clipmap_scale;
		scaled_pos.y() = camera_pos.y() / clipmap_scale;


		float2 temp_value((float)(1 << (level + 1)), (float)(1 << (level + 1)));
		float2 vec_floor;
		vec_floor.x() = scaled_pos.x() / temp_value.x();
		vec_floor.y() = scaled_pos.y() / temp_value.y();

		// rounds the value downwards to an integer that is not larger than the original
		vec_floor.x() = floor(vec_floor.x());
		vec_floor.y() = floor(vec_floor.y());

		float2 snapped_pos = float2::multiply(vec_floor, float2((float)(1 << (level + 1)), (float)(1 << (level + 1))));

		float2 pos = snapped_pos - float2((float)((2 * (terrain_node->size - 1)) << level), (float)((2 * (terrain_node->size - 1)) << level));
		return pos;
	}
}

void TerrainNodeInstance::UpdateCameraOffsets(const float2& camera_pos)
{
	for (int i = 0; i < this->terrain_node->levels; i++){
		this->level_offsets[i] = this->GetCameraOffset(camera_pos, i);
	}
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataHoriFixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	InstanceData instance;

	// Horizontal
	info.index_buffer_offset = this->terrain_node->horizontal.offset;
	info.indices = this->terrain_node->horizontal.count;
	info.instances = 0;

	// skip the first level as there are no fix up shapes there
	for (int i = 1; i < this->terrain_node->levels; i++)
	{
		// left shape
		instance.scale = this->terrain_node->clipmap_scale * float(1 << i); // scale with user defined scale and also power of two depending on the level
		instance.level = (float)i;

		instance.offset = this->level_offsets[i]; // get the camera offset
		instance.offset = instance.offset + float2::multiply(float2(0.f, (float)(2 * (this->terrain_node->size - 1))), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;

		// right side
		instance.offset = this->level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(3.f * (this->terrain_node->size - 1.f) + 2, 2.f * (this->terrain_node->size - 1)), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;
	}

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataVertFixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	InstanceData instance;

	// Vertical
	info.index_buffer_offset = this->terrain_node->vertical.offset;
	info.indices = this->terrain_node->vertical.count;
	info.instances = 0;

	for (int i = 1; i < this->terrain_node->levels; i++)
	{
		// top
		instance.scale = this->terrain_node->clipmap_scale * float(1 << i);
		instance.level = (float)i;

		instance.offset = this->level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(2.f * (this->terrain_node->size - 1), 0), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;

		// bottom
		instance.offset = this->level_offsets[i];
		instance.offset = instance.offset + float2::multiply(float2(2.f * (this->terrain_node->size - 1), 3.f * (this->terrain_node->size - 1) + 2), float2((float)(1 << i), (float)(1 << i)));
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;
	}

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataDegenerate(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, const float2& offset, const float2& ring_offset)
{
	DrawInfo info;
	info.instances = 0;
	info.index_buffer_offset = block.offset;
	info.indices = block.count;

	InstanceData instance;

	// skips the last level as we dont need degenerate triangles there
	for (int i = 0; i < this->terrain_node->levels - 1; i++)
	{
		instance.level = (float)i;
		instance.offset = this->level_offsets[i];
		instance.offset = instance.offset + float2::multiply(offset, float2((float)(1 << i), (float)(1 << i)));

		//in the other levels we need a different offset because we have fix up shapes
		if (i > 0){
			instance.offset = instance.offset + float2::multiply(ring_offset, float2((float)(1 << i), (float)(1 << i)));
		}
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));
		instance.scale = this->terrain_node->clipmap_scale * float(1 << i);

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;
	}

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataDegenerateLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataDegenerate(offset_list, scale_list, level_list, terrain_node->degenerate_left, float2(0.0f, 0.0f), float2(0.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataDegenerateRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataDegenerate(offset_list, scale_list, level_list, terrain_node->degenerate_right, float2(4.f * (terrain_node->size - 1), 0.0f), float2(2.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataDegenerateTop(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataDegenerate(offset_list, scale_list, level_list, terrain_node->degenerate_top, float2(0.0f, 0.0f), float2(0.0f, 0.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataDegenerateBottom(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataDegenerate(offset_list, scale_list, level_list, terrain_node->degenerate_bottom, float2(0.0f, 4.f * (terrain_node->size - 1)), float2(0.0f, 2.0f));
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrimFull(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	info.index_buffer_offset = this->terrain_node->trim_full.offset;
	info.indices = this->terrain_node->trim_full.count;
	info.instances = 0;

	InstanceData instance;

	instance.level = 1;
	instance.offset = this->level_offsets[1];
	instance.offset = instance.offset + float2((float)((this->terrain_node->size - 1) << 1), (float)((this->terrain_node->size - 1) << 1));
	instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));
	instance.scale = this->terrain_node->clipmap_scale * float(1 << 1);

	offset_list.Append(instance.offset);
	scale_list.Append(instance.scale);
	level_list.Append(instance.level);
	info.instances++;

	return info;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrim(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, TrimConditional cond)
{
	DrawInfo info;
	info.index_buffer_offset = block.offset;
	info.indices = block.count;
	info.instances = 0;

	// l shapes
	for (int i = 2; i < this->terrain_node->levels; i++)
	{
		float2 offset_prev_level = this->level_offsets[i - 1];
		float2 offset_current_level = this->level_offsets[i] + float2((float)((this->terrain_node->size - 1) << i), (float)((this->terrain_node->size - 1) << i));

		if (!cond(offset_prev_level - offset_current_level))
			continue;

		InstanceData instance;
		instance.level = (float)i;
		instance.offset = this->level_offsets[i];
		instance.offset = instance.offset + float2((float)((this->terrain_node->size - 1) << i), (float)((this->terrain_node->size - 1) << i));
		instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));
		instance.scale = this->terrain_node->clipmap_scale * float(1 << i);

		offset_list.Append(instance.offset);
		scale_list.Append(instance.scale);
		level_list.Append(instance.level);
		info.instances++;
	}

	return info;
}

static inline bool TrimTopRightCond(const float2& offset)
{
	return offset.x() < 0.5f && offset.y() > 0.5f;
}

static inline bool TrimTopLeftCond(const float2& offset)
{
	return offset.x() > 0.5f && offset.y() > 0.5f;
}

static inline bool TrimBottomRightCond(const float2& offset)
{
	return offset.x() < 0.5f && offset.y() < 0.5f;
}

static inline bool TrimBottomLeftCond(const float2& offset)
{
	return offset.x() > 0.5f && offset.y() < 0.5f;
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrimTopRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataTrim(offset_list, scale_list, level_list, terrain_node->trim_top_right, TrimTopRightCond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrimTopLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataTrim(offset_list, scale_list, level_list, terrain_node->trim_top_left, TrimTopLeftCond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrimBottomRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataTrim(offset_list, scale_list, level_list, terrain_node->trim_bottom_right, TrimBottomRightCond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataTrimBottomLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	return GenerateInstanceDataTrim(offset_list, scale_list, level_list, terrain_node->trim_bottom_left, TrimBottomLeftCond);
}

TerrainNodeInstance::DrawInfo TerrainNodeInstance::GenerateInstanceDataBlocks(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list)
{
	DrawInfo info;
	info.instances = 0;
	info.index_buffer_offset = this->terrain_node->block.offset;
	info.indices = this->terrain_node->block.count;

	InstanceData instance;
	instance.scale = this->terrain_node->clipmap_scale;

	// draw the innermost grid first which is a special case with 4 blocks
	for (int z = 0; z < 4; z++)
	{
		for (int x = 0; x < 4; x++)
		{
			instance.level = 0;
			instance.offset = this->level_offsets[0];
			instance.offset = instance.offset + float2::multiply(float2((float)x, (float)z), float2((float)(this->terrain_node->size - 1), (float)(this->terrain_node->size - 1)));
			instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

			if (this->IntersectsFrustum(instance.offset, 0))
			{
				offset_list.Append(instance.offset);
				scale_list.Append(instance.scale);
				level_list.Append(instance.level);
				info.instances++;
			}
		}
	}

	// the other levels. always skip generating blocks at the postion where the previous level occupy
	for (int i = 1; i < terrain_node->levels; i++)
	{
		for (int z = 0; z < 4; z++)
		{
			for (int x = 0; x < 4; x++)
			{
				if (z != 0 && z != 3 && x != 0 && x != 3)
				{
					// skip when the the inner grid is
					continue;
				}

				instance.scale = this->terrain_node->clipmap_scale * float(1 << i);
				instance.level = (float)i;
				instance.offset = this->level_offsets[i];
				instance.offset = instance.offset + float2::multiply(float2((float)x, (float)z), float2((float)((this->terrain_node->size - 1) << i), (float)((this->terrain_node->size - 1) << i)));

				// skips two texels at the middle where ring fix up will fill
				if (x >= 2)
					instance.offset.x() += 2 << i;
				if (z >= 2)
					instance.offset.y() += 2 << i;

				instance.offset = float2::multiply(instance.offset, float2(this->terrain_node->clipmap_scale, this->terrain_node->clipmap_scale));

				if (this->IntersectsFrustum(instance.offset, i))
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

void TerrainNodeInstance::UpdateInstanceIDOffset(DrawInfo& info, int& instance_id_offset)
{
	info.instance_id_offset = instance_id_offset;
	this->draw_list.Append(info);

	instance_id_offset = instance_id_offset + info.instances;
}

void TerrainNodeInstance::UpdateDrawList()
{
	this->draw_list.Clear();

	DrawInfo info;
	int instance_id_offset = 0;

	Util::Array<float2> offset_list;
	Util::Array<float> scale_list;
	Util::Array<float> level_list;

	//generate the draw list for instance draw

	// Main blocks
	info = this->GenerateInstanceDataBlocks(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Vertical ring fixups
	info = this->GenerateInstanceDataVertFixup(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Horizontal ring fixups
	info = this->GenerateInstanceDataHoriFixup(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Left-side degenerates
	info = this->GenerateInstanceDataDegenerateLeft(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Right-side degenerates
	info = this->GenerateInstanceDataDegenerateRight(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Top-side degenerates
	info = this->GenerateInstanceDataDegenerateTop(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Bottom-side degenerates
	info = this->GenerateInstanceDataDegenerateBottom(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Full trim
	info = this->GenerateInstanceDataTrimFull(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Top-right trim
	info = this->GenerateInstanceDataTrimTopRight(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Top-left trim
	info = this->GenerateInstanceDataTrimTopLeft(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Bottom-right trim
	info = this->GenerateInstanceDataTrimBottomRight(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// Bottom-left trim
	info = this->GenerateInstanceDataTrimBottomLeft(offset_list, scale_list, level_list);
	this->UpdateInstanceIDOffset(info, instance_id_offset);

	// update uniform buffer
	if (!offset_list.IsEmpty() && !scale_list.IsEmpty() && !level_list.IsEmpty())
	{
		this->offset_shdvar->SetFloat2Array(offset_list.Begin(), offset_list.Size());
		this->scale_shdvar->SetFloatArray(scale_list.Begin(), scale_list.Size());
		this->level_shdvar->SetFloatArray(level_list.Begin(), level_list.Size());
	}
	this->uniform_buffer->CycleBuffers();
}

void TerrainNodeInstance::RenderDrawList()
{
	RenderDevice* renderDevice = RenderDevice::Instance();

	// instance draw for each shape. each draw call has an index offset and instanceid offset. baseInstance is used to offset the instanceID
	for (int i = 0; i < this->draw_list.Size(); i++)
	{
		if (this->draw_list[i].instances){
			CoreGraphics::PrimitiveGroup primGroup;
			// setup the primitive group
			primGroup.SetBaseVertex(0);
			primGroup.SetBaseIndex(this->draw_list[i].index_buffer_offset);
			primGroup.SetNumIndices(this->draw_list[i].indices);
			primGroup.SetPrimitiveTopology(PrimitiveTopology::TriangleStrip);

			renderDevice->SetPrimitiveGroup(primGroup);
			renderDevice->DrawIndexedInstanced(this->draw_list[i].instances, this->draw_list[i].instance_id_offset);
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

bool TerrainNodeInstance::IntersectsFrustum(const float2& offset, unsigned int level)
{
	//// need fixing, doesn't seem to cull right
	//point center = point(offset.x(), 10.f, offset.y());
	//vector original_extent, power_of_two_scale, clipmap_scale;
	//original_extent = vector(63, 0.0f, 63); // a block is 64x64 vertices
	//power_of_two_scale = vector(float(1 << level)); // scale depending on clipmap level
	//clipmap_scale = vector(this->terrain_node->clipmap_scale); // user-defined scale
	//
	//// the extent is scaled depending on which level the block belongs to. (original extent should be divided by 2, because boundingbox has a center and an halfextent.. doesnt work though culls too early)
	//vector extent = vector::multiply(vector::multiply(original_extent, power_of_two_scale), clipmap_scale) + vector(0.f, 10.f, 0.f);
	//Math::bbox aabb(center, extent);
	//
	//if (aabb.clipstatus(CoreGraphics::TransformDevice::Instance()->GetViewProjTransform()) == ClipStatus::Outside){
	//	return false;
	//}

	// no frustum culling for now. too buggy
	return true;
}

void TerrainNodeInstance::SetupInstanceOffsets()
{
	Util::Array<float> instance_id_offset_list;
	for (unsigned int i = 0; i < 256; i++){
		instance_id_offset_list.Append((float)i);
	}
	//list with offsets memcpy
	memcpy(this->terrain_node->mapped_offsets, instance_id_offset_list.Begin(), instance_id_offset_list.Size() * sizeof(float));
}

} // namespace Models