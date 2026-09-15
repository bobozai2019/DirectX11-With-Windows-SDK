#include "Simulation.hlsli"
float4 PS(Fullscreen i) : SV_Target {
    int2 p=int2(i.position.xy);
    float2 z=A*(Current.Load(int3(Bound(p+int2(1,0)),0)).rg+
        Current.Load(int3(Bound(p-int2(1,0)),0)).rg+
        Current.Load(int3(Bound(p+int2(0,1)),0)).rg+
        Current.Load(int3(Bound(p-int2(0,1)),0)).rg)+
        (2-4*A)*Current.Load(int3(p,0)).rg-Previous.Load(int3(p,0)).rg;
    float now=Collision.Load(int3(p,0)).r, old=OldCollision.Load(int3(p,0)).r;
    if(now>0 && old==0) z.r=Amplitude*now;
    else if(now==0 && old>0) z.g=Amplitude*old;
    // Optional mouse disturbance; the hull-driven path is unchanged.
    if(Drop.z>0 && all(p==int2(Drop.xy))) z.r=Drop.z;
    if(UseLand && Land.SampleLevel(PointClamp,i.uv,0).r>0) z=0;
    return float4(z,0,1); // RGBA8 target performs original UNORM clamping.
}
