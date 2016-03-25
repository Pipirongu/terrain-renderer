//------------------------------------------------------------------------------
//  billboardentity.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainentity.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shaderserver.h"
#include "terrainnodeinstance.h"
#include "models/nodes/statenodeinstance.h"
#include "models/visresolver.h"

using namespace Resources;
using namespace CoreGraphics;
using namespace Models;
using namespace Materials;
using namespace Models;
using namespace Graphics;
using namespace Messaging;
using namespace Math;
using namespace Terrain;

namespace Graphics
{
__ImplementClass(Graphics::TerrainEntity, 'TENY', Graphics::GraphicsEntity);

//------------------------------------------------------------------------------
/**
*/
TerrainEntity::TerrainEntity() :
	modelInstance(0)
{
	this->SetType(GraphicsEntityType::Model);
}

//------------------------------------------------------------------------------
/**
*/
TerrainEntity::~TerrainEntity()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnActivate()
{
	n_assert(!this->IsActive());
	n_assert(!this->modelInstance.isvalid());
	GraphicsEntity::OnActivate();

	// setup base model
	this->terrain_model = Model::Create();
	this->terrain_node = TerrainNode::Create();
	this->SetAlwaysVisible(true);
	this->terrain_node->SetSurfaceName("sur:geoclipmap_surfaces/geoclipmap");
	this->terrain_node->SetName("root");
	// clipmap size
	this->terrain_node->SetClipmapData(64, 10, 1.f);
	this->terrain_node->LoadResources(true);
	this->terrain_model->AttachNode(this->terrain_node.upcast<ModelNode>());
	
	// create model instance
	this->modelInstance = this->terrain_model->CreateInstance();
	this->modelInstance->SetPickingId(this->pickingId);

	Ptr<TerrainNodeInstance> nodeInstance = this->modelInstance->GetRootNodeInstance().downcast<TerrainNodeInstance>();
	const Ptr<SurfaceInstance>& surfaceInstance = nodeInstance->GetSurfaceInstance();


	// extract the size of the height map
	Ptr<Materials::SurfaceConstant> height_map_texture = surfaceInstance->GetConstant("height_map");
	Ptr<CoreGraphics::Texture> textureObject = (CoreGraphics::Texture*)height_map_texture->GetValue().GetObject();

	float size = (float)textureObject->GetWidth();

	Ptr<Materials::SurfaceConstant> height_map_size = surfaceInstance->GetConstant("height_map_size");
	height_map_size->SetValue(size);

	Ptr<Materials::SurfaceConstant> height_map_multiplier = surfaceInstance->GetConstant("height_map_multiplier");
	height_map_multiplier->SetValue(500.f);

	// set to be valid
	this->SetValid(true);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnDeactivate()
{
	n_assert(this->IsActive());
	n_assert(this->modelInstance.isvalid());

	// discard model instance
	this->modelInstance->GetModel()->DiscardInstance(this->modelInstance);
	this->modelInstance = 0;	

	// kill model if this is our last billboard entity
	if (this->terrain_model->GetInstances().Size() == 0)
	{
		this->terrain_node->UnloadResources();
		this->terrain_node = 0;
		this->terrain_model->Unload();
		this->terrain_model = 0;
	}

	// up to parent class
	GraphicsEntity::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainEntity::OnHide()
{
	if (this->modelInstance.isvalid())
	{
		this->modelInstance->OnHide(this->entityTime);
	}
	GraphicsEntity::OnHide();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainEntity::OnShow()
{
	if (this->modelInstance.isvalid())
	{
		this->modelInstance->OnShow(this->entityTime);
	}
	GraphicsEntity::OnShow();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainEntity::OnResolveVisibility(IndexT frameIndex, bool updateLod)
{
	n_assert(this->modelInstance.isvalid());
	VisResolver::Instance()->AttachVisibleModelInstancePlayerCamera(frameIndex, this->modelInstance, updateLod);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnTransformChanged()
{
	// set transform of model instance
	if (this->modelInstance.isvalid())
	{
		this->modelInstance->SetTransform(this->transform);
	}

    GraphicsEntity::OnTransformChanged();
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnNotifyCullingVisible(const Ptr<GraphicsEntity>& observer, IndexT frameIndex)
{
	if (this->IsVisible())
	{
		// call back our model-instance
		if (this->modelInstance.isvalid())
		{
			this->modelInstance->OnNotifyCullingVisible(frameIndex, this->entityTime);
		}
	}

	// call parent-class
	GraphicsEntity::OnNotifyCullingVisible(observer, frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnRenderBefore(IndexT frameIndex)
{
    if (this->renderBeforeFrameIndex != frameIndex)
    {
        // if our model instance is valid, let it update itself
        if (this->modelInstance.isvalid())
        {
			Ptr<TerrainNodeInstance> nodeInstance = this->modelInstance->GetRootNodeInstance().downcast<TerrainNodeInstance>();
            this->modelInstance->OnRenderBefore(frameIndex, this->entityTime);
        }

        GraphicsEntity::OnRenderBefore(frameIndex);
    }
}

} // namespace Graphics