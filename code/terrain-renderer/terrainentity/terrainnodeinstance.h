#pragma once
//------------------------------------------------------------------------------
/**
@class Models::BillboardNodeInstance

A billboard node instance is represented as an individual renderable instance.

(C) 2013-2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "models/nodes/statenodeinstance.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/primitivegroup.h"

#include "terrainnode.h"
#include "block.h"

using namespace Math;

namespace Terrain
{
class TerrainNodeInstance : public Models::StateNodeInstance
{
	__DeclareClass(TerrainNodeInstance);
public:
	/// constructor
	TerrainNodeInstance();
	/// destructor
	virtual ~TerrainNodeInstance();

	/// called when we render the billboard node
	virtual void Render();
	virtual void OnRenderBefore(IndexT frameIndex, Timing::Time time);

	void update_level_offsets(const float2& camera_pos);

protected:
	/// called when the node sets up
	virtual void Setup(const Ptr<Models::ModelInstance>& inst, const Ptr<Models::ModelNode>& node, const Ptr<Models::ModelNodeInstance>& parentNodeInst);
	/// called when removed
	virtual void Discard();

	Ptr<CoreGraphics::ConstantBuffer> uniform_buffer;
	Ptr<TerrainNode> terrain_node;

	Ptr<CoreGraphics::ShaderVariable> offset_shdvar;
	Ptr<CoreGraphics::ShaderVariable> scale_shdvar;
	Ptr<CoreGraphics::ShaderVariable> level_shdvar;

	void SetupUniformBuffer();

	// a float consumes N unit and alignment is 4N
	struct InstanceData
	{
		float2 offset; // Offset of the block in XZ plane (world space). This is prescaled.
		float scale; // Scale factor of local offsets (vertex coordinates).
		float level; // Clipmap LOD level of block.
		//float2 debug_color;
		//float padding; //padding for debug_color
	};

	//update the instanced draw list structure
	//render the instanced draw list structure
	void update_draw_list();
	void render_draw_list();

	struct DrawInfo
	{
		size_t index_buffer_offset;
		size_t uniform_buffer_offset;
		unsigned int indices;
		unsigned int instances;
	};
	Util::Array<DrawInfo> draw_list;

	size_t uniform_buffer_size;
	//GLint uniform_buffer_align;

	eastl::vector<float2> level_offsets;

	typedef bool(*TrimConditional)(const float2& offset);

	//GLsync syncObj;
	//InstanceData* data;
	Ptr<CoreGraphics::Shader> geoclipmap_shader;
	Ptr<CoreGraphics::ShaderVariable> instance_data_blockvar;

	float2 get_offset_level(const float2& camera_pos, unsigned int level); //snapping grid
	void update_draw_list(DrawInfo& info, size_t& ubo_offset);
	DrawInfo get_draw_info_blocks(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_vert_fixup(InstanceData *instance_data);
	DrawInfo get_draw_info_horiz_fixup(InstanceData *instance_data);
	DrawInfo get_draw_info_degenerate(InstanceData *instance_data, const Block& block, const float2& offset, const float2& ring_offset);
	DrawInfo get_draw_info_degenerate_left(InstanceData *instance_data);
	DrawInfo get_draw_info_degenerate_right(InstanceData *instance_data);
	DrawInfo get_draw_info_degenerate_top(InstanceData *instance_data);
	DrawInfo get_draw_info_degenerate_bottom(InstanceData *instance_data);
	DrawInfo get_draw_info_trim_full(InstanceData *instance_data);
	DrawInfo get_draw_info_trim(InstanceData *instance_data, const Block& block, TrimConditional cond);
	DrawInfo get_draw_info_trim_top_right(InstanceData *instance_data);
	DrawInfo get_draw_info_trim_top_left(InstanceData *instance_data);
	DrawInfo get_draw_info_trim_bottom_right(InstanceData *instance_data);
	DrawInfo get_draw_info_trim_bottom_left(InstanceData *instance_data);
};
} // namespace Models
//------------------------------------------------------------------------------