#include "Simulation.hlsli"
float4 PS(Fullscreen i) : SV_Target {
    // Procedural waterline footprint replaces the demo's hull-camera pass.
    float2 q=i.uv-Boat.xy; float c=cos(Boat.z),s=sin(Boat.z);
    q=float2(c*q.x+s*q.y,-s*q.x+c*q.y)/float2(.070,.026);
    float speed=dot(q,q)<1 ? Boat.w : 0;
    return float4(speed,0,0,1);
}
