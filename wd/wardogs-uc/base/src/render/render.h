#pragma once
#include <Windows.h>
#include <ShellAPI.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <dwmapi.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../util/settings.h"
#include "../util/config.h"
#include "esp.h"
#include "fonts_resource.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline HWND g_overlayHwnd = nullptr;
inline HWND g_gameHwnd = nullptr;

inline ImFont* LoadFontFromResource(ImGuiIO& io, int resourceId, float sizePixels, bool hudQuality = false) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return nullptr;
    HGLOBAL hMemory = LoadResource(NULL, hResource);
    if (!hMemory) return nullptr;
    DWORD dwSize = SizeofResource(NULL, hResource);
    LPVOID lpAddress = LockResource(hMemory);
    if (!lpAddress || dwSize == 0) return nullptr;
    unsigned char* fontData = new unsigned char[dwSize];
    memcpy(fontData, lpAddress, dwSize);
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = true;
    if (hudQuality) {
        fontConfig.OversampleH = 3;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = true;
        fontConfig.RasterizerMultiply = 1.15f;
    }
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(fontData, (int)dwSize, sizePixels, &fontConfig);
    return font;
}

namespace d3d {
    inline ID3D11Device* pDevice = nullptr;
    inline ID3D11DeviceContext* pContext = nullptr;
    inline IDXGISwapChain* pSwapChain = nullptr;
    inline ID3D11RenderTargetView* pRenderTarget = nullptr;
    inline ImFont* fortniteFont = nullptr;
    inline ImFont* hudFont = nullptr;

    inline void ApplyStyle() {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        fortniteFont = LoadFontFromResource(io, IDR_FONT_FORTNITE, 18.0f);
        if (!fortniteFont) fortniteFont = io.Fonts->Fonts[0];
        hudFont = LoadFontFromResource(io, IDR_FONT_FORTNITE, 28.0f, true);
        if (!hudFont) hudFont = fortniteFont;
        io.Fonts->Build();
        ImGui::StyleColorsDark();
    }

    inline bool Init(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        D3D_FEATURE_LEVEL level;
        if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, &level, &pContext)))
            return false;

        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget);
        pBackBuffer->Release();

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(pDevice, pContext);
        ApplyStyle();
        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();

        return true;
    }

    inline void Resize(UINT width, UINT height) {
        if (!pSwapChain || width == 0 || height == 0) return;
        if (pRenderTarget) {
            pRenderTarget->Release();
            pRenderTarget = nullptr;
        }
        pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget);
            pBackBuffer->Release();
        }
    }

    inline void Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (pRenderTarget) pRenderTarget->Release();
        if (pSwapChain) pSwapChain->Release();
        if (pContext) pContext->Release();
        if (pDevice) pDevice->Release();
    }
}

namespace colors {
    inline ImColor bg = ImColor(16, 16, 18, 255);
    inline ImColor sidebar = ImColor(14, 14, 16, 255);
    inline ImColor accent = ImColor(155, 150, 219, 255);
    inline ImColor accentDark = ImColor(100, 95, 160, 255);
    inline ImColor textPrimary = ImColor(255, 255, 255, 255);
    inline ImColor textSecondary = ImColor(140, 140, 150, 255);
    inline ImColor separator = ImColor(255, 255, 255, 8);
    inline ImColor footer = ImColor(12, 12, 14, 255);
}

namespace menu {
    inline bool wasMenuOpen = false;
    inline int currentTab = 0;

    inline void ApplyModernStyle() {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 6.0f;
        s.FrameRounding = 3.0f;
        s.GrabRounding = 2.0f;
        s.ChildRounding = 4.0f;
        s.PopupRounding = 4.0f;
        s.ScrollbarRounding = 3.0f;
        s.WindowPadding = ImVec2(0, 0);
        s.FramePadding = ImVec2(10, 7);
        s.ItemSpacing = ImVec2(10, 10);
        s.ItemInnerSpacing = ImVec2(8, 6);
        s.WindowBorderSize = 0.0f;
        s.FrameBorderSize = 0.0f;
        s.ChildBorderSize = 0.0f;
        s.ScrollbarSize = 8.0f;
        s.GrabMinSize = 10.0f;

        ImVec4* c = s.Colors;
        const ImVec4 bg(0.07f, 0.075f, 0.085f, 0.98f);
        const ImVec4 panel(0.10f, 0.105f, 0.12f, 1.0f);
        const ImVec4 muted(0.55f, 0.56f, 0.58f, 1.0f);
        const ImVec4 text(0.92f, 0.93f, 0.94f, 1.0f);
        const ImVec4 acc(accentColor[0], accentColor[1], accentColor[2], accentColor[3]);

        c[ImGuiCol_WindowBg] = bg;
        c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg] = panel;
        c[ImGuiCol_Border] = ImVec4(1, 1, 1, 0.04f);
        c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.145f, 0.16f, 1.0f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.185f, 0.20f, 1.0f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.205f, 0.22f, 1.0f);
        c[ImGuiCol_CheckMark] = acc;
        c[ImGuiCol_SliderGrab] = acc;
        c[ImGuiCol_SliderGrabActive] = ImVec4(acc.x * 1.1f, acc.y * 1.1f, acc.z * 1.1f, 1.0f);
        c[ImGuiCol_Button] = ImVec4(0.16f, 0.165f, 0.18f, 1.0f);
        c[ImGuiCol_ButtonHovered] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_ButtonActive] = ImVec4(acc.x, acc.y, acc.z, 0.55f);
        c[ImGuiCol_Header] = ImVec4(acc.x, acc.y, acc.z, 0.15f);
        c[ImGuiCol_HeaderHovered] = ImVec4(acc.x, acc.y, acc.z, 0.25f);
        c[ImGuiCol_HeaderActive] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
        c[ImGuiCol_Separator] = ImVec4(1, 1, 1, 0.06f);
        c[ImGuiCol_Text] = text;
        c[ImGuiCol_TextDisabled] = muted;
        c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.12f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.20f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(acc.x, acc.y, acc.z, 0.6f);
    }

    inline bool NavItem(const char* label, int tabIndex, float width) {
        const bool active = (currentTab == tabIndex);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float h = 40.0f;

        ImGui::InvisibleButton(label, ImVec2(width, h));
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        if (clicked) currentTab = tabIndex;

        if (active)
            dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(255, 255, 255, 10));
        else if (hovered)
            dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(255, 255, 255, 6));

        if (active) {
            dl->AddRectFilled(
                ImVec2(p.x, p.y + 8), ImVec2(p.x + 3, p.y + h - 8),
                IM_COL32((int)(accentColor[0]*255), (int)(accentColor[1]*255), (int)(accentColor[2]*255), 255));
        }

        ImU32 textCol = active
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(160, 162, 168, 255);
        dl->AddText(ImVec2(p.x + 22, p.y + 11), textCol, label);
        return clicked;
    }

    inline void SectionHeader(const char* title) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(accentColor[0], accentColor[1], accentColor[2], 1.0f));
        if (d3d::fortniteFont) ImGui::PushFont(d3d::fortniteFont);
        ImGui::TextUnformatted(title);
        if (d3d::fortniteFont) ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 0.08f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    inline void ToggleRow(const char* label, bool* v) {
        if (ImGui::Checkbox(label, v)) config::MarkSettingsChanged();
    }

    inline void ColorToggle(const char* label, bool* v, float color[4]) {
        if (ImGui::Checkbox(label, v)) config::MarkSettingsChanged();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x > 40 ? ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 28 : 0);
        char id[64];
        std::snprintf(id, sizeof(id), "##%sCol", label);
        if (ImGui::ColorEdit4(id, color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel))
            config::MarkSettingsChanged();
    }

    inline void Render() {
        static bool styleApplied = false;
        static bool configInitialized = false;
        static float lastAccent[4] = {0, 0, 0, 0};

        bool colorsChanged = false;
        for (int i = 0; i < 4; i++) {
            if (lastAccent[i] != accentColor[i]) {
                colorsChanged = true;
                lastAccent[i] = accentColor[i];
            }
        }
        if (!styleApplied || colorsChanged) {
            ApplyModernStyle();
            styleApplied = true;
        }

        if (!configInitialized) {
            config::CreateDefaultConfig();
            config::LoadConfig(config::currentConfig);
            configInitialized = true;
        }
        config::UpdateAutoSave(ImGui::GetIO().DeltaTime);

        if (GetAsyncKeyState(VK_INSERT) & 1) showMenu = !showMenu;

        if (showMenu != wasMenuOpen) {
            wasMenuOpen = showMenu;
            LONG style = GetWindowLong(g_overlayHwnd, GWL_EXSTYLE);
            if (showMenu) {
                SetWindowLong(g_overlayHwnd, GWL_EXSTYLE, (style & ~WS_EX_TRANSPARENT & ~WS_EX_NOACTIVATE));
                SetForegroundWindow(g_overlayHwnd);
            } else {
                SetWindowLong(g_overlayHwnd, GWL_EXSTYLE, style | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
            }
        }

        if (!showMenu) return;

        const float winW = 820.0f;
        const float winH = 560.0f;
        const float sideW = 180.0f;

        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((Width - winW) * 0.5f, (Height - winH) * 0.5f), ImGuiCond_FirstUseEver);
        ImGui::Begin("##MainMenu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(14, 15, 18, 250), 6.0f);
        dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(255, 255, 255, 18), 6.0f);

        dl->AddRectFilled(wp, ImVec2(wp.x + sideW, wp.y + ws.y), IM_COL32(10, 11, 13, 255), 6.0f, ImDrawFlags_RoundCornersLeft);
        dl->AddLine(ImVec2(wp.x + sideW, wp.y + 8), ImVec2(wp.x + sideW, wp.y + ws.y - 8), IM_COL32(255, 255, 255, 14));

        ImGui::SetCursorPos(ImVec2(18, 22));
        if (d3d::fortniteFont) ImGui::PushFont(d3d::fortniteFont);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(accentColor[0], accentColor[1], accentColor[2], 1.0f));
        ImGui::TextUnformatted("WARDOGS");
        ImGui::PopStyleColor();
        if (d3d::fortniteFont) ImGui::PopFont();
        ImGui::SetCursorPos(ImVec2(18, 46));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.46f, 0.48f, 1.0f));
        ImGui::TextUnformatted("COMBAT SUITE");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(0, 88));
        NavItem("COMBAT", 0, sideW);
        NavItem("VISUALS", 1, sideW);
        NavItem("VEHICLES", 2, sideW);
        NavItem("SYSTEM", 3, sideW);
        NavItem("CONFIG", 4, sideW);

        ImGui::SetCursorPos(ImVec2(18, ws.y - 36));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.36f, 0.38f, 1.0f));
        ImGui::TextUnformatted("INSERT  toggle");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(sideW + 24, 20));
        ImGui::BeginChild("##Content", ImVec2(ws.x - sideW - 40, ws.y - 40), false);

        if (currentTab == 0) {
            SectionHeader("AIM ASSIST");
            ToggleRow("Enable Aimbot", &aimbotEnabled);
            ToggleRow("Magic Bullet", &magicBullet);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("While firing (LMB): bend bullets to nearest on-screen enemy — no aim key needed");
            if (aimbotEnabled) {
                ToggleRow("Team Check", &teamCheckAim);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Skip Lonestar / Valkyra / Manticore teammates");
                ToggleRow("Lock Target", &aimbotLockTarget);
                ToggleRow("Visible Only", &aimbotVisibleOnly);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("LastRenderTime check — skip targets behind walls");
                ToggleRow("ESP Vis Dim", &visCheckEsp);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Dim ESP for players not recently rendered");
                ToggleRow("Adaptive Smooth", &aimbotAdaptiveSmooth);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snappier when far, precise when close");
                ToggleRow("Lead Prediction", &aimbotPrediction);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("ON = ballistic lead on movers\nOFF = aim straight at the bone (no lead)");
                if (aimbotPrediction) {
                    ToggleRow("Weapon Bullet Speed", &useWeaponBulletSpeed);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Read TypicalSpeed from equipped weapon (fallback: slider)");
                    ColorToggle("Prediction Dot", &drawHeadPrediction, headPredictionColor);
                    if (drawHeadPrediction) {
                        if (ImGui::SliderFloat("Dot Radius", &predictionDotRadius, 1.0f, 8.0f, "%.1f")) config::MarkSettingsChanged();
                        ToggleRow("Show Lead Details", &showPredictionDetails);
                    }
                }

                ImGui::Spacing();
                ColorToggle("FOV Circle", &drawFovCircle, fovCircleColor);
                ColorToggle("Crosshair", &showCrosshair, crosshairColor);
                if (showCrosshair) {
                    if (ImGui::SliderFloat("Crosshair Size", &crosshairSize, 4.0f, 24.0f, "%.0f")) config::MarkSettingsChanged();
                    if (ImGui::SliderFloat("Crosshair Gap", &crosshairGap, 0.0f, 12.0f, "%.0f")) config::MarkSettingsChanged();
                }
                ColorToggle("Aim Preview Line", &showAimLine, aimLineColor);
                if (showAimLine) {
                    if (ImGui::SliderFloat("Line Thickness", &aimLineThickness, 0.5f, 4.0f, "%.1f")) config::MarkSettingsChanged();
                }

                ImGui::Spacing();
                if (ImGui::SliderFloat("FOV Radius", &aimbotFov, 20.0f, 400.0f, "%.0f px")) config::MarkSettingsChanged();
                if (ImGui::SliderFloat("Smoothing", &aimbotSmooth, 0.5f, 20.0f, "%.1f")) config::MarkSettingsChanged();
                if (ImGui::SliderFloat("Deadzone", &aimbotDeadzone, 0.0f, 8.0f, "%.1f px")) config::MarkSettingsChanged();
                if (aimbotPrediction) {
                    if (ImGui::SliderFloat("Bullet Speed Fallback", &aimbotBulletSpeed, 100.0f, 1500.0f, "%.0f m/s")) config::MarkSettingsChanged();
                    if (ImGui::SliderFloat("Bullet Gravity", &bulletGravityCm, 0.0f, 2000.0f, "%.0f cm/s2")) config::MarkSettingsChanged();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drop compensation for PredictHeadAim (980 = Earth-like)");
                }
                const char* pri[] = { "Closest to Me", "Closest to Crosshair" };
                if (ImGui::Combo("Priority", &aimbotTargetPriority, pri, 2)) config::MarkSettingsChanged();
                const char* bones[] = { "Head", "Chest", "Pelvis", "Custom" };
                int boneSel = (aimbotBoneIndex == 9 || aimbotBoneIndex == 68 || aimbotBoneIndex == 110) ? 0
                    : (aimbotBoneIndex == 6 || aimbotBoneIndex == 7 || aimbotBoneIndex == 66 || aimbotBoneIndex == 5) ? 1
                    : (aimbotBoneIndex == 1 || aimbotBoneIndex == 2) ? 2 : 3;
                if (ImGui::Combo("Aim Bone", &boneSel, bones, 4)) {
                    if (boneSel == 0) aimbotBoneIndex = 9;
                    else if (boneSel == 1) aimbotBoneIndex = 6;
                    else if (boneSel == 2) aimbotBoneIndex = 1;
                    config::MarkSettingsChanged();
                }
                if (boneSel == 3) {
                    if (ImGui::InputInt("Bone Index", &aimbotBoneIndex, 1, 10)) config::MarkSettingsChanged();
                }
                ImGui::Spacing();
                ImGui::TextDisabled("Aim Key  ·  Right Mouse Button");
            }
        }
        else if (currentTab == 1) {
            SectionHeader("PLAYER ESP");
            ImGui::Columns(2, "##VisCols", false);
            ImGui::SetColumnWidth(0, 310);
            ToggleRow("Box ESP", &boxEsp);
            if (boxEsp) {
                ToggleRow("Corner Boxes", &espCornerBox);
                if (ImGui::SliderFloat("Box Thickness", &espBoxThickness, 0.5f, 5.0f, "%.1f")) config::MarkSettingsChanged();
            }
            ToggleRow("Health Bar", &healthEsp);
            ToggleRow("Distance Fade", &espDistanceFade);
            ToggleRow("Bone ESP (skeleton)", &boneEsp);
            if (boneEsp) {
                if (ImGui::SliderFloat("Max Bone Distance", &maxBoneDistance, 50.0f, 400.0f, "%.0f m")) config::MarkSettingsChanged();
            }
            ToggleRow("Show Teammates", &showTeammates);
            ToggleRow("Snaplines", &snaplines);
            ToggleRow("Distance Label", &showDistance);
            if (ImGui::SliderFloat("Max Distance", &maxEspDistance, 50.0f, 500.0f, "%.0f m")) config::MarkSettingsChanged();

            ImGui::NextColumn();
            SectionHeader("COLORS");
            if (ImGui::ColorEdit4("Enemy", enemyEspColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) config::MarkSettingsChanged();
            if (ImGui::ColorEdit4("Teammate", teammateEspColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) config::MarkSettingsChanged();
            if (ImGui::ColorEdit4("Bot", botEspColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) config::MarkSettingsChanged();
            ImGui::Columns(1);

            ImGui::Spacing();
            SectionHeader("RADAR");
            ToggleRow("Enable Radar", &radarEnabled);
            if (radarEnabled) {
                ToggleRow("Rotated", &radarRotationMode);
                ToggleRow("Grid", &radarGrid);
                if (ImGui::SliderInt("Size", &radarSize, 50, 800)) config::MarkSettingsChanged();
                if (ImGui::SliderInt("Range", &radarRange, 50, 1000)) config::MarkSettingsChanged();
            }
        }
        else if (currentTab == 2) {
            SectionHeader("VEHICLE ESP");
            ColorToggle("Enemy Vehicles", &enemyVehicleEsp, enemyVehicleEspColor);
            ColorToggle("Team Vehicles", &teamVehicleEsp, teamVehicleEspColor);
            ToggleRow("Players in Vehicles", &vehicleOccupantEsp);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw ESP on seated occupants");
            if (ImGui::SliderFloat("Max Vehicle Distance", &maxVehicleEspDistance, 50.0f, 800.0f, "%.0f m")) config::MarkSettingsChanged();
            if (ImGui::SliderFloat("Vehicle Box Thickness", &vehicleEspThickness, 0.5f, 5.0f, "%.1f")) config::MarkSettingsChanged();
            ImGui::Spacing();
            ImGui::TextDisabled("Team check uses the 3 Wardogs factions.");
            ImGui::TextDisabled("Occupied vehicles show crew count in the label.");
        }
        else if (currentTab == 3) {
            SectionHeader("SYSTEM");
            ToggleRow("VSync", &vsyncEnabled);
            ToggleRow("FPS Counter", &showFpsCounter);
            if (showFpsCounter) {
                if (ImGui::SliderFloat("FPS Text Size", &fpsTextSize, 10.0f, 48.0f, "%.0f")) config::MarkSettingsChanged();
                if (ImGui::ColorEdit4("FPS Color", fpsColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) config::MarkSettingsChanged();
            }
            ImGui::Spacing();
            SectionHeader("APPEARANCE");
            if (ImGui::ColorEdit4("Accent", accentColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) config::MarkSettingsChanged();
            ImGui::Spacing();
            ImGui::TextDisabled("This build is free. Never pay for it.");
            if (ImGui::Selectable("Unknowncheats thread:"))
                ShellExecuteA(NULL, "open", "https://www.unknowncheats.me/forum/other-fps-games/775205-wardogs-external.html", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (currentTab == 4) {
            SectionHeader("CONFIGURATION");
            ImGui::Text("Active  ·  %s", config::currentConfig.c_str());
            ImGui::Spacing();
            static std::vector<std::string> configList;
            static bool configListNeedsRefresh = true;
            static char configPathBuf[512] = {0};
            if (configListNeedsRefresh) {
                configList = config::GetConfigList();
                strncpy_s(configPathBuf, config::CONFIG_FOLDER.c_str(), 511);
                configPathBuf[511] = 0;
                configListNeedsRefresh = false;
            }
            ImGui::TextDisabled("Save path");
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##ConfigPath", configPathBuf, sizeof(configPathBuf))) {
                config::CONFIG_FOLDER = configPathBuf;
                config::MarkSettingsChanged();
            }
            ImGui::PopItemWidth();
            ImGui::Spacing();
            ImGui::BeginChild("##ConfigList", ImVec2(0, 180), true);
            for (size_t i = 0; i < configList.size(); i++) {
                bool isSelected = (configList[i] == config::currentConfig);
                if (ImGui::Selectable(configList[i].c_str(), isSelected)) {
                    config::LoadConfig(configList[i]);
                    configListNeedsRefresh = true;
                }
            }
            ImGui::EndChild();
            ImGui::Spacing();
            static char newConfigName[64] = "config";
            static int configNumber = 2;
            ImGui::PushItemWidth(200);
            ImGui::InputText("##NewConfigName", newConfigName, sizeof(newConfigName));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Create")) {
                std::string name = newConfigName;
                if (name.empty()) name = "config" + std::to_string(configNumber++);
                if (name.find(".json") == std::string::npos) name += ".json";
                config::CreateNewConfig(name);
                configListNeedsRefresh = true;
                strcpy_s(newConfigName, "config");
            }
            ImGui::SameLine();
            if (ImGui::Button("Save")) config::SaveConfig();
            ImGui::SameLine();
            if (ImGui::Button("Reload")) {
                config::LoadConfig(config::currentConfig);
                configListNeedsRefresh = true;
            }
            ImGui::Spacing();
            ImGui::TextDisabled("Auto-saves 2 seconds after changes.");
        }

        ImGui::EndChild();
        ImGui::End();
    }
}

namespace overlay {
    inline WNDCLASSEX wc = {};
    inline int lastWidth = 0;
    inline int lastHeight = 0;

    inline LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    struct FindGameWindowData {
        DWORD pid;
        HWND hwnd;
    };

    inline BOOL CALLBACK EnumGameWindowsProc(HWND hwnd, LPARAM lParam) {
        auto* data = reinterpret_cast<FindGameWindowData*>(lParam);
        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);
        if (windowPid != data->pid || !IsWindowVisible(hwnd))
            return TRUE;

        wchar_t className[64] = {};
        GetClassNameW(hwnd, className, 64);
        if (wcscmp(className, L"UnrealWindow") != 0)
            return TRUE;

        RECT rc = {};
        GetClientRect(hwnd, &rc);
        if (rc.right > 100 && rc.bottom > 100) {
            data->hwnd = hwnd;
            return FALSE;
        }
        return TRUE;
    }

    inline HWND FindGameWindow() {
        FindGameWindowData data = { static_cast<DWORD>(driver.ProcessID), nullptr };
        EnumWindows(EnumGameWindowsProc, reinterpret_cast<LPARAM>(&data));
        return data.hwnd;
    }

    inline void SyncToGameWindow() {
        if (!g_overlayHwnd) return;

        static DWORD lastFind = 0;
        const DWORD now = GetTickCount();
        if (!g_gameHwnd || !IsWindow(g_gameHwnd)) {
            if (now - lastFind > 500) {
                lastFind = now;
                g_gameHwnd = FindGameWindow();
            }
        }
        if (!g_gameHwnd) return;

        RECT client = {};
        GetClientRect(g_gameHwnd, &client);
        POINT tl = { client.left, client.top };
        POINT br = { client.right, client.bottom };
        ClientToScreen(g_gameHwnd, &tl);
        ClientToScreen(g_gameHwnd, &br);

        const int x = tl.x;
        const int y = tl.y;
        const int w = br.x - tl.x;
        const int h = br.y - tl.y;
        if (w <= 0 || h <= 0) return;

        static int lastX = INT_MIN, lastY = INT_MIN;
        if (x != lastX || y != lastY || w != lastWidth || h != lastHeight) {
            lastX = x;
            lastY = y;
            SetWindowPos(g_overlayHwnd, HWND_TOPMOST, x, y, w, h,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
            if (w != lastWidth || h != lastHeight) {
                lastWidth = w;
                lastHeight = h;
                Width = w;
                Height = h;
                d3d::Resize(static_cast<UINT>(w), static_cast<UINT>(h));
            }
        }
    }

    inline bool Create() {
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"OverlayClass";
        RegisterClassEx(&wc);

        g_gameHwnd = FindGameWindow();

        int x = 0, y = 0, w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
        if (g_gameHwnd) {
            RECT client = {};
            GetClientRect(g_gameHwnd, &client);
            POINT tl = { client.left, client.top };
            POINT br = { client.right, client.bottom };
            ClientToScreen(g_gameHwnd, &tl);
            ClientToScreen(g_gameHwnd, &br);
            x = tl.x;
            y = tl.y;
            w = br.x - tl.x;
            h = br.y - tl.y;
        }

        g_overlayHwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            wc.lpszClassName, L"",
            WS_POPUP,
            x, y, w, h,
            nullptr, nullptr, wc.hInstance, nullptr
        );

        if (!g_overlayHwnd) return false;

        SetLayeredWindowAttributes(g_overlayHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

        MARGINS margin = { -1 };
        DwmExtendFrameIntoClientArea(g_overlayHwnd, &margin);

        ShowWindow(g_overlayHwnd, SW_SHOWNOACTIVATE);
        UpdateWindow(g_overlayHwnd);

        lastWidth = w;
        lastHeight = h;
        Width = w;
        Height = h;

        return d3d::Init(g_overlayHwnd);
    }
}

namespace debug_overlay {
    inline void Draw() {}
    inline void PrintConsole() {}
}

namespace fps {
    inline void Draw() {
        if (!showFpsCounter) return;

        auto draw = ImGui::GetForegroundDrawList();
        float fpsVal = ImGui::GetIO().Framerate;

        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", fpsVal);

        ImFont* font = d3d::hudFont ? d3d::hudFont : ImGui::GetFont();
        float size = font->FontSize;
        if (fpsTextSize > 8.f && fpsTextSize < size)
            size = fpsTextSize;

        const float x = 12.0f;
        const float y = 12.0f;
        const ImVec2 pos(floorf(x + 0.5f), floorf(y + 0.5f));

        ImU32 textColor = IM_COL32(
            (int)(fpsColor[0] * 255),
            (int)(fpsColor[1] * 255),
            (int)(fpsColor[2] * 255),
            (int)(fpsColor[3] * 255)
        );

        const ImU32 outline = IM_COL32(0, 0, 0, 230);
        const float o = 1.0f;
        draw->AddText(font, size, ImVec2(pos.x - o, pos.y), outline, fpsText);
        draw->AddText(font, size, ImVec2(pos.x + o, pos.y), outline, fpsText);
        draw->AddText(font, size, ImVec2(pos.x, pos.y - o), outline, fpsText);
        draw->AddText(font, size, ImVec2(pos.x, pos.y + o), outline, fpsText);
        draw->AddText(font, size, pos, textColor, fpsText);
    }
}

inline void RenderLoop() {
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) break;

        overlay::SyncToGameWindow();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ActorLoop();

        radar::DrawBackground();

        debug_overlay::Draw();
        debug_overlay::PrintConsole();

        fps::Draw();

        menu::Render();

        ImGui::Render();
        float clear[4] = { 0, 0, 0, 0 };
        d3d::pContext->OMSetRenderTargets(1, &d3d::pRenderTarget, nullptr);
        d3d::pContext->ClearRenderTargetView(d3d::pRenderTarget, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        d3d::pSwapChain->Present(vsyncEnabled ? 1 : 0, 0);
    }

    d3d::Shutdown();
}
