#include "Surface.hlsli"
float4 PS(Surface i):SV_Target {
    float3 n=normalize(i.normal),v=normalize(Eye.xyz-i.world),l=normalize(float3(-.4,.85,-.3));
    float height=H(i.uv);
    if(Object.w<.5) {
        if(Info.z>1.5) { float hull=Hull.SampleLevel(LinearClamp,i.uv,0).r;return float4(hull,0,0,1); }
        if(Info.z>.5) { float2 rg=Height.SampleLevel(LinearClamp,i.uv,0).rg;return Info.x<.5?float4(rg*3,0,1):float4(rg.rrr*.7,1); }
        float eps=Info.x<.5?.01:1/Info.y;
        float dx=H(i.uv+float2(eps,0))-height,dz=H(i.uv+float2(0,eps))-height;
        // Same two gradient samples as the Godot material shaders.
        n=normalize(float3(-dx,1,-dz));
        if(Info.x<.5) {
            float fresnel=sqrt(saturate(1-dot(n,v)));
            float3 albedo=float3(.09,.533,.380)+.1*fresnel;
            float spec=pow(saturate(dot(reflect(-l,n),v)),110);
            float3 col=albedo*(.55+.6*saturate(dot(n,l)))+.4*spec;
            // Opaque compositing over a matching colored water block.
            return float4(col,1);
        }
        float fresnel=.04+.96*pow(1-saturate(dot(n,v)),5);
        float3 reflection=Sky(reflect(-v,n));
        float spec=pow(saturate(dot(reflect(-l,n),v)),100);
        return float4(float3(.015,.09,.14)*(.4+.6*dot(n,l))+reflection*(.50+.45*fresnel)+spec*.8,1);
    }
    float3 color=Object.w<1.5 ? (Info.x<.5?float3(.04,.31,.22):float3(.025,.08,.12)) : Object.w<2.5?float3(.88,.73,.32):float3(1,.48,.16);
    if(Object.w>2.5 && i.normal.y>.5)color=lerp(color,float3(.95,.91,.75),.5);
    return float4(color*(.35+.65*saturate(dot(n,l))),1);
}
