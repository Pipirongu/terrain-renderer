//------------------------------------------------------------------------------
//  billboardnode.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainnode.h"
#include "terrainnodeinstance.h"

#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "resources/resourceloader.h"

using namespace CoreGraphics;
using namespace Util;
using namespace Resources;
using namespace Models;

namespace Terrain
{
__ImplementClass(Terrain::TerrainNode, 'TNNE', Models::StateNode);

//------------------------------------------------------------------------------
/**
*/
TerrainNode::TerrainNode() :
	size(64), level_size(4 * 10 - 1), levels(10), clipmap_scale(1.f), vbo(0), ibo(0)
{

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
	Ptr<Models::ModelNodeInstance> newInst = (Models::ModelNodeInstance*) TerrainNodeInstance::Create();
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
	// sets up a vertex buffer which contains vertices for all the different shapes that is needed for the terrain grid
	unsigned int num_vertices = size * size; // Regular block. M*M vertices

	// Ring fixups (vertical and horiz).  3*M vertices.
	unsigned int ring_vertices = size * 3;
	num_vertices += 2 * ring_vertices;

	// trim shapes which surrounds a level and connects it with the next.
	//(2 * M + 1)*2 vertices.
	unsigned int trim_vertices = (2 * size + 1) * 2;
	num_vertices += trim_vertices * 4;

	// degenerate triangles are used to connect the edges between two clipmap levels.
	// this is used to fix the floating point precision error that can occur at the edge which creates a crack.
	unsigned int degenerate_vertices = 2 * (size - 1) * 5;
	num_vertices += degenerate_vertices * 2;

	float *vertices = new float[2 * num_vertices];
	// pointer to use in loop, advancing
	float *pv = vertices;

	// Block
	for (int z = 0; z < size; z++)
	{
		for (int x = 0; x < size; x++)
		{
			pv[0] = (float)x;
			pv[1] = (float)z;
			pv += 2;
		}
	}

	// Vertical ring fixup
	for (int z = 0; z < size; z++)
	{
		for (int x = 0; x < 3; x++)
		{
			pv[0] = (float)x;
			pv[1] = (float)z;
			pv += 2;
		}
	}

	// Horizontal ring fixup
	for (int z = 0; z < 3; z++)
	{
		for (int x = 0; x < size; x++)
		{
			pv[0] = (float)x;
			pv[1] = (float)z;
			pv += 2;
		}
	}

	// Full interior trim
	// Top
	for (int z = 0; z < 2; z++)
	{
		for (int x = 0; x < 2 * size + 1; x++)
		{
			pv[0] = (float)x;
			pv[1] = (float)z;
			pv += 2;
		}
	}

	// Right
	for (int x = 1; x >= 0; x--)
	{
		for (int z = 0; z < 2 * size + 1; z++)
		{
			pv[0] = (float)(x + 2 * size - 1);
			pv[1] = (float)z;
			pv += 2;
		}
	}

	// Bottom
	for (int z = 1; z >= 0; z--)
	{
		for (int x = 0; x < 2 * size + 1; x++)
		{
			pv[0] = (float)(2 * size - x);
			pv[1] = (float)(z + 2 * size - 1);
			pv += 2;
		}
	}

	// Left
	for (int x = 0; x < 2; x++)
	{
		for (int z = 0; z < 2 * size + 1; z++)
		{
			pv[0] = (float)x;
			pv[1] = (float)(2 * size - z);
			pv += 2;
		}
	}

	// Degenerate triangles.
	// Left, right
	for (int y = 0; y < (size - 1) * 2; y++)
	{
		pv[0] = 0.f;
		pv[1] = (float)(y * 2);
		pv[2] = 0.f;
		pv[3] = (float)(y * 2);
		pv[4] = 0.f;
		pv[5] = (float)(y * 2 + 1);
		pv[6] = 0.f;
		pv[7] = (float)(y * 2 + 2);
		pv[8] = 0.f;
		pv[9] = (float)(y * 2 + 2);
		pv += 10;
	}

	// Top, bottom
	for (int x = 0; x < (size - 1) * 2; x++)
	{
		pv[0] = (float)(x * 2);
		pv[1] = 0.f;
		pv[2] = (float)(x * 2);
		pv[3] = 0.f;
		pv[4] = (float)(x * 2 + 1);
		pv[5] = 0.f;
		pv[6] = (float)(x * 2 + 2);
		pv[7] = 0.f;
		pv[8] = (float)(x * 2 + 2);
		pv[9] = 0.f;
		pv += 10;
	}


	// setup the vertex buffer
	Array<VertexComponent> position_components;
	position_components.Append(VertexComponent((Base::VertexComponentBase::SemanticName)0, 0, VertexComponent::Float2, 0));
	Ptr<MemoryVertexBufferLoader> vbLoader = MemoryVertexBufferLoader::Create();
	vbLoader->Setup(position_components, num_vertices, vertices, 2 * num_vertices * sizeof(float), VertexBuffer::UsageImmutable, VertexBuffer::AccessNone);

	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vbLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	if (!this->vbo->IsLoaded())
	{
		n_error("TerrainNode: Failed to setup terrain vertex buffer!");
	}
	this->vbo->SetLoader(0);



	// setup the instance offset buffer
	Array<VertexComponent> offset_components;
	offset_components.Append(VertexComponent((Base::VertexComponentBase::SemanticName)1, 0, VertexComponent::Float, 1, Base::VertexComponentBase::PerInstance, 1));
	Ptr<MemoryVertexBufferLoader> offset_vbLoader = MemoryVertexBufferLoader::Create();
	offset_vbLoader->Setup(offset_components, 768, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite, VertexBuffer::SyncingCoherentPersistent);

	this->instance_offset_buffer = VertexBuffer::Create();
	this->instance_offset_buffer->SetLoader(offset_vbLoader.upcast<ResourceLoader>());
	this->instance_offset_buffer->SetAsyncEnabled(false);
	this->instance_offset_buffer->Load();
	if (!this->instance_offset_buffer->IsLoaded())
	{
		n_error("TerrainNode: Failed to setup terrain instance offset buffer!");
	}
	this->instance_offset_buffer->SetLoader(0);

	this->mapped_offsets = this->instance_offset_buffer->Map(Base::ResourceBase::MapWrite);

	// create own vertex layout
	Array<VertexComponent> components;
	components.AppendArray(position_components);
	components.AppendArray(offset_components);
	this->vertexLayout = VertexLayout::Create();
	this->vertexLayout->SetStreamBuffer(0, this->vbo->GetOGL4VertexBuffer());
	this->vertexLayout->SetStreamBuffer(1, this->instance_offset_buffer->GetOGL4VertexBuffer());
	this->vertexLayout->Setup(components);

	delete[] vertices;
}

/// returns the number of indices needed to create a shape
static unsigned int BlockIndexCount(unsigned int width, unsigned int height)
{
	unsigned int strips = height - 1;
	return strips * (2 * width - 1) + 1;
}


static int *GenerateBlockIndices(int *pi, unsigned int vertex_buffer_offset, unsigned int width, unsigned int height)
{
	int pos = vertex_buffer_offset;
	unsigned int strips = height - 1;

	//generate indices in a zig zag pattern
	for (unsigned int z = 0; z < strips; z++)
	{
		int step_even = width;
		int step_odd;
		if (z % 2){
			step_odd = -1 - step_even;
		}
		else{
			step_odd = 1 - step_even;
		}

		for (unsigned int x = 0; x < 2 * width - 1; x++)
		{
			*pi++ = pos;
			if (x % 2){
				pos += step_odd;
			}
			else{
				pos += step_even;
			}
		}
	}
	*pi++ = pos;

	return pi;
}

void TerrainNode::SetupIndexBuffer(int size)
{
	unsigned int vertex_buffer_offset = 0;

	block.count = BlockIndexCount(size, size);

	vertical.count = BlockIndexCount(3, size);
	horizontal.count = BlockIndexCount(size, 3);

	unsigned int trim_region_indices = BlockIndexCount(2 * size + 1, 2);
	trim_full.count = 4 * trim_region_indices;
	trim_top_left.count = 2 * trim_region_indices;
	trim_bottom_right = trim_bottom_left = trim_top_right = trim_top_left;

	degenerate_left.count = (size - 1) * 2 * 6;
	degenerate_right = degenerate_bottom = degenerate_top = degenerate_left;

	num_indices = block.count + vertical.count + horizontal.count + trim_full.count +
		4 * trim_top_left.count +
		4 * degenerate_left.count;

	int *indices = new int[num_indices];
	// pointer so we can get the offset during draw call. offsetting the indices
	int *pi = indices;

	// Main block
	block.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, size, size);
	vertex_buffer_offset += size * size;

	// Vertical fixup
	vertical.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, 3, size);
	vertex_buffer_offset += 3 * size;

	// Horizontal fixup
	horizontal.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, size, 3);
	vertex_buffer_offset += 3 * size;

	// Full interior trim by the first clipmap level
	trim_full.offset = pi - indices;
	unsigned int full_trim_offset = vertex_buffer_offset;
	unsigned int trim_vertices = (2 * size + 1) * 2;
	pi = GenerateBlockIndices(pi, full_trim_offset, 2 * size + 1, 2); // Top
	full_trim_offset += trim_vertices;
	pi = GenerateBlockIndices(pi, full_trim_offset, 2 * size + 1, 2); // Right
	full_trim_offset += trim_vertices;
	pi = GenerateBlockIndices(pi, full_trim_offset, 2 * size + 1, 2); // Bottom
	full_trim_offset += trim_vertices;
	pi = GenerateBlockIndices(pi, full_trim_offset, 2 * size + 1, 2); // Left
	full_trim_offset += trim_vertices;

	// Top-right interior trim, L-Shape
	trim_top_right.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Top
	pi = GenerateBlockIndices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Right
	vertex_buffer_offset += trim_vertices;

	// Right-bottom interior trim, L-Shape
	trim_bottom_right.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Right
	pi = GenerateBlockIndices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Bottom
	vertex_buffer_offset += trim_vertices;

	// Bottom-left interior trim, L-Shape
	trim_bottom_left.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Bottom
	pi = GenerateBlockIndices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Left
	vertex_buffer_offset += trim_vertices;

	// Left-top interior trim, L-Shape
	trim_top_left.offset = pi - indices;
	pi = GenerateBlockIndices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Left
	pi = GenerateBlockIndices(pi, vertex_buffer_offset - 6 * (2 * size + 1), 2 * size + 1, 2); // Top
	vertex_buffer_offset += trim_vertices;

	// Degenerates triangles
	// Left
	degenerate_left.offset = pi - indices;
	for (int z = 0; z < (size - 1) * 2; z++)
	{
		pi[0] = (5 * z) + 0 + vertex_buffer_offset;
		pi[1] = (5 * z) + 1 + vertex_buffer_offset;
		pi[2] = (5 * z) + 2 + vertex_buffer_offset;
		pi[3] = (5 * z) + 3 + vertex_buffer_offset;
		pi[4] = (5 * z) + 4 + vertex_buffer_offset;
		pi[5] = (5 * z) + 4 + vertex_buffer_offset;
		pi += 6;
	}

	// Right
	degenerate_right.offset = pi - indices;
	int start_z = (size - 1) * 2 - 1;
	for (int z = 0; z < (size - 1) * 2; z++)
	{
		// Windings are in reverse order on this side.
		pi[0] = (5 * (start_z - z)) + 4 + vertex_buffer_offset;
		pi[1] = (5 * (start_z - z)) + 3 + vertex_buffer_offset;
		pi[2] = (5 * (start_z - z)) + 2 + vertex_buffer_offset;
		pi[3] = (5 * (start_z - z)) + 1 + vertex_buffer_offset;
		pi[4] = (5 * (start_z - z)) + 0 + vertex_buffer_offset;
		pi[5] = (5 * (start_z - z)) + 0 + vertex_buffer_offset;
		pi += 6;
	}

	vertex_buffer_offset += (size - 1) * 2 * 5;

	// Top
	degenerate_top.offset = pi - indices;
	for (int x = 0; x < (size - 1) * 2; x++)
	{
		pi[0] = (5 * x) + 0 + vertex_buffer_offset;
		pi[1] = (5 * x) + 1 + vertex_buffer_offset;
		pi[2] = (5 * x) + 2 + vertex_buffer_offset;
		pi[3] = (5 * x) + 3 + vertex_buffer_offset;
		pi[4] = (5 * x) + 4 + vertex_buffer_offset;
		pi[5] = (5 * x) + 4 + vertex_buffer_offset;
		pi += 6;
	}

	// Bottom
	degenerate_bottom.offset = pi - indices;
	int start_x = (size - 1) * 2 - 1;
	for (int x = 0; x < (size - 1) * 2; x++)
	{
		// Windings are in reverse order on this side.
		pi[0] = (5 * (start_x - x)) + 4 + vertex_buffer_offset;
		pi[1] = (5 * (start_x - x)) + 3 + vertex_buffer_offset;
		pi[2] = (5 * (start_x - x)) + 2 + vertex_buffer_offset;
		pi[3] = (5 * (start_x - x)) + 1 + vertex_buffer_offset;
		pi[4] = (5 * (start_x - x)) + 0 + vertex_buffer_offset;
		pi[5] = (5 * (start_x - x)) + 0 + vertex_buffer_offset;
		pi += 6;
	}

	// setup index buffer
	Ptr<MemoryIndexBufferLoader> ibLoader = MemoryIndexBufferLoader::Create();
	ibLoader->Setup(IndexType::Index32, num_indices, indices, num_indices * sizeof(int), IndexBuffer::UsageImmutable, IndexBuffer::AccessNone);

	this->ibo = IndexBuffer::Create();
	this->ibo->SetLoader(ibLoader.upcast<ResourceLoader>());
	this->ibo->SetAsyncEnabled(false);
	this->ibo->Load();
	if (!this->ibo->IsLoaded())
	{
		n_error("TerrainNode: Failed to setup terrain index buffer!");
	}
	this->ibo->SetLoader(0);

	delete[] indices;
}

void TerrainNode::UnloadResources()
{
	n_assert(this->vbo->IsLoaded());
	n_assert(this->instance_offset_buffer->IsLoaded());
	n_assert(this->ibo->IsLoaded());

	this->vbo->Unload();
	this->vbo = 0;
	this->instance_offset_buffer->Unload();
	this->instance_offset_buffer = 0;
	this->ibo->Unload();
	this->ibo = 0;

	Models::StateNode::UnloadResources();
}

} // namespace Models