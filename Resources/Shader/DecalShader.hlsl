#pragma pack_matrix(row_major)

cbuffer PerObject : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};


struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    float4 Color : COLOR;
};

float4 mainVS(VS_IN Input) : SV_POSITION
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);
    float4 ViewPosition = mul(WorldPosition, View);
    return mul(ViewPosition, Projection);
}

float4 mainPS() : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}