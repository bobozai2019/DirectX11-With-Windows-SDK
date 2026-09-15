#include "Surface.hlsli"
Surface VS(uint id:SV_VertexID) {
    const uint2 corners[6]={uint2(0,0),uint2(1,0),uint2(0,1),uint2(0,1),uint2(1,0),uint2(1,1)};
    uint cell=id/6; float2 uv=(float2(cell%128,cell/128)+corners[id%6])/128;
    Surface o;o.uv=uv;o.normal=float3(0,1,0);float3 pos=float3((uv.x-.5)*22,0,(uv.y-.5)*22);
    if(Object.w<.5) {
        if(Hull.SampleLevel(LinearClamp,uv,0).r==0) {
            float edge=saturate(min(min(uv.x,uv.y),min(1-uv.x,1-uv.y))*30);
            pos.y=Info.w*H(uv)*edge;
        }
    } else if(Object.w<1.5) {
        // Four vertical walls share the same procedural grid.
        float f=uv.x*4;uint side=min((uint)f,3);float t=frac(f);
        float2 xz=side==0?float2(t,0):side==1?float2(1,t):side==2?float2(1-t,1):float2(0,1-t);
        pos=float3((xz.x-.5)*22,-uv.y*3,(xz.y-.5)*22);
        o.normal=side==0?float3(0,0,-1):side==1?float3(1,0,0):side==2?float3(0,0,1):float3(-1,0,0);
    } else {
        float phi=uv.x*6.2831853,theta=uv.y*3.14159265;
        float3 sphere=float3(cos(phi)*sin(theta),cos(theta),sin(phi)*sin(theta));
        float3 scale=Object.w<2.5?float3(2.5,1.1,2.4):float3(1.54,.35,.572);
        pos=sphere*scale; o.normal=normalize(sphere/scale);
        if(Object.w>2.5) { float c=cos(BoatPose.z),s=sin(BoatPose.z);pos.xz=float2(c*pos.x-s*pos.z,s*pos.x+c*pos.z);o.normal.xz=float2(c*o.normal.x-s*o.normal.z,s*o.normal.x+c*o.normal.z); }
        pos+=Object.xyz;
    }
    o.world=pos;o.position=mul(float4(pos,1),ViewProjection);return o;
}
