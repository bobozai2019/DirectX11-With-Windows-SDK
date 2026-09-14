// Ports of John Wigg DynamicWaterDemo and Godot Compute Texture (MIT).
cbuffer Simulation : register(b0) {
    uint N; float A; float Amplitude; float Damp;
    float4 Boat; // uv, angle, speed encoding
    float4 Drop; // pixel xy, amplitude, unused
    uint UseLand; float3 Padding;
};
Texture2D<float4> Current : register(t0);
Texture2D<float4> Previous : register(t1);
Texture2D<float4> Collision : register(t2);
Texture2D<float4> OldCollision : register(t3);
Texture2D<float4> Land : register(t4);
SamplerState PointClamp : register(s0);
int2 Bound(int2 p) { return clamp(p, int2(0,0), int2(N-1,N-1)); }
struct Fullscreen { float4 position : SV_Position; float2 uv : TEXCOORD; };
