//------------------------------------------------------------------------------
//  billboardnode.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainnode.h"
#include "terrainnodeinstance.h"

namespace Terrain
{
__ImplementClass(Terrain::TerrainNode, 'TNNE', Models::StateNode);

//------------------------------------------------------------------------------
/**
*/
TerrainNode::TerrainNode()
{
	this->size = 64;
	this->level_size = 4 * 10 - 1;
	this->levels = 10;
	this->scale = 1.f;
}

//------------------------------------------------------------------------------
/**
*/
TerrainNode::~TerrainNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Models::ModelNodeInstance> 
TerrainNode::CreateNodeInstance() const
{
	Ptr<Models::ModelNodeInstance> newInst = (Models::ModelNodeInstance*) BillboardNodeInstance::Create();
	return newInst;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainNode::SetSurfaceName(const Util::String& name)
{
    this->materialName = name;
}

void TerrainNode::LoadResources(bool sync)
{
	Models::StateNode::LoadResources(sync);

	//setup grid vbo/ibo
	this->SetupVertexBuffer(this->size);
	this->SetupIndexBuffer(this->size);
	//allocate a uniform buffer?
	//uniform buffer alignment??
}

void TerrainNode::SetClipmapData(int size, int levels, float scale)
{
	this->size = size;
	this->level_size = 4 * size - 1;
	this->levels = levels;
	this->scale = scale;
}

void TerrainNode::SetupVertexBuffer(int size)
{

}

void TerrainNode::SetupIndexBuffer(int size)
{

}

} // namespace Models