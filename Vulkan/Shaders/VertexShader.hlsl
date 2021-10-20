// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major)
struct OBJ_ATTR
{
	float3		Kd;				// diffuse reflectivity
	float			d;					// dissolve (transparency) 
	float3		Ks;				// specular reflectivity
	float			Ns;				// specular exponent
	float3		Ka;				// ambient reflectivity
	float			sharpness;	// local reflection map sharpness
	float3		Tf;					// transmission filter
	float			Ni;				// optical density (index of refraction)
	float3		Ke;				// emissive reflectivity
	float			illum;			// illumination model
};
struct SHADER_MODEL_DATA
{
	float4 lightDirection;
	float4 lightColor;
	float4 ambientLight;
	float4 cameraPos;
	float4x4 vMatrix;
	float4x4 pMatrix;
	float4x4 wMatrix[1024];
	OBJ_ATTR material[1024];
};
[[vk::push_constant]]
cbuffer MESH_INDEX {
	uint mesh_ID;
};
struct Vertex_Output
{
	float4 posH : SV_POSITION;
	float3 nrmW : NORMAL;
	float3 posW : WORLD;
};
struct Vertex_Input
{
	float3 pos : POSITION;
	float3 uvw : TEXCOORD;
	float3 nrm : NORMAL;
};
StructuredBuffer<SHADER_MODEL_DATA> SCENE_DATA;
Vertex_Output main(Vertex_Input INPUT)
{
	Vertex_Output OUTPUT;
	OUTPUT.posH = float4(INPUT.pos, 1);																		// Convert Input Position into a float4 for matrix mult
	float4 temp_nrm = mul(float4(INPUT.nrm, 1), SCENE_DATA[0].wMatrix[mesh_ID]);	// Create Temp float4 to get get the world space normal
	OUTPUT.nrmW = temp_nrm.xyz;																					// Store world space normal in float3 OUTPUT variable
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].wMatrix[mesh_ID]);				// Get world Position by multiplying world matrix
	OUTPUT.posW = OUTPUT.posH.xyz;																			// Store World Position for Lighting in Float3 OUTPUT var
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].vMatrix);									// View Matrix Multiplication
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].pMatrix);									// Proj Matrix Multiplication

	return OUTPUT;
}