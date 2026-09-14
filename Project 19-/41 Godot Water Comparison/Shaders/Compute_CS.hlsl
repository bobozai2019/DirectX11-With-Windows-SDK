#include "Simulation.hlsli"
RWTexture2D<float> Output : register(u0);
[numthreads(8,8,1)]
void CS(uint3 id : SV_DispatchThreadID) {
    if(id.x>=N || id.y>=N) return;
    int2 p=id.xy;
    float v=Current.Load(int3(p,0)).r;
    float sum=Current.Load(int3(Bound(p+int2(1,0)),0)).r+
        Current.Load(int3(Bound(p-int2(1,0)),0)).r+
        Current.Load(int3(Bound(p+int2(0,1)),0)).r+
        Current.Load(int3(Bound(p-int2(0,1)),0)).r;
    float next=2*v-Previous.Load(int3(p,0)).r+.25*(sum-4*v);
    next-=Damp*next*.001;
    if(Drop.z>0 && all(p==int2(Drop.xy))) next=Drop.z;
    Output[p]=max(next,0); // Keep official demo's negative-value clamp.
}
