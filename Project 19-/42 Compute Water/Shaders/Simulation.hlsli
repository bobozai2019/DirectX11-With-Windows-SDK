// Port of Godot Compute Texture (MIT).
cbuffer Simulation : register(b0) {
    uint N; float Damp; float2 Padding;
    float4 Drop; // pixel xy, amplitude, unused
};
Texture2D<float4> Current : register(t0);
Texture2D<float4> Previous : register(t1);
int2 Bound(int2 p) { return clamp(p, int2(0,0), int2(N-1,N-1)); }
struct Fullscreen { float4 position : SV_Position; float2 uv : TEXCOORD; };
