#pragma pack_matrix(row_major)

cbuffer PerObject : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

cbuffer DecalConstants : register(b1)
{
    matrix DecalWorld;
    matrix DecalVP;
}


struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    float4 Color : COLOR;
};

struct VS_OUT
{
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD;
};

VS_OUT mainVS(VS_IN Input) 
{
    VS_OUT OUTPUT;
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);
    float4 ViewPosition = mul(WorldPosition, View);
    
    OUTPUT.Position = mul(ViewPosition, Projection);
    OUTPUT.WorldPosition = WorldPosition.xyz;
    return OUTPUT;
}

float4 mainPS(VS_OUT Input) : SV_Target
{
    float4 DecalClip = mul(float4(Input.WorldPosition, 1), DecalVP);
    
    return float4(DecalClip.x*0.5+0.5, DecalClip.y*0.5+0.5  , 0, 1); // 상수버퍼 제대로 넘어갔나 실험용!
}