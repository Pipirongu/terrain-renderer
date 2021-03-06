#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::TerrainEntity
    
	Terrain using geometry clipmaps
    
    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicsentity.h"
#include "coregraphics/shaderinstance.h"
#include "resources/resourceid.h"
#include "models/model.h"
#include "terrainnode.h"
#include "models/modelinstance.h"
#include "materials/surfaceconstantinstance.h"

namespace Graphics
{
class TerrainEntity : public Graphics::GraphicsEntity
{
	__DeclareClass(TerrainEntity);
public:
	/// constructor
	TerrainEntity();
	/// destructor
	virtual ~TerrainEntity();

	/// activate entity
	void OnActivate();
	/// deactivate entity
	void OnDeactivate();

	/// hide
	void OnHide();
	/// show
	void OnShow();

	/// set picking id of model entity
	void SetPickingId(IndexT i);
	/// get picking id of model entity
	const IndexT GetPickingId() const;

private:

	/// resolve visibility
	void OnResolveVisibility(IndexT frameIndex, bool updateLod = false);
	/// update transforms
	void OnTransformChanged();
	/// notify visibility
	void OnNotifyCullingVisible(const Ptr<GraphicsEntity>& observer, IndexT frameIndex);
	/// prepare rendering
	void OnRenderBefore(IndexT frameIndex);
	
	IndexT pickingId;
    
	Ptr<Models::ModelInstance> modelInstance;
	Ptr<Models::Model> terrain_model;
	Ptr<Terrain::TerrainNode> terrain_node;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const IndexT 
TerrainEntity::GetPickingId() const
{
	return this->pickingId;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TerrainEntity::SetPickingId(IndexT i)
{
	this->pickingId = i;
}

} // namespace Graphics
//------------------------------------------------------------------------------