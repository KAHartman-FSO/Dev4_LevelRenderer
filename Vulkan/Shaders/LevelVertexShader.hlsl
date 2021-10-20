// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major)
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
	float4x4 wMatrix;
	float4x4 vp_Matrix;
};
Vertex_Output main(Vertex_Input INPUT)
{
	Vertex_Output OUTPUT;
	OUTPUT.posH = float4(INPUT.pos, 1);
	OUTPUT.posH = mul(OUTPUT.posH, wMatrix);
	OUTPUT.posH = mul(OUTPUT.posH, vp_Matrix);
	OUTPUT.nrmW = INPUT.nrm;

	return OUTPUT;
}