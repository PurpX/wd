#pragma once
#include "../util/math.h"
#include "offsets.h"
#include "../memory/driver.h"
#include "../util/settings.h"
#include "types.h"
#include <bit>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <chrono>

namespace LocalPtrs {
    inline uintptr_t Player;
    inline uintptr_t PlayerController;
    inline uintptr_t Gworld;
}

inline Camera vCamera{ {}, {}, 90.0f };
inline Camera vCameraLive{ {}, {}, 90.0f };

#pragma pack(push, 1)
struct CameraPovLite {
    sdk::FVector Location{};
    sdk::FRotator Rotation{};
    float FOV = 90.f;
};
#pragma pack(pop)

inline bool CameraFromPov(const CameraPovLite& pov, Camera& out) {
    if (!(pov.FOV > 1.f && pov.FOV < 170.f)) return false;
    if (!std::isfinite(static_cast<double>(pov.Location.X)) || !std::isfinite(static_cast<double>(pov.Rotation.Yaw)))
        return false;
    out.Location = Vector3(static_cast<double>(pov.Location.X), static_cast<double>(pov.Location.Y), static_cast<double>(pov.Location.Z));
    out.Rotation = Vector3(static_cast<double>(pov.Rotation.Pitch), static_cast<double>(pov.Rotation.Yaw), static_cast<double>(pov.Rotation.Roll));
    out.FieldOfView = pov.FOV;
    return true;
}

struct RotationData {
    double A;
    char pad1[24];
    double B;
    char pad2[424];
    double C;
};

inline Vector3 GetBoneWithRotation(DWORD_PTR mesh, int boneId) {
    (void)mesh;
    (void)boneId;
    return Vector3();
}

inline Camera GetViewAngles() {
    return vCamera;
}

inline bool is_dead(uintptr_t pawn) {
    (void)pawn;
    return false;
}

struct W2SCache {
    Vector3 location{};
    Vector3 axisX{}, axisY{}, axisZ{};
    float invFovTan = 0.f;
    float centerX = 0.f;
    float centerY = 0.f;
    bool valid = false;
};
inline W2SCache g_w2s{};

inline void UpdateW2SCache() {
    g_w2s.valid = false;
    if (!std::isfinite(vCamera.Location.x) || !std::isfinite(vCamera.Location.y) || !std::isfinite(vCamera.Location.z))
        return;
    if (!(vCamera.FieldOfView > 1.f && vCamera.FieldOfView < 170.f))
        return;

    auto rotMatrix = Matrix(vCamera.Rotation);
    g_w2s.location = vCamera.Location;
    g_w2s.axisX = Vector3(rotMatrix.m[0][0], rotMatrix.m[0][1], rotMatrix.m[0][2]);
    g_w2s.axisY = Vector3(rotMatrix.m[1][0], rotMatrix.m[1][1], rotMatrix.m[1][2]);
    g_w2s.axisZ = Vector3(rotMatrix.m[2][0], rotMatrix.m[2][1], rotMatrix.m[2][2]);
    float fovRad = static_cast<float>(tan(vCamera.FieldOfView * std::numbers::pi / 360.f));
    if (fovRad < 0.001f) return;
    g_w2s.invFovTan = 1.f / fovRad;
    g_w2s.centerX = Width * 0.5f;
    g_w2s.centerY = Height * 0.5f;
    g_w2s.valid = true;
}

inline double AngleDeltaDeg(double from, double to) {
    double d = to - from;
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

inline Camera BlendCamera(const Camera& a, const Camera& b, float t) {
    if (t <= 0.f) return a;
    if (t >= 1.f) return b;
    const double td = static_cast<double>(t);
    Camera o{};
    o.Location.x = a.Location.x + (b.Location.x - a.Location.x) * td;
    o.Location.y = a.Location.y + (b.Location.y - a.Location.y) * td;
    o.Location.z = a.Location.z + (b.Location.z - a.Location.z) * td;
    o.Rotation.x = a.Rotation.x + AngleDeltaDeg(a.Rotation.x, b.Rotation.x) * td;
    o.Rotation.y = a.Rotation.y + AngleDeltaDeg(a.Rotation.y, b.Rotation.y) * td;
    o.Rotation.z = a.Rotation.z + AngleDeltaDeg(a.Rotation.z, b.Rotation.z) * td;
    o.FieldOfView = a.FieldOfView + (b.FieldOfView - a.FieldOfView) * t;
    return o;
}

struct EspCamSample {
    double t = 0.0;
    Camera cam{};
};
inline EspCamSample g_espCamHist[96]{};
inline int g_espCamHistHead = 0;
inline int g_espCamHistCount = 0;
inline int g_espCamHistKind = -1;

inline double EspNowSeconds() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

inline bool CamSame(const Camera& a, const Camera& b) {
    return a.Rotation.x == b.Rotation.x && a.Rotation.y == b.Rotation.y &&
           a.Rotation.z == b.Rotation.z && a.Location.x == b.Location.x &&
           a.Location.y == b.Location.y && a.Location.z == b.Location.z &&
           a.FieldOfView == b.FieldOfView;
}

inline void EspCamPush(const Camera& cam, double now, int kind) {
    if (kind != g_espCamHistKind) {
        g_espCamHistKind = kind;
        g_espCamHistHead = 0;
        g_espCamHistCount = 0;
    }
    constexpr int N = 96;
    if (g_espCamHistCount > 0) {
        const int last = (g_espCamHistHead + N - 1) % N;
        if (CamSame(g_espCamHist[last].cam, cam)) return;
    }
    g_espCamHist[g_espCamHistHead].t = now;
    g_espCamHist[g_espCamHistHead].cam = cam;
    g_espCamHistHead = (g_espCamHistHead + 1) % N;
    if (g_espCamHistCount < N) ++g_espCamHistCount;
}

inline bool EspCamAt(double target, Camera& out) {
    constexpr int N = 96;
    if (g_espCamHistCount <= 0) return false;
    auto at = [&](int i) -> const EspCamSample& {
        return g_espCamHist[(g_espCamHistHead + N - g_espCamHistCount + i) % N];
    };
    const int n = g_espCamHistCount;
    if (target >= at(n - 1).t) { out = at(n - 1).cam; return true; }
    if (target <= at(0).t) { out = at(0).cam; return true; }
    for (int i = n - 1; i > 0; --i) {
        const EspCamSample& b = at(i);
        const EspCamSample& a = at(i - 1);
        if (target >= a.t) {
            const double span = b.t - a.t;
            const float f = span > 1e-6 ? static_cast<float>((target - a.t) / span) : 1.f;
            out = BlendCamera(a.cam, b.cam, f);
            return true;
        }
    }
    out = at(0).cam;
    return true;
}

inline void RefreshEspCamera(bool inVehicle) {
    if (!LocalPtrs::PlayerController || !memory.BaseAddress) return;
    uint64_t pcm = memory.read<uint64_t>(LocalPtrs::PlayerController + offsets::CameraManager);
    if (!pcm) return;

    CameraPovLite lastPov = memory.read<CameraPovLite>(pcm + offsets::LastFrameCameraCache + offsets::CameraPov);
    CameraPovLite livePov = memory.read<CameraPovLite>(pcm + offsets::CameraCachePrivate + offsets::CameraPov);
    CameraPovLite viewPov = memory.read<CameraPovLite>(pcm + offsets::ViewTarget + offsets::CameraPov);

    Camera lastCam{}, liveCam{}, viewCam{};
    const bool hasLast = CameraFromPov(lastPov, lastCam);
    const bool hasLive = CameraFromPov(livePov, liveCam);
    const bool hasView = CameraFromPov(viewPov, viewCam);

    if (hasLive) vCameraLive = liveCam;
    else if (hasView) vCameraLive = viewCam;
    else if (hasLast) vCameraLive = lastCam;

    Camera src{};
    bool haveSrc = false;
    if (inVehicle) {
        if (hasLive) { src = liveCam; haveSrc = true; }
        else if (hasView) { src = viewCam; haveSrc = true; }
        else if (hasLast) { src = lastCam; haveSrc = true; }
    } else {
        if (hasLast) { src = lastCam; haveSrc = true; }
        else if (hasLive) { src = liveCam; haveSrc = true; }
    }
    if (!haveSrc) return;

    const double now = EspNowSeconds();
    EspCamPush(src, now, inVehicle ? 1 : 0);

    constexpr double kEspCamDelaySec = 0.060;
    Camera espCam = src;
    if (!EspCamAt(now - kEspCamDelaySec, espCam))
        espCam = src;

    vCamera = espCam;
    UpdateW2SCache();
}

inline Vector2 ProjectWorldToScreen(Vector3 worldPos) {
    if (!g_w2s.valid) UpdateW2SCache();
    if (!g_w2s.valid) return Vector2(0, 0);
    if (!std::isfinite(worldPos.x) || !std::isfinite(worldPos.y) || !std::isfinite(worldPos.z))
        return Vector2(0, 0);

    Vector3 delta = worldPos - g_w2s.location;
    Vector3 transformed(delta.Dot(g_w2s.axisY), delta.Dot(g_w2s.axisZ), delta.Dot(g_w2s.axisX));
    if (transformed.z < 1.0) return Vector2(0, 0);

    return Vector2(
        g_w2s.centerX + static_cast<float>(transformed.x) * (g_w2s.centerX * g_w2s.invFovTan) / static_cast<float>(transformed.z),
        g_w2s.centerY - static_cast<float>(transformed.y) * (g_w2s.centerX * g_w2s.invFovTan) / static_cast<float>(transformed.z)
    );
}

inline bool IsInFrontOfCamera(const Vector3& worldPos) {
    if (!g_w2s.valid) UpdateW2SCache();
    if (!g_w2s.valid) return false;
    Vector3 delta = worldPos - g_w2s.location;
    return delta.Dot(g_w2s.axisX) > 0.0;
}

inline double ReadWorldTimeSeconds() {
    static uintptr_t s_world = 0;
    static uint64_t s_off = 0;
    static double s_last = 0.0;

    if (!LocalPtrs::Gworld) return 0.0;
    if (LocalPtrs::Gworld != s_world) {
        s_world = LocalPtrs::Gworld;
        s_off = 0;
        s_last = 0.0;
    }

    auto plausible = [](double t) {
        return std::isfinite(t) && t > 0.5 && t < 1.0e8;
    };

    if (s_off) {
        double t = memory.read<double>(s_world + s_off);
        if (plausible(t)) {
            s_last = t;
            return t;
        }
        s_off = 0;
    }

    {
        double pinned = memory.read<double>(s_world + offsets::World::TimeSeconds);
        if (plausible(pinned)) {
            float dt = memory.read<float>(s_world + offsets::World::DeltaTimeSeconds);
            if ((dt > 0.00005f && dt < 0.25f) || pinned > 1.0) {
                s_off = offsets::World::TimeSeconds;
                s_last = pinned;
                return pinned;
            }
        }
    }

    double serverT = 0.0;
    uint64_t gs = memory.read<uint64_t>(s_world + offsets::World::GameState);
    if (gs)
        serverT = memory.read<double>(gs + offsets::GameStateBase::ReplicatedWorldTimeSecondsDouble);

    auto tryOff = [&](uint64_t off) -> bool {
        double a = memory.read<double>(s_world + off);
        if (!plausible(a)) return false;
        double b = memory.read<double>(s_world + off + 8);
        double c = memory.read<double>(s_world + off + 16);
        if (!plausible(b) || !plausible(c)) return false;
        if (std::fabs(a - b) > 30.0) return false;
        float dt = memory.read<float>(s_world + off + 32 + 4);
        float dtAlt = memory.read<float>(s_world + off + 32);
        bool dtOk = (dt > 0.00005f && dt < 0.25f) || (dtAlt > 0.00005f && dtAlt < 0.25f);
        if (!dtOk && serverT > 1.0 && std::fabs(a - serverT) > 300.0) return false;
        s_off = off;
        s_last = a;
        return true;
    };

    for (uint64_t off : offsets::World::TimeSecondsCandidates) {
        if (tryOff(off)) return s_last;
    }
    for (uint64_t off = offsets::World::TimeSecondsPadBegin;
         off + 40 < offsets::World::TimeSecondsPadEnd; off += 8) {
        if (tryOff(off)) return s_last;
    }

    if (plausible(serverT)) {
        s_last = serverT;
        return serverT;
    }
    return s_last;
}

inline float BestLastRenderTime(uint64_t mesh, double now) {
    const uint64_t offs[] = {
        offsets::MeshLastRenderTime,
        offsets::MeshLastRenderTimeOnScreen,
        offsets::MeshLastSubmitTime,
        0x360,
        0x35C,
    };
    float best = 0.f;
    double bestDelta = 1e9;
    for (uint64_t off : offs) {
        float v = memory.read<float>(mesh + off);
        if (!std::isfinite(v) || v <= 1.f) continue;
        const double d = std::fabs(now - static_cast<double>(v));
        if (d < bestDelta) {
            bestDelta = d;
            best = v;
        }
    }
    return best;
}

inline bool CheckVisibility(uint64_t pawn, double now, double   = 0.0) {
    if (!pawn || now <= 0.0) return false;

    uint64_t mesh = memory.read<uint64_t>(pawn + offsets::MoverMesh);
    if (!mesh)
        mesh = memory.read<uint64_t>(pawn + offsets::CharacterMesh);
    if (!mesh) return false;

    float last = BestLastRenderTime(mesh, now);
    if (!std::isfinite(last) || last <= 0.f) return false;

    return (now - static_cast<double>(last)) <= 0.10;
}

inline bool IsVisible(uintptr_t mesh) {
    if (!mesh) return false;
    double now = ReadWorldTimeSeconds();
    if (now <= 0.0) return false;
    float last = BestLastRenderTime(mesh, now);
    if (!std::isfinite(last) || last <= 0.f) return false;
    return (now - static_cast<double>(last)) <= 0.10;
}
