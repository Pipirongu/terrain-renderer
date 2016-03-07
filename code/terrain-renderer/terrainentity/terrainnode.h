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

private:
	int size; //size of a block
	int level_size; //n from clipmap paper
	int levels;
	float scale;

	void SetupVertexBuffer(int size);
	void SetupIndexBuffer(int size);
}; 

} // namespace Models
//------------------------------------------------------------------------------