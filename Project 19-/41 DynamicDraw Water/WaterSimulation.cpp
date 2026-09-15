#include "WaterSimulation.h"
#include <WICTextureLoader11.h>
#include <string>
using namespace DirectX;
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr)
{
    if (FAILED(hr))
        throw std::runtime_error("D3D11 failure: " + std::to_string(static_cast<unsigned long>(hr)));
}
ComPtr<ID3DBlob> Compile(const wchar_t *file, const char *entry, const char *profile)
{
    ComPtr<ID3DBlob> code, error;
    HRESULT hr = D3DCompileFromFile(file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, profile,
                                    D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &error);
    if (FAILED(hr))
        throw std::runtime_error(error ? std::string((char *)error->GetBufferPointer(), error->GetBufferSize())
                                       : "Missing shader file");
    return code;
}
void WaterSimulation::Init(ID3D11Device *d, ID3D11DeviceContext *c)
{
    device = d;
    context = c;
    auto b = Compile(L"Shaders/Fullscreen_VS.hlsl", "VS", "vs_5_0");
    Check(d->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &fullVS));
    b = Compile(L"Shaders/Dynamic_PS.hlsl", "PS", "ps_5_0");
    Check(d->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &dynamicPS));
    b = Compile(L"Shaders/Collision_PS.hlsl", "PS", "ps_5_0");
    Check(d->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &collisionPS));
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(SimParams);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Check(d->CreateBuffer(&bd, nullptr, &constants));
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    Check(d->CreateSamplerState(&sd, &point));
    D3D11_RASTERIZER_DESC rs = {};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    Check(d->CreateRasterizerState(&rs, &raster));
    Check(CreateWICTextureFromFile(d, L"Assets/land.png", nullptr, &land));
}
WaterSimulation::Texture WaterSimulation::MakeTexture(DXGI_FORMAT format, UINT flags)
{
    Texture t;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = td.Height = size;
    td.MipLevels = td.ArraySize = 1;
    td.Format = format;
    td.SampleDesc.Count = 1;
    td.BindFlags = flags | D3D11_BIND_SHADER_RESOURCE;
    Check(device->CreateTexture2D(&td, nullptr, &t.tex));
    Check(device->CreateShaderResourceView(t.tex.Get(), nullptr, &t.srv));
    if (flags & D3D11_BIND_RENDER_TARGET)
        Check(device->CreateRenderTargetView(t.tex.Get(), nullptr, &t.rtv));
    if (flags & D3D11_BIND_UNORDERED_ACCESS)
        Check(device->CreateUnorderedAccessView(t.tex.Get(), nullptr, &t.uav));
    const float zero[4] = {};
    if (t.rtv)
        context->ClearRenderTargetView(t.rtv.Get(), zero);
    if (t.uav)
        context->ClearUnorderedAccessViewFloat(t.uav.Get(), zero);
    return t;
}
void WaterSimulation::Unbind()
{
    ID3D11ShaderResourceView *empty[5] = {};
    context->VSSetShaderResources(0, 5, empty);
    context->PSSetShaderResources(0, 5, empty);
    context->CSSetShaderResources(0, 5, empty);
    ID3D11UnorderedAccessView *u = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &u, nullptr);
    context->OMSetRenderTargets(0, nullptr, nullptr);
}
void WaterSimulation::Reset(UINT n)
{
    Unbind();
    size = n;
    current = 0;
    previous = 1;
    next = 2;
    collisionIndex = 0;
    for (auto &t : height)
        t = MakeTexture(DXGI_FORMAT_R8G8B8A8_UNORM,
                        D3D11_BIND_RENDER_TARGET);
    for (auto &t : collision)
        t = MakeTexture(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
}
void WaterSimulation::Step(const SimParams &params)
{
    Unbind();
    context->UpdateSubresource(constants.Get(), 0, nullptr, &params, 0, 0);
    ID3D11Buffer *cb = constants.Get();
    context->PSSetConstantBuffers(0, 1, &cb);
    D3D11_VIEWPORT vp = {0, 0, float(size), float(size), 0, 1};
    context->RSSetViewports(1, &vp);
    context->RSSetState(raster.Get());
    context->OMSetBlendState(nullptr, nullptr, ~0u);
    context->OMSetDepthStencilState(nullptr, 0);
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(fullVS.Get(), nullptr, 0);
    {
        int nc = 1 - collisionIndex;
        auto rtv = collision[nc].rtv.Get();
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->PSSetShader(collisionPS.Get(), nullptr, 0);
        context->Draw(3, 0);
        context->OMSetRenderTargets(0, nullptr, nullptr);
        ID3D11ShaderResourceView *views[] = {height[current].srv.Get(), height[previous].srv.Get(),
                                             collision[nc].srv.Get(), collision[collisionIndex].srv.Get(), land.Get()};
        context->PSSetShaderResources(0, 5, views);
        auto sampler = point.Get();
        context->PSSetSamplers(0, 1, &sampler);
        rtv = height[next].rtv.Get();
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->PSSetShader(dynamicPS.Get(), nullptr, 0);
        context->Draw(3, 0);
        collisionIndex = nc;
    }
    Unbind();
    context->CSSetShader(nullptr, nullptr, 0);
    int free = previous;
    previous = current;
    current = next;
    next = free;
}
std::vector<XMFLOAT2> WaterSimulation::Readback()
{
    Unbind();
    D3D11_TEXTURE2D_DESC td;
    height[current].tex->GetDesc(&td);
    td.BindFlags = 0;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    Check(device->CreateTexture2D(&td, nullptr, &staging));
    context->CopyResource(staging.Get(), height[current].tex.Get());
    D3D11_MAPPED_SUBRESOURCE map;
    Check(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map));
    std::vector<XMFLOAT2> data(size * size);
    for (UINT y = 0; y < size; y++)
        for (UINT x = 0; x < size; x++)
        {
            auto row = (unsigned char *)map.pData + y * map.RowPitch;
            data[y * size + x] =
                XMFLOAT2(row[x * 4] / 255.f, row[x * 4 + 1] / 255.f);
        }
    context->Unmap(staging.Get(), 0);
    return data;
}
