//------------------------------------------------------------------------------
//  geoclipmaps.fx
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D height_map;
sampler2D albedo_map;

float height_map_size;
float height_map_multiplier;

#define INSTANCE_SIZE 256

shared varblock InstanceData[bool System = true;]
{
	vec2 offset[INSTANCE_SIZE];
	float scale[INSTANCE_SIZE];
	float level[INSTANCE_SIZE];
};



samplerstate HeightmapSampler
{
	Samplers = {height_map};
	Filter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
};

samplerstate TextureSampler
{
	Samplers = {albedo_map};
	AddressU = Wrap;
	AddressV = Wrap;
};

state GeoclipState
{
	CullMode = None;
};

state WireframeState
{
	CullMode = None;
	FillMode = Line;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGeoclipmap(in vec2 position, in float ginstanceID_offset , out float height_value, out vec2 terrain_uv, out float terrain_level, out vec4 normals)
{
	vec2 local_offset = position * scale[int(ginstanceID_offset)];
	vec2 pos = offset[int(ginstanceID_offset)] + local_offset;
	
	float level = level[int(ginstanceID_offset)];
	
	vec2 uvcoord = pos/height_map_size; //send in the size of the heightmap
	float height = textureLod(height_map, uvcoord, level).r;
	
	vec3 off = vec3(1.0, 1.0, 0.0);
	float hL =  textureLod(height_map, (pos - off.xz)/height_map_size, level).r;
	float hR =  textureLod(height_map, (pos + off.xz)/height_map_size, level).r;
	float hD =  textureLod(height_map, (pos - off.zy)/height_map_size, level).r;
	float hU =  textureLod(height_map, (pos + off.zy)/height_map_size, level).r;
	
	// deduce terrain normal
	vec3 Normal;
	Normal.x = hL - hR;
	Normal.y = hD - hU;
	Normal.z = 2.0;
	Normal = normalize(Normal);
	
	normals = PackViewSpaceNormal(Normal);
	
	vec4 vert = vec4(pos.x, height_map_multiplier*height, pos.y, 1.0);
	
	gl_Position = ViewProjection * vert;
	
	height_value = height;
	terrain_uv = uvcoord;
	terrain_level = level;
	
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGeoclipmap(in float height_value, in vec2 terrain_uv, in float terrain_level, in vec4 normals, [color0] out vec4 color, [color1] out vec4 Normals)
{
	vec3 color1 = textureLod(albedo_map, terrain_uv, terrain_level).rgb;
	color = vec4(color1, 1);
	Normals = normals;
	
	//color = vec4(height_value, height_value, height_value, 1);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Geoclipmap, "GeoclipmapStatic", vsGeoclipmap(), psGeoclipmap(), GeoclipState);
SimpleTechnique(Geoclipmap_Wireframe, "GeoclipmapWireframe", vsGeoclipmap(), psGeoclipmap(), WireframeState);