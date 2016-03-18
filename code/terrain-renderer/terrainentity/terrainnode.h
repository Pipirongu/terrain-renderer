#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::TerrainNode
    
     representation of a billboard shape. 
	Is basically a shape node but without a mesh load from file.
    
    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "models/nodes/statenode.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/shaderinstance.h"

#include "block.h"

namespace Terrain
{
class TerrainNode : public Models::StateNode
{
	__DeclareClass(TerrainNode);
public:
	/// constructor
	TerrainNode();
	/// destructor
	virtual ~TerrainNode();

	/// create a model node instance
	virtual Ptr<Models::ModelNodeInstance> CreateNodeInstance() const;

	void LoadResources(bool sync);
    /// set material name
    void SetSurfaceName(const Util::String& name);
	
	// Sets the size of clipmap blocks, MxM vertices per block. Should be power-of-two. M is based on N. A clipmap level is (4M-1) * (4M-1) grid.
	void SetClipmapData(int size, int levels, float scale);


	int size; //size of a block
	int level_size; //n from clipmap paper
	int levels;
	float clipmap_scale;

	size_t num_indices;

	Ptr<CoreGraphics::VertexBuffer> vbo;
	Ptr<CoreGraphics::IndexBuffer> ibo;
	Ptr<CoreGraphics::VertexBuffer> offset_buffer;
	Ptr<CoreGraphics::VertexLayout> vertexLayout;
	void* mapped_offsets;


	//blocks
	Block block;
	Block vertical;
	Block horizontal;
	Block trim_full;
	Block trim_top_right;
	Block trim_bottom_right;
	Block trim_bottom_left;
	Block trim_top_left;
	Block degenerate_left;
	Block degenerate_top;
	Block degenerate_right;
	Block degenerate_bottom;

private:
	void SetupVertexBuffer(int size);
	void SetupIndexBuffer(int size);
	void SetupBlockRanges(int size);
}; 

} // namespace Models
//------------------------------------------------------------------------------