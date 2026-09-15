#pragma once
#include <array>
#include <d3dApp.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <stdexcept>
#include <vector>
void Check(HRESULT hr);
Microsoft::WRL::ComPtr<ID3DBlob> Compile(const wchar_t *file, const char *entry, const char *profile);
struct alignas(16) SimParams
{
    UINT n = 256;
    float a = .077f, amplitude = .5f, damp = 1;
    DirectX::XMFLOAT4 boat = {.5f, .5f, 0, .8f};
    DirectX::XMFLOAT4 drop = {0, 0, 0, 0};
    UINT land = 1;
    float padding[3] = {};
};
class WaterSimulation
{
    template <class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    struct Texture
    {
        Ptr<ID3D11Texture2D> tex;
        Ptr<ID3D11ShaderResourceView> srv;
        Ptr<ID3D11RenderTargetView> rtv;
        Ptr<ID3D11UnorderedAccessView> uav;
    };
    Ptr<ID3D11Device> device;
    Ptr<ID3D11DeviceContext> context;
    std::array<Texture, 3> height;
    std::array<Texture, 2> collision;
    Ptr<ID3D11VertexShader> fullVS;
    Ptr<ID3D11PixelShader> dynamicPS, collisionPS;
    Ptr<ID3D11Buffer> constants;
    Ptr<ID3D11SamplerState> point;
    Ptr<ID3D11RasterizerState> raster;
    Ptr<ID3D11ShaderResourceView> land;
    UINT size = 0;
    int current = 0, previous = 1, next = 2, collisionIndex = 0;
    Texture MakeTexture(DXGI_FORMAT format, UINT flags);

  public:
    void Init(ID3D11Device *d, ID3D11DeviceContext *c);
    void Reset(UINT n);
    void Step(const SimParams &params);
    void Unbind();
    ID3D11ShaderResourceView *Height() const
    {
        return height[current].srv.Get();
    }
    ID3D11ShaderResourceView *Collision() const
    {
        return collision[collisionIndex].srv.Get();
    }
    ID3D11ShaderResourceView *Land() const
    {
        return land.Get();
    }
    UINT Size() const
    {
        return size;
    }
    std::vector<DirectX::XMFLOAT2> Readback();
};
