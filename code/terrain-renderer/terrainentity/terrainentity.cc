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
	//texture(0),
   // color(1,1,1,1),
	modelInstance(0)
	//viewAligned(false)
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
	//n_assert(this->resource.IsValid());
	//n_assert(!this->texture.isvalid());
	n_assert(!this->modelInstance.isvalid());
	//n_assert(!this->textureVariable.isvalid());
	GraphicsEntity::OnActivate();

	//ResourceManager* resManager = ResourceManager::Instance();
	//ShaderServer* shdServer = ShaderServer::Instance();

	// create texture
	//this->texture = resManager->CreateManagedResource(Texture::RTTI, this->resource, NULL, true).downcast<ManagedTexture>();

	// setup base model
	this->terrain_model = Model::Create();
	this->terrain_node = TerrainNode::Create();
	//Setup terrain node wti
	//this->terrain_node->SetBoundingBox(Math::bbox(Math::point(0, 0, 0), Math::vector(1, 1, 1)));
	this->terrain_node->SetBoundingBox(Math::bbox(Math::point(0, 0, 0), Math::vector((4096 - 1.f) / 2.f, 10.f, (4096 - 1.f) / 2.f)));
	//this->SetAlwaysVisible(true);
	this->terrain_node->SetSurfaceName("sur:geoclipmap_surfaces/geoclipmap");
	this->terrain_node->SetName("root");
	this->terrain_node->SetClipmapData(64, 10, 1.f);
	this->terrain_node->LoadResources(true);
	this->terrain_model->AttachNode(this->terrain_node.upcast<ModelNode>());
	
	// create model instance
	this->modelInstance = this->terrain_model->CreateInstance();
	this->modelInstance->SetTransform(this->transform); //*****
	this->modelInstance->SetPickingId(this->pickingId);

	// get node instance and set the view space aligned flag
	Ptr<TerrainNodeInstance> nodeInstance = this->modelInstance->GetRootNodeInstance().downcast<TerrainNodeInstance>();

 //   // setup material
 //   const Ptr<SurfaceInstance>& surface = nodeInstance->GetSurfaceInstance();
 //   this->textureVariable = surface->GetConstant("AlbedoMap");
 //   this->colorVariable = surface->GetConstant("Color");

	//// create a variable instance and set the texture
	//this->textureVariable->SetTexture(this->texture->GetTexture());
	//nodeInstance->SetInViewSpace(this->viewAligned);

	// set to be valid
	this->SetValid(true);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEntity::OnDeactivate()
{
	//n_assert(this->IsActive());
	//n_assert(this->texture.isvalid());
	//n_assert(this->modelInstance.isvalid());
	//n_assert(this->textureVariable.isvalid());

	//// cleanup resources
	//ResourceManager* resManager = ResourceManager::Instance();
	//resManager->DiscardManagedResource(this->texture.upcast<ManagedResource>());

	//// discard model instance
	//this->modelInstance->GetModel()->DiscardInstance(this->modelInstance);
	//this->modelInstance = 0;	

	//// discard texture variable
 //   this->textureVariable = 0;
 //   this->colorVariable = 0;

	//// kill model if this is our last billboard entity
	//if (this->terrain_model->GetInstances().Size() == 0)
	//{
	//	this->terrain_node->UnloadResources();
	//	this->terrain_node = 0;
	//	this->terrain_model->Unload();
	//	this->terrain_model = 0;
	//}

	// up to parent class
	GraphicsEntity::OnDeactivate();
}

////------------------------------------------------------------------------------
///**
//*/
//void
//TerrainEntity::OnResolveVisibility(IndexT frameIndex, bool updateLod)
//{
//	n_assert(this->modelInstance.isvalid());
//	VisResolver::Instance()->AttachVisibleModelInstancePlayerCamera(frameIndex, this->modelInstance, updateLod);
//}
//
////------------------------------------------------------------------------------
///**
//*/
//void 
//TerrainEntity::OnTransformChanged()
//{
//	// set transform of model instance
//	if (this->modelInstance.isvalid())
//	{
//		this->modelInstance->SetTransform(this->transform);
//	}
//
//    GraphicsEntity::OnTransformChanged();
//}
//
////------------------------------------------------------------------------------
///**
//*/
//void 
//TerrainEntity::OnNotifyCullingVisible(const Ptr<GraphicsEntity>& observer, IndexT frameIndex)
//{
//	if (this->IsVisible())
//	{
//		// call back our model-instance
//		if (this->modelInstance.isvalid())
//		{
//			this->modelInstance->OnNotifyCullingVisible(frameIndex, this->entityTime);
//		}
//	}
//
//	// call parent-class
//	GraphicsEntity::OnNotifyCullingVisible(observer, frameIndex);
//}

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