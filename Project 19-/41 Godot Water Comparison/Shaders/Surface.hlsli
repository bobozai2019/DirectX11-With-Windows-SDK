cbuffer Scene : register(b0) {
    row_major float4x4 ViewProjection;
    float4 Eye;
    float4 Info; // mode, texture resolution, display mode, displacement amplitude
    float4 Object; // center xyz, type: 0 water / 1 sides / 2 island / 3 boat
    float4 BoatPose;
};
Texture2D<float4> Height : register(t0);
Texture2D<float4> Hull : register(t1);
Texture2D<float4> Land : register(t2);
SamplerState LinearClamp : register(s0);
struct Surface { float4 position:SV_Position; float3 world:TEXCOORD0;float2 uv:TEXCOORD1;float3 normal:TEXCOORD2; };
float H(float2 uv) { float2 rg=Height.SampleLevel(LinearClamp,uv,0).rg; return Info.x<.5 ? rg.x-rg.y : rg.x; }
float3 Sky(float3 r) {
    float horizon=pow(1-saturate(abs(r.y)),5);
    float3 col=lerp(float3(.12,.23,.40),float3(.52,.73,.87),saturate(r.y*.8+.3));
    col=lerp(col,float3(.98,.74,.48),horizon*.45);
    float sun=pow(saturate(dot(r,normalize(float3(-.4,.8,-.2)))),180);
    return col+float3(1,.86,.65)*sun*3;
}
