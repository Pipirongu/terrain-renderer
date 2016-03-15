//------------------------------------------------------------------------------
//  billboardnode.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainnode.h"
#include "terrainnodeinstance.h"

//#include "models/modelnode.h"
//#include "models/nodes/transformnode.h"
//#include "models/nodes/statenodeinstance.h"
//#include "models/modelnodeinstance.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/vertexcomponent.h"
//#include "coregraphics/shaderserver.h"
//#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "resources/resourceloader.h"
//#include "coregraphics/renderdevice.h"

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
	// The ground consists of many smaller tesselated quads.
	// These smaller quads can be instanced to stamp out a big area (clipmap) where quads further away from camera
	// can be larger, and hence, less detail.
	// The grid is completely flat (XZ-plane), but they are offset in Y direction with a heightmap in vertex shader.
	// We also need padding/fixup regions to fill the missing space which
	// shows up when the clipmap is put together.

	// See Doxygen for an illustration on how these blocks are laid out to form the terrain.

	unsigned int num_vertices = size * size; // Regular block

	// Ring fixups (vertical and horiz). The regions are 3-by-N vertices.
	unsigned int ring_vertices = size * 3;
	num_vertices += 2 * ring_vertices;

	// Trim regions are thin stripes which surround blocks from the lower LOD level.
	// Need (2 * size + 1)-by-2 vertices. One stripe for each four sides are needed.
	unsigned int trim_vertices = (2 * size + 1) * 2;
	num_vertices += trim_vertices * 4;

	// Degenerate triangles. These are run on the edge between clipmap levels.
	// These are needed to avoid occasional "missing pixels" between clipmap levels as
	// imperfections in precision can cause the terrain to not perfectly overlap at the clipmap level boundary.
	//
	// 5 vertices are used per vertex to create a suitable triangle strip.
	// (This is somewhat redundant, but it simplifies the implementation).
	// Two different strips are needed for left/right and top/bottom.
	unsigned int degenerate_vertices = 2 * (size - 1) * 5;
	num_vertices += degenerate_vertices * 2;

	float *vertices = new float[2 * num_vertices];
	//! [Generating vertex buffer]
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
	//! [Generating vertex buffer]

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

	// Right and bottom share vertices with left and top respectively.


	// setup the corner vertex buffer
	Array<VertexComponent> components;
	components.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float2, 0));
	//components.Append(VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2, 0));
	GLubyte cornerVertexData[] = { 0, 1, 0, 0, 1, 0, 1, 1 };
	Ptr<MemoryVertexBufferLoader> vbLoader = MemoryVertexBufferLoader::Create();
	//vbLoader->Setup(components, 4, cornerVertexData, sizeof(cornerVertexData), VertexBuffer::UsageImmutable, VertexBuffer::AccessNone);
	vbLoader->Setup(components, num_vertices, vertices, 2 * num_vertices * sizeof(float), VertexBuffer::UsageImmutable, VertexBuffer::AccessNone);

	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vbLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	if (!this->vbo->IsLoaded())
	{
		n_error("TerrainNode: Failed to setup terrain vertex buffer!");
	}
	this->vbo->SetLoader(0);

	//glGenBuffers(1, &vertex_buffer);
	//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	//glBufferData(GL_ARRAY_BUFFER, 2 * num_vertices * sizeof(GLubyte), vertices, GL_STATIC_DRAW);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete[] vertices;
}

// Returns number of indices needed to create a triangle stripped mesh using generate_block_indices() below.
static unsigned int block_index_count(unsigned int width, unsigned int height)
{
	unsigned int strips = height - 1;
	return strips * (2 * width - 1) + 1;
}

//! [Generating index buffer]
static int *generate_block_indices(int *pi, unsigned int vertex_buffer_offset,
	unsigned int width, unsigned int height)
{
	// Stamp out triangle strips back and forth.
	int pos = vertex_buffer_offset;
	unsigned int strips = height - 1;

	// After even indices in a strip, always step to next strip.
	// After odd indices in a strip, step back again and one to the right or left.
	// Which direction we take depends on which strip we're generating.
	// This creates a zig-zag pattern.
	for (unsigned int z = 0; z < strips; z++)
	{
		int step_even = width;
		int step_odd;
		//int step_odd = ((z & 1) ? -1 : 1) - step_even;
		if (z % 2){
			step_odd = -1 - step_even;
		}
		else{
			step_odd = 1 - step_even;
		}

		// We don't need the last odd index.
		// The first index of the next strip will complete this strip.
		for (unsigned int x = 0; x < 2 * width - 1; x++)
		{
			*pi++ = pos;
			//pos += (x & 1) ? step_odd : step_even;
			if (x % 2){
				pos += step_odd;
			}
			else{
				pos += step_even;
			}
		}
	}
	// There is no new strip, so complete the block here.
	*pi++ = pos;

	// Return updated index buffer pointer.
	// More explicit than taking reference to pointer.
	return pi;
}

void TerrainNode::SetupIndexBuffer(int size)
{
	unsigned int vertex_buffer_offset = 0;

	block.count = block_index_count(size, size);

	vertical.count = block_index_count(3, size);
	horizontal.count = block_index_count(size, 3);

	unsigned int trim_region_indices = block_index_count(2 * size + 1, 2);
	trim_full.count = 4 * trim_region_indices;
	trim_top_left.count = 2 * trim_region_indices;
	trim_bottom_right = trim_bottom_left = trim_top_right = trim_top_left;

	// 6 indices are used here per vertex.
	// Need to repeat one vertex to get correct winding when
	// connecting the triangle strips.
	degenerate_left.count = (size - 1) * 2 * 6;
	degenerate_right = degenerate_bottom = degenerate_top = degenerate_left;

	num_indices = block.count + vertical.count + horizontal.count + trim_full.count +
		4 * trim_top_left.count +
		4 * degenerate_left.count;

	int *indices = new int[num_indices];
	int *pi = indices;

	// Main block
	block.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, size, size);
	vertex_buffer_offset += size * size;

	// Vertical fixup
	vertical.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, 3, size);
	vertex_buffer_offset += 3 * size;

	// Horizontal fixup
	horizontal.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, size, 3);
	vertex_buffer_offset += 3 * size;

	// Full interior trim
	// All trims can be run after each other.
	// The vertex buffer is generated such that this creates a "ring".
	// The full trim is only used to connect clipmap level 0 to level 1. See Doxygen for more detail.
	trim_full.offset = pi - indices;
	unsigned int full_trim_offset = vertex_buffer_offset;
	unsigned int trim_vertices = (2 * size + 1) * 2;
	pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Top
	full_trim_offset += trim_vertices;
	pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Right
	full_trim_offset += trim_vertices;
	pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Bottom
	full_trim_offset += trim_vertices;
	pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Left
	full_trim_offset += trim_vertices;

	// Top-right interior trim
	// This is a half ring (L-shaped).
	trim_top_right.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Top
	pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Right
	vertex_buffer_offset += trim_vertices;

	// Right-bottom interior trim
	// This is a half ring (L-shaped).
	trim_bottom_right.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Right
	pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Bottom
	vertex_buffer_offset += trim_vertices;

	// Bottom-left interior trim
	// This is a half ring (L-shaped).
	trim_bottom_left.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Bottom
	pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Left
	vertex_buffer_offset += trim_vertices;

	// Left-top interior trim
	// This is a half ring (L-shaped).
	trim_top_left.offset = pi - indices;
	pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Left
	pi = generate_block_indices(pi, vertex_buffer_offset - 6 * (2 * size + 1), 2 * size + 1, 2); // Top
	vertex_buffer_offset += trim_vertices;

	// One of the trim regions will be used to connect level N with level N + 1.

	// Degenerates. Left and right share vertices (with different offsets in vertex shader). Top and bottom share.
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

	//glGenBuffers(1, &index_buffer);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLushort), indices, GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	delete[] indices;
}

} // namespace Models