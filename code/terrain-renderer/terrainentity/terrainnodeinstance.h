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

	/// called when visibility resolves
	virtual void OnVisibilityResolve(IndexT resolveIndex, float distToViewer);
	/// called when we render the billboard node
	virtual void Render();

	void update_level_offsets(const float2& camera_pos);

protected:
	/// called when the node sets up
	virtual void Setup(const Ptr<Models::ModelInstance>& inst, const Ptr<Models::ModelNode>& node, const Ptr<Models::ModelNodeInstance>& parentNodeInst);
	/// called when removed
	virtual void Discard();

	IndexT bufferIndex;
	// create buffer lock
	Ptr<CoreGraphics::BufferLock> offsets_BufferLock;
	Ptr<CoreGraphics::ConstantBuffer> uniform_buffer;
	Ptr<TerrainNode> terrain_node;
	Ptr<Graphics::CameraEntity> camera;

	Ptr<CoreGraphics::ShaderVariable> offset_shdvar;
	Ptr<CoreGraphics::ShaderVariable> scale_shdvar;
	Ptr<CoreGraphics::ShaderVariable> level_shdvar;
	Util::Array<float> id_offset_list;

	void SetupUniformBuffer();

	// a float consumes N unit and alignment is 4N
	struct InstanceData
	{
		float2 offset; // Offset of the block in XZ plane (world space). This is prescaled.
		float scale; // Scale factor of local offsets (vertex coordinates).
		float level; // Clipmap LOD level of block.
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
		float4 debug_color;
	};
	Util::Array<DrawInfo> draw_list;

	eastl::vector<float2> level_offsets;

	typedef bool(*TrimConditional)(const float2& offset);

	Ptr<CoreGraphics::Shader> geoclipmap_shader;
	Ptr<CoreGraphics::ShaderVariable> instance_data_blockvar;

	float2 get_offset_level(const float2& camera_pos, unsigned int level); //snapping grid
	void update_draw_list(DrawInfo& info, size_t& ubo_offset);
	DrawInfo get_draw_info_blocks(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_vert_fixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_horiz_fixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_degenerate(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, const float2& offset, const float2& ring_offset);
	DrawInfo get_draw_info_degenerate_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_degenerate_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_degenerate_top(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_degenerate_bottom(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_trim_full(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_trim(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, TrimConditional cond);
	DrawInfo get_draw_info_trim_top_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_trim_top_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_trim_bottom_right(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo get_draw_info_trim_bottom_left(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
};
} // namespace Models
//------------------------------------------------------------------------------