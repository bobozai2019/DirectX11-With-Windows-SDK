#pragma once
#include "WaterSimulation.h"
#include <fstream>
class GameApp : public D3DApp
{
  public:
    GameApp(HINSTANCE instance, bool validation);
    bool Init() override;
    void OnResize() override;
    void UpdateScene(float dt) override;
    void DrawScene() override;
    int Validate();

  private:
    struct alignas(16) SceneParams
    {
        DirectX::XMFLOAT4X4 vp;
        DirectX::XMFLOAT4 eye, info, object, boat;
    };
    WaterSimulation water;
    ComPtr<ID3D11VertexShader> waterVS;
    ComPtr<ID3D11PixelShader> waterPS;
    ComPtr<ID3D11Buffer> sceneBuffer;
    ComPtr<ID3D11SamplerState> linear;
    ComPtr<ID3D11RasterizerState> raster;
    ComPtr<ID3D11Texture2D> depth;
    ComPtr<ID3D11DepthStencilView> depthView;
    SimParams params;
    float accumulator = 0, rainTimer = 0;
    float heightScale = 1, yaw = .65f, pitch = .68f, distance = 43;
    bool paused = false, rain = true, validation = false;
    int quality = 1, display = 0, frames = 0;
    unsigned random = 12345;
    DirectX::XMFLOAT2 pending = {-1, -1};
    DirectX::XMMATRIX CameraView() const;
    void Advance(float dt);
    void RenderWater(const D3D11_VIEWPORT &viewport);
    void Reset();
    void Capture(const wchar_t *path);
    void NumericalTests(std::ofstream &out);
};
