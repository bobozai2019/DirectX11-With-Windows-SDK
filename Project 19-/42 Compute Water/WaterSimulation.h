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
    float damp = 1;
    float padding[2] = {};
    DirectX::XMFLOAT4 drop = {0, 0, 0, 0};
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
    Ptr<ID3D11ComputeShader> computeCS;
    Ptr<ID3D11Buffer> constants;
    UINT size = 0;
    int current = 0, previous = 1, next = 2;
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
    UINT Size() const
    {
        return size;
    }
    std::vector<DirectX::XMFLOAT2> Readback();
};
