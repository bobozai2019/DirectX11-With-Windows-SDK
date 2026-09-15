#include "Surface.hlsli"
float4 PS(Surface i):SV_Target {
    float3 n=normalize(i.normal),v=normalize(Eye.xyz-i.world),l=normalize(float3(-.4,.85,-.3));
    float height=H(i.uv);
    if(Object.w<.5) {
        if(Info.z>.5) { float2 rg=Height.SampleLevel(LinearClamp,i.uv,0).rg;return float4(rg.rrr*.7,1); }
        float eps=1/Info.y;
        float dx=H(i.uv+float2(eps,0))-height,dz=H(i.uv+float2(0,eps))-height;
        // Same two gradient samples as the Godot material shaders.
        n=normalize(float3(-dx,1,-dz));
        float fresnel=.04+.96*pow(1-saturate(dot(n,v)),5);
        float3 reflection=Sky(reflect(-v,n));
        float spec=pow(saturate(dot(reflect(-l,n),v)),100);
        return float4(float3(.015,.09,.14)*(.4+.6*dot(n,l))+reflection*(.50+.45*fresnel)+spec*.8,1);
    }
    float3 color=float3(.025,.08,.12);
    return float4(color*(.35+.65*saturate(dot(n,l))),1);
}
