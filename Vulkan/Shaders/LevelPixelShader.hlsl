// an ultra simple hlsl pixel shader
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
	float4x4 wMatrix[1024];
	OBJ_ATTR material[1024];
};
[[vk::push_constant]]
cbuffer MATRIX_DATA {
	float meshID;
	float materialID;
};

StructuredBuffer<LEVEL_MODEL_DATA> SCENE_DATA;
float4 main(float4 posH : SV_POSITION, float3 nrmW : NORMAL, float3 posW : WORLD) : SV_TARGET
{
	return float4(SCENE_DATA[0].material[materialID].Kd, 1);
}