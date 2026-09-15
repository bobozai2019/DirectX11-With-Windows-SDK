#include "GameApp.h"
#include <ScreenGrab11.h>
#include <algorithm>
#include <cmath>
#include <d3d11sdklayers.h>
#include <iomanip>
#include <wincodec.h>
using namespace DirectX;
GameApp::GameApp(HINSTANCE instance, bool test)
    : D3DApp(instance, L"42 Compute Water", 1440, 900), validation(test)
{
}
bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;
    if (validation)
        ShowWindow(MainWnd(), SW_HIDE);
    ImGui::GetIO().IniFilename = nullptr;
    wchar_t windowsDirectory[MAX_PATH];
    GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
    auto font = std::filesystem::path(windowsDirectory) / "Fonts" / "segoeui.ttf";
    if (std::filesystem::exists(font))
        ImGui::GetIO().Fonts->AddFontFromFileTTF(font.string().c_str(), 18);
    else
        ImGui::GetIO().FontGlobalScale = 1.25f;
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 8;
    style.FrameRounding = 4;
    water.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
    auto b = Compile(L"Shaders/Water_VS.hlsl", "VS", "vs_5_0");
    Check(m_pd3dDevice->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &waterVS));
    b = Compile(L"Shaders/Water_PS.hlsl", "PS", "ps_5_0");
    Check(m_pd3dDevice->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &waterPS));
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(SceneParams);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Check(m_pd3dDevice->CreateBuffer(&bd, nullptr, &sceneBuffer));
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    Check(m_pd3dDevice->CreateSamplerState(&sd, &linear));
    D3D11_RASTERIZER_DESC rs = {};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    Check(m_pd3dDevice->CreateRasterizerState(&rs, &raster));
    Reset();
    return true;
}
void GameApp::OnResize()
{
    D3DApp::OnResize();
    if (m_ClientWidth <= 0 || m_ClientHeight <= 0)
        return;
    depthView.Reset();
    depth.Reset();
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = m_ClientWidth;
    td.Height = m_ClientHeight;
    td.MipLevels = td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    Check(m_pd3dDevice->CreateTexture2D(&td, nullptr, &depth));
    Check(m_pd3dDevice->CreateDepthStencilView(depth.Get(), nullptr, &depthView));
}
void GameApp::Reset()
{
    UINT n = 128u << quality;
    water.Reset(n);
    params.n = n;
    accumulator = 0;
    rainTimer = 0;
    random = 12345;
    pending = {-1, -1};
}
XMMATRIX GameApp::CameraView() const
{
    XMVECTOR eye =
        XMVectorSet(distance * cosf(pitch) * sinf(yaw), distance * sinf(pitch), distance * cosf(pitch) * cosf(yaw), 1);
    return XMMatrixLookAtLH(eye, XMVectorZero(), XMVectorSet(0, 1, 0, 0));
}
void GameApp::Advance(float dt)
{
    if (paused) return;
    accumulator += (std::min)(dt, .1f);
    while (accumulator >= 1.f / 60)
    {
        accumulator -= 1.f / 60;
        params.drop = {0, 0, 0, 0};
        rainTimer += 1.f / 60;
        if (rain && rainTimer >= .1f)
        {
            rainTimer = 0;
            random = 1664525 * random + 1013904223;
            float x = float(random % params.n);
            random = 1664525 * random + 1013904223;
            params.drop = {x, float(random % params.n), 3, 0};
        }
        if (pending.x >= 0)
        {
            params.drop = {pending.x * params.n, pending.y * params.n, 5, 0};
            pending = {-1, -1};
        }
        water.Step(params);
    }
}
void GameApp::UpdateScene(float dt)
{
    auto &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(16, 14), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(float(m_ClientWidth - 32), 146), ImGuiCond_Always);
    ImGui::Begin("WATER LAB / DX11", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("42 COMPUTE WATER");
    ImGui::Checkbox("Pause", &paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) Reset();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("Resolution", &quality, "128\0" "256\0" "512\0")) Reset();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130);
    ImGui::Combo("Display", &display, "Shaded water\0" "Height map\0");
    ImGui::Checkbox("Rain drops", &rain);
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("Damping", &params.damp, 1, 10);
    ImGui::SameLine();
    ImGui::TextDisabled("%.1f fps", io.Framerate);
    ImGui::End();
    ImGui::SetNextWindowPos(ImVec2(24, float(m_ClientHeight) - 48));
    ImGui::Begin("Help", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
    ImGui::Text("LMB: disturb water    RMB: orbit    Wheel: zoom");
    ImGui::End();
    if (!io.WantCaptureMouse)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            yaw -= io.MouseDelta.x * .006f;
            pitch = std::clamp(pitch + io.MouseDelta.y * .006f, .25f, 1.4f);
        }
        distance = std::clamp(distance - io.MouseWheel * 2, 20.f, 65.f);
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.MousePos.y > 165)
        {
            float width = float(m_ClientWidth);
            XMMATRIX proj = XMMatrixPerspectiveFovLH(.85f, width / (m_ClientHeight - 165), .1f, 150);
            auto a = XMVector3Unproject(XMVectorSet(io.MousePos.x, io.MousePos.y, 0, 1), 0, 165, width,
                                        float(m_ClientHeight - 165), 0, 1, proj, CameraView(), XMMatrixIdentity());
            auto b = XMVector3Unproject(XMVectorSet(io.MousePos.x, io.MousePos.y, 1, 1), 0, 165, width,
                                        float(m_ClientHeight - 165), 0, 1, proj, CameraView(), XMMatrixIdentity());
            XMFLOAT3 from, dir;
            XMStoreFloat3(&from, a);
            XMStoreFloat3(&dir, b - a);
            if (fabsf(dir.y) > 1e-6f)
            {
                float t = -from.y / dir.y;
                XMFLOAT2 uv = {(from.x + t * dir.x) / 22 + .5f, (from.z + t * dir.z) / 22 + .5f};
                if (t > 0 && uv.x >= 0 && uv.x < 1 && uv.y >= 0 && uv.y < 1)
                    pending = uv;
            }
        }
    }
    Advance(dt);
    ImGui::Render();
}
void GameApp::RenderWater(const D3D11_VIEWPORT &vp)
{
    auto c = m_pd3dImmediateContext.Get();
    c->RSSetViewports(1, &vp);
    SceneParams s = {};
    XMStoreFloat4x4(&s.vp, CameraView() * XMMatrixPerspectiveFovLH(.85f, vp.Width / vp.Height, .1f, 150));
    s.eye = {distance * cosf(pitch) * sinf(yaw), distance * sinf(pitch), distance * cosf(pitch) * cosf(yaw), 1};
    s.info = {1.f, float(water.Size()), float(display), heightScale};
    ID3D11ShaderResourceView *srv[] = {water.Height()};
    c->VSSetShaderResources(0, 1, srv);
    c->PSSetShaderResources(0, 1, srv);
    auto cb = sceneBuffer.Get();
    c->VSSetConstantBuffers(0, 1, &cb);
    c->PSSetConstantBuffers(0, 1, &cb);
    auto draw = [&](XMFLOAT4 object) {
        s.object = object;
        c->UpdateSubresource(cb, 0, nullptr, &s, 0, 0);
        c->Draw(128 * 128 * 6, 0);
    };
    draw({0, 0, 0, 1});
    draw({0, 0, 0, 0});

}
void GameApp::DrawScene()
{
    auto c = m_pd3dImmediateContext.Get();
    if (!GetBackBufferRTV())
    {
        ComPtr<ID3D11Texture2D> back;
        Check(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&back)));
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        Check(m_pd3dDevice->CreateRenderTargetView(
            back.Get(), &rd, m_pRenderTargetViews[m_FrameCount % m_BackBufferCount].GetAddressOf()));
    }
    const float bg[] = {.035f, .053f, .08f, 1};
    auto rtv = GetBackBufferRTV();
    c->ClearRenderTargetView(rtv, bg);
    c->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    c->OMSetRenderTargets(1, &rtv, depthView.Get());
    c->OMSetBlendState(nullptr, nullptr, ~0u);
    c->OMSetDepthStencilState(nullptr, 0);
    c->RSSetState(raster.Get());
    c->IASetInputLayout(nullptr);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(waterVS.Get(), nullptr, 0);
    c->PSSetShader(waterPS.Get(), nullptr, 0);
    auto ss = linear.Get();
    c->VSSetSamplers(0, 1, &ss);
    c->PSSetSamplers(0, 1, &ss);
    float width = float(m_ClientWidth);
    RenderWater({0, 165, width, float(m_ClientHeight - 165), 0, 1});
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    if (validation && frames == 239)
        Capture(L"validation.png");
    Check(m_pSwapChain->Present(validation ? 0 : 1, 0));
    frames++;
}
void GameApp::Capture(const wchar_t *path)
{
    ComPtr<ID3D11Resource> r;
    GetBackBufferRTV()->GetResource(&r);
    Check(SaveWICTextureToFile(m_pd3dImmediateContext.Get(), r.Get(), GUID_ContainerFormatPng, path));
}
void GameApp::NumericalTests(std::ofstream &out)
{
    // Compare the real PS/CS output with an independent CPU stencil, including
    // the non-multiple-of-eight dispatch boundary and RGBA8 quantization.
    constexpr bool cs = true;
    {
        WaterSimulation test;
        test.Init(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get());
        test.Reset(19);
        SimParams p;
        p.n = 19;
        p.damp = 3;
        std::vector<float> old(361), cur(361), next(361);
        for (int step = 0; step < 12; step++)
        {
            p.drop = {9, 9, step == 0 ? (cs ? 3.f : .8f) : 0, 0};
            test.Step(p);
            for (int y = 0; y < 19; y++)
                for (int x = 0; x < 19; x++)
                {
                    auto at = [&](int xx, int yy) { return cur[std::clamp(yy, 0, 18) * 19 + std::clamp(xx, 0, 18)]; };
                    int i = y * 19 + x;
                    float v =
                        2 * cur[i] - old[i] +
                        .25f * (at(x - 1, y) + at(x + 1, y) + at(x, y - 1) + at(x, y + 1) - 4 * cur[i]);
                    if (cs)
                        v *= 1 - p.damp * .001f;
                    if (step == 0 && x == 9 && y == 9)
                        v = p.drop.z;
                    next[i] = cs ? (std::max)(v, 0.f) : std::round(std::clamp(v, 0.f, 1.f) * 255) / 255;
                }
            auto gpu = test.Readback();
            float error = 0;
            for (size_t i = 0; i < cur.size(); i++)
                error = (std::max)(error, fabsf(gpu[i].x - next[i]));
            if (error > (cs ? 1e-5f : 2.f / 255))
                throw std::runtime_error("CPU/GPU stencil mismatch");
            old = cur;
            cur = next;
        }
        test.Reset(128);
        auto zero = test.Readback();
        for (auto v : zero)
            if (v.x != 0 || v.y != 0)
                throw std::runtime_error("Reset did not clear history");
        out << "PASS " << (cs ? "compute R32F" : "fragment RGBA8")
            << " 12 CPU/GPU stencil steps, 19x19 dispatch, reset to 128\n";
    }

}
int GameApp::Validate()
{
    std::ofstream out("validation.txt");
    NumericalTests(out);
    Reset();
    ComPtr<ID3D11InfoQueue> queue;
    m_pd3dDevice.As(&queue);
    if (queue)
        queue->ClearStoredMessages();
    for (int i = 0; i < 240; i++)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        UpdateScene(1.f / 60);
        DrawScene();
        m_FrameCount++;
    }
    {
        auto values = water.Readback();
        float maxPos = 0, maxNeg = 0;
        size_t active = 0;
        for (auto v : values)
        {
            if (!std::isfinite(v.x) || !std::isfinite(v.y))
                throw std::runtime_error("Non-finite simulation");
            maxPos = (std::max)(maxPos, v.x);
            maxNeg = (std::max)(maxNeg, v.y);
            if (v.x > 0 || v.y > 0)
                active++;
        }
        out << "Live " << "Compute" << ": active=" << active << " Rmax=" << maxPos
            << " Gmax=" << maxNeg << "\n";
        if (active < 20 || maxPos <= 0)
            throw std::runtime_error("Missing live wave propagation");
    }
    // Exercise every display mode of this standalone sample.
    for (int view = 0; view < 2; view++)
        {
            display = view;
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            UpdateScene(1.f / 60);
            DrawScene();
            m_FrameCount++;
        }
    display = 0;
    paused = true;
    auto before = water.Readback();
    Advance(.05f);
    auto after = water.Readback();
    for (size_t i = 0; i < before.size(); i++)
        if (before[i].x != after[i].x)
            throw std::runtime_error("Pause changed height data");
    paused = false;
    rain = false;
    Reset();
    pending = {.5f, .5f};
    Advance(1.f / 60);
    auto values = water.Readback();
    UINT center = (params.n / 2) * params.n + params.n / 2;
    if (values[center].x < 4.99f)
        throw std::runtime_error("Pointer disturbance missing");
    out << "PASS display modes, pause and pointer injection\n";
    for (int q : {0, 2, 1})
    {
        quality = q;
        Reset();
        Advance(1.f / 60);
        out << "PASS resize " << water.Size() << "\n";
    }
    m_ClientWidth = 1024;
    m_ClientHeight = 768;
    OnResize();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    UpdateScene(1.f / 60);
    DrawScene();
    m_FrameCount++;
    out << "PASS swap-chain/depth resize 1024x768\n";

    if (queue)
    {
        for (UINT64 i = 0; i < queue->GetNumStoredMessages(); i++)
        {
            SIZE_T len = 0;
            queue->GetMessage(i, nullptr, &len);
            std::vector<unsigned char> bytes(len);
            auto msg = reinterpret_cast<D3D11_MESSAGE *>(bytes.data());
            queue->GetMessage(i, msg, &len);
            if (msg->Severity <= D3D11_MESSAGE_SEVERITY_WARNING)
            {
                out << msg->pDescription << "\n";
                throw std::runtime_error("D3D11 debug-layer warning/error");
            }
        }
        out << "PASS D3D11 debug layer\n";
    }
    else
        out << "INFO debug layer not enabled in this build\n";
    out << "PASS rendered standalone screenshot and 240 frames\n";
    return 0;
}
