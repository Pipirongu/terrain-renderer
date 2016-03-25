#pragma once
//------------------------------------------------------------------------------
/**
@class Terrain::TerrainNodeInstance

has a uniform buffer with the instance data for the terrain grid
each frame the instance data is updated
render call with different offsets

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
	/// called when we render the terrain node
	virtual void Render();

protected:
	/// called when the node sets up
	virtual void Setup(const Ptr<Models::ModelInstance>& inst, const Ptr<Models::ModelNode>& node, const Ptr<Models::ModelNodeInstance>& parentNodeInst);
	/// called when removed
	virtual void Discard();


	Ptr<TerrainNode> terrain_node;
	Ptr<CoreGraphics::ConstantBuffer> uniform_buffer;


	Ptr<CoreGraphics::ShaderVariable> offset_shdvar;
	Ptr<CoreGraphics::ShaderVariable> scale_shdvar;
	Ptr<CoreGraphics::ShaderVariable> level_shdvar;

	Ptr<CoreGraphics::Shader> geoclipmap_shader;
	Ptr<CoreGraphics::ShaderVariable> instance_data_blockvar;

	// represents an instance of a shape
	struct InstanceData
	{
		float2 offset; // offset of a clipmap block in xz.
		float scale; // scale vertices and also depending on the clipmap level, power of two scale
		float level; // the level of the instance
	};

	// stores info for one instanced draw call
	struct DrawInfo
	{
		int index_buffer_offset; // index offset for draw call
		int instance_id_offset; // used to offset the id that is used to access the instance data in shader
		unsigned int indices; // amount of indices
		unsigned int instances; // how many instances
	};

	// list of draw calls
	Util::Array<DrawInfo> draw_list;
	eastl::vector<float2> level_offsets;

	typedef bool(*TrimConditional)(const float2& offset);

	/// frustum culling
	bool IntersectsFrustum(const float2& offset, unsigned int level);
	/// setup the instance id offsets
	void SetupInstanceOffsets();
	/// setup uniform buffer for instance data
	void SetupUniformBuffer();

	/// updates the camera offsets for each clipmap level which will be used to offset the shapes
	void UpdateCameraOffsets(const float2& camera_pos);
	/// generate the draw list with instance data
	void UpdateDrawList();
	void UpdateInstanceIDOffset(DrawInfo& info, int& instance_id_offset);
	/// render the terrain
	void RenderDrawList();

	float2 GetCameraOffset(const float2& camera_pos, unsigned int level);

	// functions to generate the instance data and draw data
	DrawInfo GenerateInstanceDataBlocks(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataVertFixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataHoriFixup(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataDegenerate(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, const float2& offset, const float2& ring_offset);
	DrawInfo GenerateInstanceDataDegenerateLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataDegenerateRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataDegenerateTop(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataDegenerateBottom(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataTrimFull(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataTrim(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list, const Block& block, TrimConditional cond);
	DrawInfo GenerateInstanceDataTrimTopRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataTrimTopLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataTrimBottomRight(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
	DrawInfo GenerateInstanceDataTrimBottomLeft(Util::Array<float2>& offset_list, Util::Array<float>& scale_list, Util::Array<float>& level_list);
};
} // namespace Models
//------------------------------------------------------------------------------