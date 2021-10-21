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
struct LEVEL_MODEL_DATA
{
	float4 lightDirection;
	float4 lightColor;
	float4 ambientLight;
	float4 cameraPos;
	float4x4 vMatrix;
	float4x4 pMatrix;
	float4 pointLightPositions[16];
	float4 pointLightColor;
	float4x4 wMatrix[1024];
	OBJ_ATTR material[1024];
	float numLights;
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
[[vk::push_constant]]
cbuffer MATRIX_DATA {
	float meshID;
	float materialID;
};
StructuredBuffer<LEVEL_MODEL_DATA> SCENE_DATA;
Vertex_Output main(Vertex_Input INPUT)
{
	Vertex_Output OUTPUT;
	OUTPUT.posH = float4(INPUT.pos, 1);

	// Get World Space Normal and Position
	OUTPUT.nrmW = mul(INPUT.nrm, SCENE_DATA[0].wMatrix[meshID]);
	OUTPUT.posW = mul(INPUT.pos, SCENE_DATA[0].wMatrix[meshID]);

	// Get World, View, Projection Space of Pixel (Homogenous Position)
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].wMatrix[meshID]);
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].vMatrix);
	OUTPUT.posH = mul(OUTPUT.posH, SCENE_DATA[0].pMatrix);

	return OUTPUT;
}