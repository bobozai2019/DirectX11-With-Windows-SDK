#include "WaterSimulation.h"
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
    auto b = Compile(L"Shaders/Compute_CS.hlsl", "CS", "cs_5_0");
    Check(d->CreateComputeShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &computeCS));
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(SimParams);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Check(d->CreateBuffer(&bd, nullptr, &constants));

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
    for (auto &t : height)
        t = MakeTexture(DXGI_FORMAT_R32_FLOAT,
                        D3D11_BIND_UNORDERED_ACCESS);
}
void WaterSimulation::Step(const SimParams &params)
{
    Unbind();
    context->UpdateSubresource(constants.Get(), 0, nullptr, &params, 0, 0);
    ID3D11Buffer *cb = constants.Get();
    context->CSSetConstantBuffers(0, 1, &cb);
    {
        ID3D11ShaderResourceView *views[] = {height[current].srv.Get(), height[previous].srv.Get()};
        context->CSSetShaderResources(0, 2, views);
        auto uav = height[next].uav.Get();
        context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        context->CSSetShader(computeCS.Get(), nullptr, 0);
        context->Dispatch((size + 7) / 8, (size + 7) / 8, 1);
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
                XMFLOAT2(((float *)row)[x], 0);
        }
    context->Unmap(staging.Get(), 0);
    return data;
}
