#pragma once
#include "sdk/sdk.h"
#include "../imgui/imgui.h"
#include "prediction.h"
#include <iostream>
#include <algorithm>
#include <bit>
#include <vector>
#include <cmath>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

struct ActorInfo {
    uintptr_t addr = 0;
    uintptr_t mesh = 0;
    uintptr_t boneArrayOff = 0;
    uintptr_t c2wOff = 0;
    uintptr_t rootC2wOff = 0;
    int headBone = -1;
    int chestBone = -1;
    Vector3 pos{};
    Vector3 head{};
    Vector3 chest{};
    bool isPlayer = false;
    bool isVehicle = false;
    bool inVehicle = false;
    uintptr_t vehicleAddr = 0;
    int occupantCount = 0;
    bool hasHead = false;
    int teamId = -1;
    uint32_t factionId = 0;
    bool isTeammate = false;
    float health = -1.f;
    float maxHealth = -1.f;
    bool isDead = false;
    bool isVisible = true;
    bool visCached = false;
    int visAge = 0;
    float distanceCm = 0.f;
};
inline std::vector<ActorInfo> g_actor_cache;
inline std::mutex g_actor_cache_mutex;
inline std::atomic<bool> g_scanner_running{ false };
inline std::thread g_scanner_thread;
inline int g_localTeamId = -1;
inline uint32_t g_localFactionId = 0;

static void ScanLoop(unsigned int scan_interval_ms);

inline void StartScanner(unsigned int ms = 50) {
    if (g_scanner_running.load()) return;
    g_scanner_running.store(true);
    g_scanner_thread = std::thread(ScanLoop, ms);
}

inline void StopScanner() {
    g_scanner_running.store(false);
    if (g_scanner_thread.joinable()) g_scanner_thread.join();
}

namespace Bones {
    constexpr int Head = 9;
    constexpr int Neck = 7;
    constexpr int SpineUpper = 6;
    constexpr int SpineMid = 4;
    constexpr int SpineLow = 2;
    constexpr int Pelvis = 1;
    constexpr int UpperarmL = 11;
    constexpr int LowerarmL = 12;
    constexpr int HandL = 22;
    constexpr int UpperarmR = 65;
    constexpr int LowerarmR = 66;
    constexpr int HandR = 76;
    constexpr int ThighL = 145;
    constexpr int CalfL = 146;
    constexpr int FootL = 147;
    constexpr int ThighR = 122;
    constexpr int CalfR = 123;
    constexpr int FootR = 124;
}

inline void FillM(FMatrix &M) {
    M.m[0][0] = M.XPlane.x; M.m[0][1] = M.XPlane.y; M.m[0][2] = M.XPlane.z; M.m[0][3] = M.XPlane.W;
    M.m[1][0] = M.YPlane.x; M.m[1][1] = M.YPlane.y; M.m[1][2] = M.YPlane.z; M.m[1][3] = M.YPlane.W;
    M.m[2][0] = M.ZPlane.x; M.m[2][1] = M.ZPlane.y; M.m[2][2] = M.ZPlane.z; M.m[2][3] = M.ZPlane.W;
    M.m[3][0] = M.WPlane.x; M.m[3][1] = M.WPlane.y; M.m[3][2] = M.WPlane.z; M.m[3][3] = M.WPlane.W;
}

inline void MulM(const FMatrix &A, const FMatrix &B, FMatrix &Out) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k) sum += A.m[r][k] * B.m[k][c];
            Out.m[r][c] = sum;
        }
    }
    Out.XPlane.x = Out.m[0][0]; Out.XPlane.y = Out.m[0][1]; Out.XPlane.z = Out.m[0][2]; Out.XPlane.W = Out.m[0][3];
    Out.YPlane.x = Out.m[1][0]; Out.YPlane.y = Out.m[1][1]; Out.YPlane.z = Out.m[1][2]; Out.YPlane.W = Out.m[1][3];
    Out.ZPlane.x = Out.m[2][0]; Out.ZPlane.y = Out.m[2][1]; Out.ZPlane.z = Out.m[2][2]; Out.ZPlane.W = Out.m[2][3];
    Out.WPlane.x = Out.m[3][0]; Out.WPlane.y = Out.m[3][1]; Out.WPlane.z = Out.m[3][2]; Out.WPlane.W = Out.m[3][3];
}

inline uint64_t decrypt_world(uint64_t encoded)
{
    const uint64_t A = 0x6501B96661E130DDULL;
    const uint64_t B = 0x79D95BD19230E74DULL;
    auto modinv64 = [](uint64_t a) -> uint64_t {
        uint64_t x = 1;
        for (int i = 0; i < 6; ++i) x = x * (2 - a * x);
        return x;
    };
    static uint64_t Ainv = modinv64(A);
    return Ainv * (encoded - B);
}

inline FTransform ReadUETransform(uint64_t addr) {
    struct UETransform {
        double qx, qy, qz, qw;
        double tx, ty, tz;
        double pad0;
        double sx, sy, sz;
        double pad1;
    };
    static_assert(sizeof(UETransform) == 0x60, "UETransform must be 0x60");
    UETransform raw = memory.read<UETransform>(addr);
    FTransform out;
    out.Rotation.x = raw.qx; out.Rotation.y = raw.qy; out.Rotation.z = raw.qz; out.Rotation.w = raw.qw;
    out.Translation = Vector3(raw.tx, raw.ty, raw.tz);
    out.Scale3D = Vector3(
        (raw.sx != 0.0) ? raw.sx : 1.0,
        (raw.sy != 0.0) ? raw.sy : 1.0,
        (raw.sz != 0.0) ? raw.sz : 1.0);
    return out;
}

inline Vector3 TransformPoint(const FTransform& t, Vector3 p) {
    p = Vector3(p.x * t.Scale3D.x, p.y * t.Scale3D.y, p.z * t.Scale3D.z);
    const double n = sqrt(t.Rotation.x * t.Rotation.x + t.Rotation.y * t.Rotation.y + t.Rotation.z * t.Rotation.z + t.Rotation.w * t.Rotation.w);
    if (n < 1e-8) {
        return Vector3(p.x + t.Translation.x, p.y + t.Translation.y, p.z + t.Translation.z);
    }
    const double inv = 1.0 / n;
    Vector3 q(t.Rotation.x * inv, t.Rotation.y * inv, t.Rotation.z * inv);
    const double w = t.Rotation.w * inv;
    Vector3 tv(
        (q.y * p.z - q.z * p.y) * 2.0,
        (q.z * p.x - q.x * p.z) * 2.0,
        (q.x * p.y - q.y * p.x) * 2.0);
    return Vector3(
        p.x + tv.x * w + (q.y * tv.z - q.z * tv.y) + t.Translation.x,
        p.y + tv.y * w + (q.z * tv.x - q.x * tv.z) + t.Translation.y,
        p.z + tv.z * w + (q.x * tv.y - q.y * tv.x) + t.Translation.z);
}

inline bool ReadActorWorldPos(uint64_t actor, Vector3& out, uintptr_t* cachedRootC2w = nullptr);

inline bool ReadComponentToWorld(uint64_t mesh, const Vector3& rootPos, uintptr_t& outOff, FTransform& outC2W) {
    constexpr uintptr_t candidates[] = {
        offsets::MeshComponentToWorld,
        offsets::MeshComponentToWorldAlt1,
        offsets::MeshComponentToWorldAlt2
    };
    for (uintptr_t off : candidates) {
        FTransform c2w = ReadUETransform(mesh + off);
        Vector3 t = c2w.Translation;
        if (!std::isfinite(t.x) || !std::isfinite(t.y) || !std::isfinite(t.z)) continue;
        double dx = t.x - rootPos.x, dy = t.y - rootPos.y, dz = t.z - rootPos.z;
        if ((dx * dx + dy * dy + dz * dz) > (500.0 * 500.0)) continue;
        double qn = c2w.Rotation.x * c2w.Rotation.x + c2w.Rotation.y * c2w.Rotation.y + c2w.Rotation.z * c2w.Rotation.z + c2w.Rotation.w * c2w.Rotation.w;
        if (qn < 0.25 || qn > 4.0) continue;
        outOff = off;
        outC2W = c2w;
        return true;
    }
    return false;
}

inline bool TryBoneWorld(uint64_t mesh, int boneIdx, uintptr_t boneOff, const FTransform& c2w, const Vector3& rootPos, Vector3& outWorld) {
    uint64_t data = memory.read<uint64_t>(mesh + boneOff);
    int32_t count = memory.read<int32_t>(mesh + boneOff + 8);
    int32_t maxc = memory.read<int32_t>(mesh + boneOff + 12);
    if (!data || count < 10 || count > 1024 || boneIdx < 0 || boneIdx >= count) return false;
    if (maxc > 0 && maxc < count) return false;

    FTransform bone = ReadUETransform(data + static_cast<uint64_t>(boneIdx) * offsets::FTransformSize);
    Vector3 world = TransformPoint(c2w, bone.Translation);
    if (!std::isfinite(world.x) || !std::isfinite(world.y) || !std::isfinite(world.z)) return false;
    double dx = world.x - rootPos.x, dy = world.y - rootPos.y, dz = world.z - rootPos.z;
    double distSq = dx * dx + dy * dy + dz * dz;
    if (distSq > (400.0 * 400.0) || distSq < 1.0) return false;
    outWorld = world;
    return true;
}

inline bool ResolvePlayerBones(ActorInfo& info) {
    if (!info.mesh) return false;
    Vector3 rootPos{};
    if (!ReadActorWorldPos(info.addr, rootPos, &info.rootC2wOff)) return false;
    info.pos = rootPos;

    FTransform c2w{};
    if (info.c2wOff && info.boneArrayOff && info.headBone >= 0) {
        c2w = ReadUETransform(info.mesh + info.c2wOff);
        Vector3 t = c2w.Translation;
        double dx = t.x - rootPos.x, dy = t.y - rootPos.y, dz = t.z - rootPos.z;
        double qn = c2w.Rotation.x * c2w.Rotation.x + c2w.Rotation.y * c2w.Rotation.y
                  + c2w.Rotation.z * c2w.Rotation.z + c2w.Rotation.w * c2w.Rotation.w;
        if (std::isfinite(t.x) && (dx * dx + dy * dy + dz * dz) <= (500.0 * 500.0) && qn >= 0.25 && qn <= 4.0) {
            Vector3 hw{}, cw{};
            if (TryBoneWorld(info.mesh, info.headBone, info.boneArrayOff, c2w, rootPos, hw)) {
                info.head = hw;
                info.hasHead = true;
                int chestIdx = info.chestBone >= 0 ? info.chestBone : Bones::SpineUpper;
                if (TryBoneWorld(info.mesh, chestIdx, info.boneArrayOff, c2w, rootPos, cw)
                    && cw.z <= hw.z + 5.0) {
                    info.chest = cw;
                    info.chestBone = chestIdx;
                } else {
                    info.chest = Vector3(hw.x, hw.y, hw.z - 35.0);
                }
                return true;
            }
        }
    }

    uintptr_t c2wOff = info.c2wOff;
    if (!c2wOff || !ReadComponentToWorld(info.mesh, rootPos, c2wOff, c2w)) {
        if (!ReadComponentToWorld(info.mesh, rootPos, c2wOff, c2w)) {
            info.head = Vector3(rootPos.x, rootPos.y, rootPos.z + 75.0);
            info.chest = Vector3(rootPos.x, rootPos.y, rootPos.z + 25.0);
            info.hasHead = false;
            return false;
        }
    }
    info.c2wOff = c2wOff;

    constexpr uintptr_t boneArrayOffsets[] = { 0xA18, offsets::BoneArrayCache, offsets::BoneArray };
    const int headCandidates[] = { info.headBone >= 0 ? info.headBone : Bones::Head, Bones::Head, Bones::Neck, 9, 7 };
    const int chestCandidates[] = { info.chestBone >= 0 ? info.chestBone : Bones::SpineUpper, Bones::SpineUpper, Bones::SpineMid, 6, 4 };

    uintptr_t arraysToTry[4];
    int nArr = 0;
    if (info.boneArrayOff) arraysToTry[nArr++] = info.boneArrayOff;
    for (uintptr_t o : boneArrayOffsets) {
        bool dup = false;
        for (int i = 0; i < nArr; ++i) if (arraysToTry[i] == o) { dup = true; break; }
        if (!dup) arraysToTry[nArr++] = o;
    }

    info.hasHead = false;
    for (int ai = 0; ai < nArr && !info.hasHead; ++ai) {
        for (int bi : headCandidates) {
            Vector3 hw{};
            if (!TryBoneWorld(info.mesh, bi, arraysToTry[ai], c2w, rootPos, hw)) continue;
            if (hw.z < rootPos.z - 20.0 || hw.z > rootPos.z + 200.0) continue;
            info.head = hw;
            info.headBone = bi;
            info.boneArrayOff = arraysToTry[ai];
            info.hasHead = true;
            break;
        }
    }

    bool hasChest = false;
    if (info.hasHead) {
        for (int ai = 0; ai < nArr && !hasChest; ++ai) {
            for (int bi : chestCandidates) {
                Vector3 cw{};
                if (!TryBoneWorld(info.mesh, bi, arraysToTry[ai], c2w, rootPos, cw)) continue;
                if (cw.z < rootPos.z - 40.0 || cw.z > info.head.z) continue;
                info.chest = cw;
                info.chestBone = bi;
                hasChest = true;
                break;
            }
        }
    }

    if (!info.hasHead) {
        info.head = Vector3(rootPos.x, rootPos.y, rootPos.z + 75.0);
        info.chest = Vector3(rootPos.x, rootPos.y, rootPos.z + 25.0);
    } else if (!hasChest) {
        info.chest = Vector3(info.head.x, info.head.y, info.head.z - 35.0);
    }
    return info.hasHead;
}

inline void ResolveGWorldAndLocal() {
    if (!memory.BaseAddress) return;

    uint64_t encrypted = memory.read<uint64_t>(memory.BaseAddress + offsets::UWorld);
    if (encrypted) {
        uint64_t cand = decrypt_world(encrypted);
        uintptr_t base = memory.BaseAddress;
        constexpr uintptr_t kMaxRange = 0x20000000;
        auto in_module = [&](uint64_t p) { return p >= base && p < base + kMaxRange; };
        auto plausible = [&](uint64_t p) -> bool {
            if (p < 0x10000) return false;
            uint64_t t1 = memory.read<uint64_t>(p);
            uint64_t t2 = memory.read<uint64_t>(p + offsets::UObject::Class);
            return (t1 && in_module(t1)) || (t2 && in_module(t2));
        };
        uint64_t candidates[] = { cand, encrypted, std::rotr(cand, 32), cand ^ base };
        for (uint64_t c : candidates) {
            if (c && plausible(c)) {
                LocalPtrs::Gworld = static_cast<uintptr_t>(c);
                break;
            }
        }
    }

    if (!LocalPtrs::Gworld) return;
    uint64_t gameInstance = memory.read<uint64_t>(LocalPtrs::Gworld + offsets::World::OwningGameInstance);
    if (!gameInstance) return;
    auto localPlayers = memory.read< tarray<uint64_t> >(gameInstance + offsets::GameInstance::LocalPlayers);
    if (!localPlayers.data || localPlayers.count <= 0) return;
    uint64_t lp = memory.read<uint64_t>(reinterpret_cast<uint64_t>(localPlayers.data));
    if (!lp) return;
    uint64_t pc = memory.read<uint64_t>(lp + offsets::LocalPlayer::PlayerController);
    if (!pc) return;
    LocalPtrs::PlayerController = static_cast<uintptr_t>(pc);
    uint64_t pawn = memory.read<uint64_t>(pc + offsets::ControllerPawnCurrent);
    if (!pawn) pawn = memory.read<uint64_t>(pc + offsets::ControllerPawn);
    if (pawn) LocalPtrs::Player = static_cast<uintptr_t>(pawn);
}

inline uint64_t ObjectAtIndex(int index) {
    if (index < 0 || !memory.BaseAddress) return 0;
    uint64_t chunks = memory.read<uint64_t>(memory.BaseAddress + offsets::GObjects);
    if (!chunks) return 0;
    const uint64_t perChunk = offsets::UObjectArray::ObjectsPerChunk;
    uint64_t chunk = memory.read<uint64_t>(chunks + static_cast<uint64_t>(index / perChunk) * 8) & ~0xFULL;
    if (!chunk) return 0;
    uint64_t item = chunk + static_cast<uint64_t>(index % perChunk) * offsets::UObjectArray::ItemStride;
    uint64_t obj = memory.read<uint64_t>(item + offsets::UObjectArray::ItemObjectPtr);
    if (!obj) obj = memory.read<uint64_t>(item + offsets::UObjectArray::ItemObjectAlt);
    return obj;
}

inline bool IsAClass(uint64_t object, uint64_t targetClass) {
    if (!object || !targetClass) return false;
    uint64_t type = memory.read<uint64_t>(object + offsets::UObject::Class);
    for (int depth = 0; type && depth < 32; ++depth) {
        if (type == targetClass) return true;
        type = memory.read<uint64_t>(type + offsets::UStruct::Super);
    }
    return false;
}

inline bool ReadActorWorldPos(uint64_t actor, Vector3& out, uintptr_t* cachedRootC2w) {
    uint64_t root = memory.read<uint64_t>(actor + offsets::RootComponent);
    if (!root) return false;

    auto tryOff = [&](uint64_t off) -> bool {
        FTransform c2w = ReadUETransform(root + off);
        Vector3 t = c2w.Translation;
        if (!std::isfinite(t.x) || !std::isfinite(t.y) || !std::isfinite(t.z)) return false;
        if (t.x == 0.0 && t.y == 0.0 && t.z == 0.0) return false;
        double qn = c2w.Rotation.x * c2w.Rotation.x + c2w.Rotation.y * c2w.Rotation.y
                  + c2w.Rotation.z * c2w.Rotation.z + c2w.Rotation.w * c2w.Rotation.w;
        if (qn < 0.25 || qn > 4.0) return false;
        out = t;
        if (cachedRootC2w) *cachedRootC2w = static_cast<uintptr_t>(off);
        return true;
    };

    if (cachedRootC2w && *cachedRootC2w) {
        if (tryOff(*cachedRootC2w)) return true;
        *cachedRootC2w = 0;
    }

    const uint64_t c2wOffs[] = {
        offsets::MeshComponentToWorld,
        offsets::MeshComponentToWorldAlt1,
        offsets::MeshComponentToWorldAlt2
    };
    for (uint64_t off : c2wOffs) {
        if (tryOff(off)) return true;
    }

    sdk::FVector loc = memory.read<sdk::FVector>(root + offsets::RelativeLocation);
    out = Vector3(static_cast<double>(loc.X), static_cast<double>(loc.Y), static_cast<double>(loc.Z));
    if (!std::isfinite(out.x) || !std::isfinite(out.y) || !std::isfinite(out.z)) return false;
    if (out.x == 0.0 && out.y == 0.0 && out.z == 0.0) return false;
    return true;
}

inline bool ReadActorVelocity(uint64_t actor, Vector3& outVel) {
    if (!actor) return false;
    uint64_t root = memory.read<uint64_t>(actor + offsets::RootComponent);
    if (!root) return false;
    sdk::FVector v = memory.read<sdk::FVector>(root + offsets::ComponentVelocity);
    if (!std::isfinite(static_cast<double>(v.X)) || !std::isfinite(static_cast<double>(v.Y))) return false;
    outVel = Vector3(static_cast<double>(v.X), static_cast<double>(v.Y), static_cast<double>(v.Z));
    double sp = outVel.x * outVel.x + outVel.y * outVel.y + outVel.z * outVel.z;
    return sp > 1.0;
}

inline void ReadFactionFromComponent(uint64_t comp, int& teamId, uint32_t& factionId) {
    teamId = -1;
    factionId = 0;
    if (!comp) return;
    factionId = memory.read<uint32_t>(comp + offsets::FactionComponentTag);
    if (!factionId) factionId = memory.read<uint32_t>(comp + offsets::FactionComponentTagAlt);
    uint64_t factionObj = memory.read<uint64_t>(comp + offsets::FactionComponentFaction);
    uint64_t data = factionObj ? memory.read<uint64_t>(factionObj + offsets::FactionData) : 0;
    if (data) {
        uint8_t tid = memory.read<uint8_t>(data + offsets::GenericTeamId);
        if (tid <= 32) teamId = static_cast<int>(tid);
        if (!factionId) factionId = memory.read<uint32_t>(data + offsets::FactionTag);
    }
}

inline void ReadFactionFromPlayerState(uint64_t playerState, int& teamId, uint32_t& factionId) {
    teamId = -1;
    factionId = 0;
    if (!playerState) return;
    ReadFactionFromComponent(memory.read<uint64_t>(playerState + offsets::PlayerStateFactionComponent), teamId, factionId);
}

inline bool IsSameTeam(int teamA, uint32_t factionA, int teamB, uint32_t factionB) {
    if (teamA >= 0 && teamB >= 0 && teamA == teamB) return true;
    if (factionA != 0 && factionB != 0 && factionA == factionB) return true;
    return false;
}

inline void UpdateLocalTeam() {
    g_localTeamId = -1;
    g_localFactionId = 0;
    if (!LocalPtrs::Player) return;
    uint64_t state = memory.read<uint64_t>(LocalPtrs::Player + offsets::PawnPlayerState);
    if (!state && LocalPtrs::PlayerController)
        state = memory.read<uint64_t>(LocalPtrs::PlayerController + 0x2C0);
    ReadFactionFromPlayerState(state, g_localTeamId, g_localFactionId);
}

inline uint64_t GetVehicleFromOperator(uint64_t operatorComp) {
    if (!operatorComp) return 0;
    uint64_t seat = memory.read<uint64_t>(operatorComp + offsets::OperatorSeatInfo);
    if (!seat) seat = memory.read<uint64_t>(operatorComp + offsets::OperatorSeatInfoAlt);
    if (!seat) return 0;
    return memory.read<uint64_t>(seat + offsets::UObject::Outer);
}

inline int CountVehicleOccupants(uint64_t vehicle, uint64_t* outPawns, int maxOut) {
    int found = 0;
    if (!vehicle) return 0;
    uint64_t seat = memory.read<uint64_t>(vehicle + offsets::VehicleSeatComponent);
    if (!seat) return 0;
    const uint64_t arrays[] = { offsets::SeatOperatorsA, offsets::SeatOperatorsB };
    for (uint64_t arrOff : arrays) {
        auto ops = memory.read< tarray<uint64_t> >(seat + arrOff);
        if (!ops.data || ops.count <= 0 || ops.count > 16) continue;
        for (int i = 0; i < ops.count; ++i) {
            uint64_t op = memory.read<uint64_t>(reinterpret_cast<uint64_t>(ops.data) + static_cast<uint64_t>(i) * sizeof(uint64_t));
            if (!op) continue;
            uint64_t pawn = memory.read<uint64_t>(op + offsets::UObject::Outer);
            if (!pawn || pawn == LocalPtrs::Player) continue;
            if (outPawns && found < maxOut) {
                bool dup = false;
                for (int j = 0; j < found; ++j) if (outPawns[j] == pawn) { dup = true; break; }
                if (!dup) outPawns[found++] = pawn;
            } else if (!outPawns) {
                ++found;
            }
        }
    }
    return found;
}

inline void ReadFactionFromVehicle(uint64_t vehicle, int& teamId, uint32_t& factionId) {
    teamId = -1;
    factionId = 0;
    if (!vehicle) return;
    const uint64_t factionOffs[] = {
        offsets::VehicleFactionWheeled,
        offsets::VehicleFactionTracked,
        offsets::VehicleFactionAirplane,
        offsets::VehicleFactionRotary
    };
    for (uint64_t off : factionOffs) {
        uint64_t comp = memory.read<uint64_t>(vehicle + off);
        if (!comp) continue;
        ReadFactionFromComponent(comp, teamId, factionId);
        if (teamId >= 0 || factionId) return;
    }
    uint64_t ps = memory.read<uint64_t>(vehicle + offsets::VehiclePlayerState);
    if (ps) {
        ReadFactionFromPlayerState(ps, teamId, factionId);
        if (teamId >= 0 || factionId) return;
    }
    uint64_t occupants[8]{};
    int n = CountVehicleOccupants(vehicle, occupants, 8);
    for (int i = 0; i < n; ++i) {
        uint64_t state = memory.read<uint64_t>(occupants[i] + offsets::PawnPlayerState);
        ReadFactionFromPlayerState(state, teamId, factionId);
        if (teamId >= 0 || factionId) return;
    }
}

inline bool VehicleAlreadyCached(const std::vector<ActorInfo>& cache, uintptr_t addr) {
    for (const auto& a : cache) {
        if (a.isVehicle && a.addr == addr) return true;
    }
    return false;
}

inline void TryAddVehicle(std::vector<ActorInfo>& cache, uint64_t vehicle, const Vector3& localPos, bool haveLocal) {
    if (!vehicle || vehicle == LocalPtrs::Player) return;
    if (VehicleAlreadyCached(cache, static_cast<uintptr_t>(vehicle))) return;
    if (cache.size() > 200) return;

    Vector3 worldPos{};
    if (!ReadActorWorldPos(vehicle, worldPos)) return;
    if (haveLocal) {
        double dx = worldPos.x - localPos.x, dy = worldPos.y - localPos.y, dz = worldPos.z - localPos.z;
        double distM = sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
        if (distM > maxVehicleEspDistance + 50.0) return;
    }

    ActorInfo info{};
    info.addr = static_cast<uintptr_t>(vehicle);
    info.mesh = static_cast<uintptr_t>(memory.read<uint64_t>(vehicle + offsets::VehicleMesh));
    info.pos = worldPos;
    info.isVehicle = true;
    info.occupantCount = CountVehicleOccupants(vehicle, nullptr, 0);
    ReadFactionFromVehicle(vehicle, info.teamId, info.factionId);
    info.isTeammate = IsSameTeam(info.teamId, info.factionId, g_localTeamId, g_localFactionId);
    cache.push_back(std::move(info));
}

inline void ScanLevelForVehicles(std::vector<ActorInfo>& cache, uint64_t level, uint64_t vehicleClass, const Vector3& localPos, bool haveLocal) {
    if (!level || !vehicleClass) return;
    auto actors = memory.read< tarray<uint64_t> >(level + offsets::Level::Actors);
    if (!actors.data || actors.count <= 0 || actors.count > 30000) {
        uint64_t container = memory.read<uint64_t>(level + offsets::LevelActorsContainer);
        if (!container) return;
        actors = memory.read< tarray<uint64_t> >(container + offsets::LevelActorsArray);
        if (!actors.data || actors.count <= 0 || actors.count > 30000) return;
    }

    int vehicleHits = 0;
    const int maxHits = 64;
    for (int i = 0; i < actors.count && vehicleHits < maxHits; ++i) {
        uint64_t actor = memory.read<uint64_t>(reinterpret_cast<uint64_t>(actors.data) + static_cast<uint64_t>(i) * sizeof(uint64_t));
        if (!actor) continue;
        if (!IsAClass(actor, vehicleClass)) continue;
        size_t before = cache.size();
        TryAddVehicle(cache, actor, localPos, haveLocal);
        if (cache.size() > before) ++vehicleHits;
    }
}

static void ScanLoop(unsigned int scan_interval_ms) {
    while (g_scanner_running.load()) {
        ResolveGWorldAndLocal();
        UpdateLocalTeam();

        std::vector<ActorInfo> new_cache;
        if (!memory.BaseAddress || !LocalPtrs::Gworld) {
            std::this_thread::sleep_for(std::chrono::milliseconds(scan_interval_ms));
            continue;
        }

        uint64_t gameState = memory.read<uint64_t>(LocalPtrs::Gworld + offsets::World::GameState);
        if (!gameState) {
            std::this_thread::sleep_for(std::chrono::milliseconds(scan_interval_ms));
            continue;
        }

        Vector3 localPos{};
        bool haveLocal = LocalPtrs::Player && ReadActorWorldPos(LocalPtrs::Player, localPos);

        auto playerStates = memory.read< tarray<uint64_t> >(gameState + offsets::GameStateBase::PlayerArray);
        const uint64_t playerClass = ObjectAtIndex(static_cast<int>(offsets::PlayerBaseClassIndex));
        const uint64_t vehicleClass = ObjectAtIndex(static_cast<int>(offsets::VehicleBaseClassIndex));

        if (playerStates.data && playerStates.count > 0 && playerStates.count <= 256) {
            for (int i = 0; i < playerStates.count; ++i) {
                uint64_t state = memory.read<uint64_t>(reinterpret_cast<uint64_t>(playerStates.data) + static_cast<uint64_t>(i) * sizeof(uint64_t));
                if (!state) continue;

                uint64_t pawn = memory.read<uint64_t>(state + offsets::PlayerStatePawn);
                if (!pawn || pawn == LocalPtrs::Player) continue;

                uint64_t mesh = memory.read<uint64_t>(pawn + offsets::MoverMesh);
                if (!mesh) mesh = memory.read<uint64_t>(pawn + offsets::CharacterMesh);
                if (!mesh) continue;
                (void)playerClass;

                Vector3 worldPos{};
                if (!ReadActorWorldPos(pawn, worldPos)) continue;

                ActorInfo info;
                info.addr = static_cast<uintptr_t>(pawn);
                info.mesh = static_cast<uintptr_t>(mesh);
                info.pos = worldPos;
                info.isPlayer = true;
                info.headBone = Bones::Head;
                info.chestBone = Bones::SpineUpper;
                ReadFactionFromPlayerState(state, info.teamId, info.factionId);
                info.isTeammate = IsSameTeam(info.teamId, info.factionId, g_localTeamId, g_localFactionId);
                if (haveLocal) {
                    double dx = worldPos.x - localPos.x;
                    double dy = worldPos.y - localPos.y;
                    double dz = worldPos.z - localPos.z;
                    info.distanceCm = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
                }

                uint64_t op = memory.read<uint64_t>(pawn + offsets::PawnVehicleOperator);
                uint64_t vehicle = GetVehicleFromOperator(op);
                info.inVehicle = (vehicle != 0);
                info.vehicleAddr = static_cast<uintptr_t>(vehicle);
                if (vehicle) TryAddVehicle(new_cache, vehicle, localPos, haveLocal);

                new_cache.push_back(std::move(info));
            }
        }

        std::vector<ActorInfo> prevVehicles;
        {
            std::lock_guard<std::mutex> lk(g_actor_cache_mutex);
            for (const auto& a : g_actor_cache) {
                if (a.isVehicle) prevVehicles.push_back(a);
            }
        }

        static int vehicleScanTick = 0;
        const bool doLevelScan = vehicleClass && (enemyVehicleEsp || teamVehicleEsp) && ((vehicleScanTick++ & 1) == 0);
        if (doLevelScan) {
            uint64_t persistent = memory.read<uint64_t>(LocalPtrs::Gworld + offsets::World::PersistentLevel);
            ScanLevelForVehicles(new_cache, persistent, vehicleClass, localPos, haveLocal);

            auto levels = memory.read< tarray<uint64_t> >(LocalPtrs::Gworld + offsets::World::Levels);
            if (levels.data && levels.count > 0 && levels.count <= 64) {
                for (int li = 0; li < levels.count; ++li) {
                    uint64_t level = memory.read<uint64_t>(reinterpret_cast<uint64_t>(levels.data) + static_cast<uint64_t>(li) * sizeof(uint64_t));
                    if (!level || level == persistent) continue;
                    ScanLevelForVehicles(new_cache, level, vehicleClass, localPos, haveLocal);
                }
            }
        } else if (enemyVehicleEsp || teamVehicleEsp) {
            for (auto& v : prevVehicles) {
                if (VehicleAlreadyCached(new_cache, v.addr)) continue;
                Vector3 wp{};
                if (ReadActorWorldPos(v.addr, wp, &v.rootC2wOff))
                    v.pos = wp;
                if (v.pos.x == 0.0 && v.pos.y == 0.0 && v.pos.z == 0.0) continue;
                new_cache.push_back(std::move(v));
            }
        }

        {
            std::lock_guard<std::mutex> lk(g_actor_cache_mutex);
            g_actor_cache.swap(new_cache);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(scan_interval_ms));
    }
}

inline bool IsBot(uintptr_t playerState) {
    (void)playerState;
    return false;
}

inline void VisTick(std::vector<ActorInfo>& slots, double seconds) {
    const bool haveTime = (seconds > 0.0);
    const bool doCheck = aimbotVisibleOnly || visCheckEsp || drawHeadPrediction || showAimLine;
    for (auto& slot : slots) {
        if (!slot.isPlayer) continue;
        if (!doCheck || !haveTime) {
            slot.visCached = false;
            slot.isVisible = true;
            slot.visAge = 0;
            continue;
        }

        if (slot.visCached && slot.visAge > 0) {
            slot.visAge--;
            continue;
        }

        if (slot.mesh || slot.addr) {
            slot.isVisible = CheckVisibility(slot.addr, seconds, static_cast<double>(slot.distanceCm));
            slot.visCached = true;
            slot.visAge = (slot.distanceCm < 8000.f) ? 3 : 10;
        }
    }
}

inline std::string GetPlatformRaw(uintptr_t playerState) {
    (void)playerState;
    return "";
}

inline std::string GetPlatformName(const std::string& platform) {
    if (platform.find("WIN") != std::string::npos) return "Windows";
    if (platform.find("XBL") != std::string::npos || platform.find("XSX") != std::string::npos || platform.find("XB1") != std::string::npos) return "Xbox";
    if (platform.find("PSN") != std::string::npos || platform.find("PS5") != std::string::npos || platform.find("PS4") != std::string::npos) return "PlayStation";
    if (platform.find("SWT") != std::string::npos) return "Nintendo Switch";
    if (platform.find("AND") != std::string::npos) return "Android";
    if (platform.find("IOS") != std::string::npos) return "iOS";
    return platform.empty() ? "" : platform;
}

inline ImU32 GetPlatformColor(const std::string& platform) {
    if (platform.find("WIN") != std::string::npos) return IM_COL32(125, 165, 170, 255);
    if (platform.find("XBL") != std::string::npos || platform.find("XSX") != std::string::npos || platform.find("XB1") != std::string::npos) return IM_COL32(138, 154, 91, 255);
    if (platform.find("PSN") != std::string::npos || platform.find("PS5") != std::string::npos || platform.find("PS4") != std::string::npos) return IM_COL32(100, 120, 220, 255);
    if (platform.find("SWT") != std::string::npos) return IM_COL32(230, 50, 50, 255);
    if (platform.find("AND") != std::string::npos) return IM_COL32(115, 135, 83, 255);
    if (platform.find("IOS") != std::string::npos) return IM_COL32(144, 154, 251, 255);
    return IM_COL32(180, 180, 180, 255);
}

inline void DrawTextOutlined(const char* text, float x, float y, ImU32 color, float fontSize = 0.f) {
    auto draw = ImGui::GetBackgroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float size = (fontSize > 1.f) ? fontSize : font->FontSize;
    ImU32 shadow = IM_COL32(0, 0, 0, 200);
    draw->AddText(font, size, ImVec2(x + 1.f, y + 1.f), shadow, text);
    draw->AddText(font, size, ImVec2(x, y), color, text);
}

inline int ColorA(ImU32 c) { return (c >> IM_COL32_A_SHIFT) & 0xFF; }
inline ImU32 WithAlpha(ImU32 c, float a01) {
    int a = (int)((std::max)(0.f, (std::min)(1.f, a01)) * 255.f);
    return (c & ~IM_COL32_A_MASK) | ((a & 0xFF) << IM_COL32_A_SHIFT);
}

inline float EspAlphaForDistance(float distM, float maxDist) {
    if (!espDistanceFade) return 1.f;
    if (maxDist < 1.f) maxDist = 1.f;
    float t = distM / maxDist;
    if (t < 0.55f) return 1.f;
    float fade = 1.f - (t - 0.55f) / 0.45f;
    if (fade < 0.18f) fade = 0.18f;
    return fade;
}

inline void DrawCornerBox(float x, float y, float w, float h, ImU32 color, float thickness) {
    auto draw = ImGui::GetBackgroundDrawList();
    float len = (std::min)(w, h) * 0.28f;
    if (len < 6.f) len = 6.f;
    if (len > 22.f) len = 22.f;
    ImU32 outline = WithAlpha(IM_COL32(0, 0, 0, 220), ColorA(color) / 255.f);

    auto corner = [&](float x0, float y0, float dx, float dy) {
        draw->AddLine(ImVec2(x0, y0), ImVec2(x0 + dx, y0), outline, thickness + 1.4f);
        draw->AddLine(ImVec2(x0, y0), ImVec2(x0, y0 + dy), outline, thickness + 1.4f);
        draw->AddLine(ImVec2(x0, y0), ImVec2(x0 + dx, y0), color, thickness);
        draw->AddLine(ImVec2(x0, y0), ImVec2(x0, y0 + dy), color, thickness);
    };
    corner(x, y, len, len);
    corner(x + w, y, -len, len);
    corner(x, y + h, len, -len);
    corner(x + w, y + h, -len, -len);
}

inline void DrawBox(float x, float y, float w, float h, ImU32 color) {
    auto draw = ImGui::GetBackgroundDrawList();
    if (espCornerBox) {
        DrawCornerBox(x, y, w, h, color, espBoxThickness);
    } else {
        draw->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), WithAlpha(IM_COL32(0, 0, 0, 255), ColorA(color) / 255.f), 2.0f, 0, espBoxThickness + 1.0f);
        draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 2.0f, 0, espBoxThickness);
    }
}

inline void DrawHealthBar(float boxX, float boxY, float boxH, float health, float maxHealth, float alpha) {
    if (!healthEsp || maxHealth <= 1.f || health < 0.f) return;
    float pct = health / maxHealth;
    if (pct > 1.f) pct = 1.f;
    if (pct < 0.f) pct = 0.f;

    const float barW = 3.5f;
    const float gap = 4.0f;
    const float x0 = boxX - gap - barW;
    auto draw = ImGui::GetBackgroundDrawList();
    draw->AddRectFilled(ImVec2(x0 - 1.f, boxY - 1.f), ImVec2(x0 + barW + 1.f, boxY + boxH + 1.f), WithAlpha(IM_COL32(0, 0, 0, 200), alpha), 1.5f);
    draw->AddRectFilled(ImVec2(x0, boxY), ImVec2(x0 + barW, boxY + boxH), WithAlpha(IM_COL32(20, 20, 24, 220), alpha), 1.0f);

    float filled = boxH * pct;
    int r, g, b;
    if (pct > 0.5f) {
        float t = (pct - 0.5f) * 2.f;
        r = (int)(255 * (1.f - t) + 60 * t);
        g = (int)(210 * t + 200 * (1.f - t));
        b = 70;
    } else {
        float t = pct * 2.f;
        r = 255;
        g = (int)(200 * t);
        b = 55;
    }
    draw->AddRectFilled(
        ImVec2(x0, boxY + boxH - filled),
        ImVec2(x0 + barW, boxY + boxH),
        WithAlpha(IM_COL32(r, g, b, 255), alpha), 1.0f);
}

inline void DrawSnaplineTo(float x, float y, ImU32 color) {
    if (!snaplines) return;
    auto draw = ImGui::GetBackgroundDrawList();
    draw->AddLine(ImVec2(Width * 0.5f, (float)Height), ImVec2(x, y), color, 1.0f);
}

inline void ReadPawnHealth(ActorInfo& info) {
    info.health = -1.f;
    info.maxHealth = -1.f;
    info.isDead = false;
    if (!info.addr) return;
    uint64_t vit = memory.read<uint64_t>(info.addr + offsets::PawnVitalityComponent);
    if (!vit) return;
    float hp = memory.read<float>(vit + offsets::VitalityHealth);
    float mx = memory.read<float>(vit + offsets::VitalityMaxHealth);
    if (!std::isfinite(hp) || !std::isfinite(mx)) return;
    if (mx < 1.f || mx > 5000.f) return;
    if (hp < -50.f || hp > mx * 1.5f) return;
    info.health = hp;
    info.maxHealth = mx;
    info.isDead = (hp <= 0.5f);
}

inline float ReadLocalWeaponBulletSpeedCm() {
    const float fallbackCm = aimbotBulletSpeed * 100.f;
    if (!useWeaponBulletSpeed || !LocalPtrs::Player) return fallbackCm;

    uint64_t beh = memory.read<uint64_t>(LocalPtrs::Player + offsets::PawnWeaponBehavior);
    if (!beh) return fallbackCm;
    uint64_t statsData = memory.read<uint64_t>(beh + offsets::WeaponBehaviorStatsData);
    if (!statsData) return fallbackCm;

    auto trySpeed = [&](uint64_t off) -> float {
        float s = memory.read<float>(statsData + offsets::WeaponStatsStruct + off);
        if (!std::isfinite(s)) return 0.f;
        if (s >= 2000.f && s <= 200000.f) return s;
        if (s >= 50.f && s <= 2000.f) return s * 100.f;
        return 0.f;
    };

    float speed = trySpeed(offsets::WeaponStatsTypicalSpeedAlt);
    if (speed <= 0.f) speed = trySpeed(offsets::WeaponStatsTypicalSpeed);
    if (speed <= 0.f) return fallbackCm;
    return speed;
}

inline float ReadLocalWeaponBulletSpeedMs() {
    return ReadLocalWeaponBulletSpeedCm() * 0.01f;
}

inline void DrawFovCircle() {
    if (!drawFovCircle) return;
    auto draw = ImGui::GetBackgroundDrawList();
    float centerX = Width / 2.0f;
    float centerY = Height / 2.0f;
    ImU32 fovColor = IM_COL32(
        (int)(fovCircleColor[0] * 255), (int)(fovCircleColor[1] * 255),
        (int)(fovCircleColor[2] * 255), (int)(fovCircleColor[3] * 255));
    draw->AddCircle(ImVec2(centerX, centerY), aimbotFov, fovColor, 64, 1.5f);
}

inline void DrawCrosshair() {
    if (!showCrosshair) return;
    auto draw = ImGui::GetBackgroundDrawList();
    const float cx = Width * 0.5f;
    const float cy = Height * 0.5f;
    const float sz = crosshairSize;
    const float gap = crosshairGap;
    const float th = crosshairThickness;
    ImU32 col = IM_COL32(
        (int)(crosshairColor[0] * 255), (int)(crosshairColor[1] * 255),
        (int)(crosshairColor[2] * 255), (int)(crosshairColor[3] * 255));
    ImU32 outline = IM_COL32(0, 0, 0, (int)(crosshairColor[3] * 255));
    draw->AddLine(ImVec2(cx - sz - gap, cy), ImVec2(cx - gap, cy), outline, th + 1.5f);
    draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + sz + gap, cy), outline, th + 1.5f);
    draw->AddLine(ImVec2(cx, cy - sz - gap), ImVec2(cx, cy - gap), outline, th + 1.5f);
    draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + sz + gap), outline, th + 1.5f);
    draw->AddLine(ImVec2(cx - sz - gap, cy), ImVec2(cx - gap, cy), col, th);
    draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + sz + gap, cy), col, th);
    draw->AddLine(ImVec2(cx, cy - sz - gap), ImVec2(cx, cy - gap), col, th);
    draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + sz + gap), col, th);
    draw->AddCircleFilled(ImVec2(cx, cy), 1.5f, col, 8);
}

inline void DrawSkelSegment(const Vector3& a, const Vector3& b, ImU32 color, float thickness = 1.6f) {
    Vector2 sa = ProjectWorldToScreen(a);
    Vector2 sb = ProjectWorldToScreen(b);
    if ((sa.x == 0.f && sa.y == 0.f) || (sb.x == 0.f && sb.y == 0.f)) return;
    auto draw = ImGui::GetBackgroundDrawList();
    draw->AddLine(ImVec2(sa.x, sa.y), ImVec2(sb.x, sb.y), IM_COL32(0, 0, 0, 180), thickness + 1.4f);
    draw->AddLine(ImVec2(sa.x, sa.y), ImVec2(sb.x, sb.y), color, thickness);
}

inline bool GetSkeletonBoneWorldCached(uint64_t boneData, int32_t boneCount, const FTransform& c2w, const Vector3& rootPos, int boneIdx, Vector3& outWorld) {
    if (!boneData || boneIdx < 0 || boneIdx >= boneCount) return false;
    FTransform bone = ReadUETransform(boneData + static_cast<uint64_t>(boneIdx) * offsets::FTransformSize);
    Vector3 world = TransformPoint(c2w, bone.Translation);
    if (!std::isfinite(world.x) || !std::isfinite(world.y) || !std::isfinite(world.z)) return false;
    double dx = world.x - rootPos.x, dy = world.y - rootPos.y, dz = world.z - rootPos.z;
    double distSq = dx * dx + dy * dy + dz * dz;
    if (distSq > (280.0 * 280.0)) return false;
    outWorld = world;
    return true;
}

inline bool GetSkeletonBoneWorld(const ActorInfo& info, const FTransform& c2w, uintptr_t boneOff, int boneIdx, Vector3& outWorld) {
    if (!info.mesh || !boneOff || boneIdx < 0) return false;
    uint64_t data = memory.read<uint64_t>(info.mesh + boneOff);
    int32_t count = memory.read<int32_t>(info.mesh + boneOff + 8);
    if (!data || count < 10 || count > 1024) return false;
    return GetSkeletonBoneWorldCached(data, count, c2w, info.pos, boneIdx, outWorld);
}

inline void DrawPlayerSkeleton(ActorInfo& info, ImU32 color) {
    if (!info.mesh) return;

    FTransform c2w{};
    uintptr_t c2wOff = info.c2wOff;
    if (c2wOff) {
        c2w = ReadUETransform(info.mesh + c2wOff);
        Vector3 t = c2w.Translation;
        double dx = t.x - info.pos.x, dy = t.y - info.pos.y, dz = t.z - info.pos.z;
        if (!std::isfinite(t.x) || (dx * dx + dy * dy + dz * dz) > (500.0 * 500.0))
            c2wOff = 0;
    }
    if (!c2wOff) {
        if (!ReadComponentToWorld(info.mesh, info.pos, c2wOff, c2w)) return;
        info.c2wOff = c2wOff;
    }

    uintptr_t boneOff = info.boneArrayOff;
    uint64_t boneData = 0;
    int32_t boneCount = 0;
    auto loadArray = [&](uintptr_t off) -> bool {
        boneData = memory.read<uint64_t>(info.mesh + off);
        boneCount = memory.read<int32_t>(info.mesh + off + 8);
        return boneData && boneCount >= 10 && boneCount <= 1024;
    };

    if (!boneOff || !loadArray(boneOff)) {
        boneOff = 0;
        constexpr uintptr_t candidates[] = { 0xA18, offsets::BoneArrayCache, offsets::BoneArray };
        for (uintptr_t o : candidates) {
            if (!loadArray(o)) continue;
            Vector3 test{};
            if (GetSkeletonBoneWorldCached(boneData, boneCount, c2w, info.pos, Bones::Head, test)) {
                boneOff = o;
                info.boneArrayOff = o;
                break;
            }
        }
    }
    if (!boneOff || !boneData) return;

    auto bone = [&](int idx, Vector3& out) -> bool {
        return GetSkeletonBoneWorldCached(boneData, boneCount, c2w, info.pos, idx, out);
    };

    Vector3 head{}, neck{}, spineU{}, spineM{}, pelvis{};
    Vector3 thighL{}, calfL{}, footL{}, thighR{}, calfR{}, footR{};

    const bool hasHead = bone(Bones::Head, head);
    const bool hasNeck = bone(Bones::Neck, neck);
    const bool hasSpineU = bone(Bones::SpineUpper, spineU);
    const bool hasSpineM = bone(Bones::SpineMid, spineM);
    const bool hasPelvis = bone(Bones::Pelvis, pelvis);
    const bool hasThighL = bone(Bones::ThighL, thighL);
    const bool hasCalfL = bone(Bones::CalfL, calfL);
    const bool hasFootL = bone(Bones::FootL, footL);
    const bool hasThighR = bone(Bones::ThighR, thighR);
    const bool hasCalfR = bone(Bones::CalfR, calfR);
    const bool hasFootR = bone(Bones::FootR, footR);

    if (hasHead && hasNeck) DrawSkelSegment(head, neck, color);
    if (hasNeck && hasSpineU) DrawSkelSegment(neck, spineU, color);
    else if (hasNeck && hasSpineM) DrawSkelSegment(neck, spineM, color);
    else if (hasNeck && hasPelvis) DrawSkelSegment(neck, pelvis, color);

    if (hasSpineU && hasSpineM) DrawSkelSegment(spineU, spineM, color);
    if (hasSpineM && hasPelvis) DrawSkelSegment(spineM, pelvis, color);
    else if (hasSpineU && hasPelvis) DrawSkelSegment(spineU, pelvis, color);

    if (hasPelvis && hasThighL) DrawSkelSegment(pelvis, thighL, color);
    if (hasThighL && hasCalfL) DrawSkelSegment(thighL, calfL, color);
    if (hasCalfL && hasFootL) DrawSkelSegment(calfL, footL, color);

    if (hasPelvis && hasThighR) DrawSkelSegment(pelvis, thighR, color);
    if (hasThighR && hasCalfR) DrawSkelSegment(thighR, calfR, color);
    if (hasCalfR && hasFootR) DrawSkelSegment(calfR, footR, color);

    if (hasHead) {
        Vector2 hs = ProjectWorldToScreen(head);
        if (!(hs.x == 0.f && hs.y == 0.f))
            ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(hs.x, hs.y), 3.0f, color, 10);
    }
}

struct FRotator {
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;

    FRotator() = default;
    FRotator(double p, double y, double r) : pitch(p), yaw(y), roll(r) {}

    FRotator operator-(const FRotator& other) const {
        return FRotator(pitch - other.pitch, yaw - other.yaw, roll - other.roll);
    }

    FRotator& operator-=(const FRotator& other) {
        pitch -= other.pitch;
        yaw -= other.yaw;
        roll -= other.roll;
        return *this;
    }

    double clampAxis(double angle) const {
        angle = fmod(angle, 360.0);
        if (angle < 0.0) angle += 360.0;
        return angle;
    }

    double normalizeAxis(double angle) const {
        angle = clampAxis(angle);
        if (angle > 180.0) angle -= 360.0;
        return angle;
    }

    void normalize() {
        pitch = normalizeAxis(pitch);
        yaw = normalizeAxis(yaw);
        roll = normalizeAxis(roll);
    }
};

inline FRotator FindLookAtRotation(Vector3 start, Vector3 target) {
    Vector3 direction = target - start;

    double length = sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (length > 0.0001) {
        direction.x /= length;
        direction.y /= length;
        direction.z /= length;
    }

    constexpr double PI = 3.14159265358979323846;
    double yaw = atan2(direction.y, direction.x) * (180.0 / PI);
    double pitch = atan2(direction.z, sqrt(direction.x * direction.x + direction.y * direction.y)) * (180.0 / PI);

    return FRotator(pitch, yaw, 0.0);
}

inline FRotator GetCameraRotationForAim() {
    return FRotator(vCamera.Rotation.x, vCamera.Rotation.y, vCamera.Rotation.z);
}

struct AimTarget {
    bool valid = false;
    Vector2 screenPos;
    Vector3 worldPos;
    float distance;
    uintptr_t mesh;
    uintptr_t pawn;
    int playerId;
    bool inVehicle = false;
};

inline AimTarget currentTarget = {};
inline std::vector<uintptr_t> g_espBoxPawns;
inline bool EspBoxDrawnFor(uintptr_t pawn) {
    for (uintptr_t a : g_espBoxPawns) if (a == pawn) return true;
    return false;
}
inline AimTarget lockedTarget = {};
inline bool isTargetLocked = false;

struct AimVelocitySample {
    uintptr_t pawn = 0;
    Vector3 lastPos{};
    Vector3 vel{};
    double lastTime = 0.0;
    bool hasSample = false;
};
inline AimVelocitySample g_aimVel[32]{};

inline double AimNowSeconds() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

inline AimVelocitySample* FindOrCreateVelSample(uintptr_t pawn) {
    if (!pawn) return nullptr;
    for (auto& s : g_aimVel) {
        if (s.pawn == pawn) return &s;
    }
    for (auto& s : g_aimVel) {
        if (!s.pawn) {
            s = {};
            s.pawn = pawn;
            return &s;
        }
    }
    g_aimVel[0] = {};
    g_aimVel[0].pawn = pawn;
    return &g_aimVel[0];
}

inline Vector3 UpdateAndPredictAimPos(uintptr_t pawn, Vector3 bonePos, float distMeters, uintptr_t vehicleAddr = 0) {
    AimVelocitySample* s = FindOrCreateVelSample(pawn);
    if (!s) return bonePos;

    const double now = AimNowSeconds();

    Vector3 engineVel{};
    bool haveEngine = false;
    if (vehicleAddr) haveEngine = ReadActorVelocity(vehicleAddr, engineVel);
    if (!haveEngine) haveEngine = ReadActorVelocity(pawn, engineVel);

    if (haveEngine) {
        const double a = 0.65;
        if (s->hasSample) {
            s->vel.x = s->vel.x * (1.0 - a) + engineVel.x * a;
            s->vel.y = s->vel.y * (1.0 - a) + engineVel.y * a;
            s->vel.z = s->vel.z * (1.0 - a) + engineVel.z * a;
        } else {
            s->vel = engineVel;
        }
        s->hasSample = true;
    } else if (s->hasSample) {
        const double dt = now - s->lastTime;
        Vector3 raw{};
        if (pred::EstimateLeadVelocity(s->lastPos, bonePos, dt, raw)) {
            const double a = vehicleAddr ? 0.55 : 0.35;
            s->vel.x = s->vel.x * (1.0 - a) + raw.x * a;
            s->vel.y = s->vel.y * (1.0 - a) + raw.y * a;
            s->vel.z = s->vel.z * (1.0 - a) + raw.z * a;
        }
    } else {
        s->hasSample = true;
    }
    s->lastPos = bonePos;
    s->lastTime = now;

    if (!aimbotPrediction) return bonePos;

    const Vector3 origin = vCameraLive.Location.x != 0.0 || vCameraLive.Location.y != 0.0
        ? vCameraLive.Location
        : vCamera.Location;
    const double speedCm = static_cast<double>(ReadLocalWeaponBulletSpeedCm());
    const double gravity = static_cast<double>(bulletGravityCm);

    Vector3 aim{};
    double flight = 0.0;
    if (pred::PredictHeadAim(origin, bonePos, s->vel, speedCm, gravity, aim, flight))
        return aim;

    Vector3 lead{};
    if (pred::PredictHeadLead(origin, bonePos, s->vel, speedCm, lead))
        return lead;

    float speedMs = static_cast<float>(speedCm * 0.01);
    if (speedMs < 50.f) speedMs = 50.f;
    float t = distMeters / speedMs;
    if (vehicleAddr) t += 0.045f;
    if (t > 0.85f) t = 0.85f;
    return Vector3(
        bonePos.x + s->vel.x * t,
        bonePos.y + s->vel.y * t,
        bonePos.z + s->vel.z * t);
}

inline Vector3 GetAimWorldPos(const ActorInfo& p) {
    const bool wantHead = (aimbotBoneIndex == Bones::Head || aimbotBoneIndex == 110 || aimbotBoneIndex == 68 || aimbotBoneIndex == 9);
    const bool wantChest = (aimbotBoneIndex == Bones::Neck || aimbotBoneIndex == Bones::SpineUpper || aimbotBoneIndex == 66 || aimbotBoneIndex == 5 || aimbotBoneIndex == 7 || aimbotBoneIndex == 6);
    if (wantHead) {
        if (p.hasHead) return p.head;
        if (p.chest.x != 0.0 || p.chest.y != 0.0 || p.chest.z != 0.0) return p.chest;
        return p.head;
    }
    if (wantChest) {
        if (p.chest.x != 0.0 || p.chest.y != 0.0 || p.chest.z != 0.0) return p.chest;
        if (p.hasHead) return p.head;
        return p.pos;
    }
    return p.pos;
}

inline bool IsValidAimWorld(const Vector3& w) {
    if (!std::isfinite(w.x) || !std::isfinite(w.y) || !std::isfinite(w.z)) return false;
    if (w.x == 0.0 && w.y == 0.0 && w.z == 0.0) return false;
    if (!IsInFrontOfCamera(w)) return false;
    double wx = w.x - vCamera.Location.x;
    double wy = w.y - vCamera.Location.y;
    double wz = w.z - vCamera.Location.z;
    double distM = sqrt(wx * wx + wy * wy + wz * wz) / 100.0;
    if (distM < 1.5 || distM > 500.0) return false;
    return true;
}

inline bool IsValidAimScreen(const Vector2& scr, float cx, float cy, float* outCrossDist = nullptr) {
    if (scr.x == 0.f && scr.y == 0.f) return false;
    if (!std::isfinite(scr.x) || !std::isfinite(scr.y)) return false;
    if (scr.x < 0.f || scr.y < 0.f || scr.x > Width || scr.y > Height) return false;
    float dx = scr.x - cx;
    float dy = scr.y - cy;
    float crossDist = sqrtf(dx * dx + dy * dy);
    if (crossDist > aimbotFov) return false;
    if (outCrossDist) *outCrossDist = crossDist;
    return true;
}

inline void DrawHeadPredictionDots(std::vector<ActorInfo>& players) {
    if (!drawHeadPrediction || !aimbotPrediction) return;
    if (!boxEsp) return;

    const Vector3 origin = (vCameraLive.Location.x != 0.0 || vCameraLive.Location.y != 0.0)
        ? vCameraLive.Location
        : vCamera.Location;
    const double speedCm = static_cast<double>(ReadLocalWeaponBulletSpeedCm());
    const double gravity = static_cast<double>(bulletGravityCm);
    auto draw = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(
        (int)(headPredictionColor[0] * 255),
        (int)(headPredictionColor[1] * 255),
        (int)(headPredictionColor[2] * 255),
        (int)(headPredictionColor[3] * 255));

    for (auto& p : players) {
        if (!p.isPlayer || p.isVehicle || p.isTeammate) continue;
        if (p.isDead) continue;
        if (!p.isVisible) continue;
        if (p.pos.x == 0.0 && p.pos.y == 0.0 && p.pos.z == 0.0) continue;

        double dx = p.pos.x - vCamera.Location.x;
        double dy = p.pos.y - vCamera.Location.y;
        double dz = p.pos.z - vCamera.Location.z;
        double distMeters = sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
        if (distMeters > maxEspDistance) continue;

        constexpr double kCapsuleHalf = 96.0;
        Vector3 feet(p.pos.x, p.pos.y, p.pos.z - kCapsuleHalf);
        Vector3 head = p.hasHead
            ? p.head
            : Vector3(p.pos.x, p.pos.y, p.pos.z + kCapsuleHalf + 14.0);
        if (head.z < feet.z + 110.0)
            head.z = feet.z + 170.0;

        Vector2 sf = ProjectWorldToScreen(feet);
        Vector2 sh = ProjectWorldToScreen(head);
        if ((sf.x == 0.f && sf.y == 0.f) || (sh.x == 0.f && sh.y == 0.f)) continue;
        if (!std::isfinite(sf.x) || !std::isfinite(sh.x)) continue;

        float boxH = fabsf(sf.y - sh.y);
        if (boxH < 2.f || boxH > Height * 2.f) continue;
        float pad = boxH * 0.06f;
        float top = (std::min)(sf.y, sh.y) - pad;
        boxH += pad * 2.f;
        float boxW = boxH * 0.55f;
        float midX = (sf.x + sh.x) * 0.5f;
        float boxMinX = midX - boxW * 0.5f;
        float boxMinY = top;
        if (boxMinX + boxW < 0.f || boxMinX > (float)Width ||
            boxMinY + boxH < 0.f || boxMinY > (float)Height)
            continue;

        Vector3 bone = GetAimWorldPos(p);
        if (!IsValidAimWorld(bone)) continue;

        Vector3 aimWorld = UpdateAndPredictAimPos(p.addr, bone, static_cast<float>(distMeters), p.vehicleAddr);
        AimVelocitySample* s = FindOrCreateVelSample(p.addr);
        if (!s || !s->hasSample) continue;

        Vector3 aim{};
        double flight = 0.0;
        if (!pred::PredictHeadAim(origin, bone, s->vel, speedCm, gravity, aim, flight)) {
            if (!pred::PredictHeadLead(origin, bone, s->vel, speedCm, aim))
                continue;
            flight = 0.0;
        }

        Vector2 scr = ProjectWorldToScreen(aim);
        if (scr.x == 0.f && scr.y == 0.f) continue;
        if (!std::isfinite(scr.x) || !std::isfinite(scr.y)) continue;
        const float boxCx = boxMinX + boxW * 0.5f;
        const float boxCy = boxMinY + boxH * 0.5f;
        const float maxAway = (std::max)(boxH * 2.5f, 120.f);
        const float adx = scr.x - boxCx;
        const float ady = scr.y - boxCy;
        if (adx * adx + ady * ady > maxAway * maxAway) continue;

        draw->AddCircleFilled(ImVec2(scr.x, scr.y), predictionDotRadius, col, 16);
        if (showPredictionDetails) {
            char info[64];
            std::snprintf(info, sizeof(info), "%.0fm / %.2fs", distMeters, flight);
            draw->AddText(ImVec2(scr.x + 6.f, scr.y), IM_COL32(255, 255, 255, 230), info);
        }
        (void)aimWorld;
    }
}

inline void MoveMouseAim(float dx, float dy) {
    static float carryX = 0.f, carryY = 0.f;
    carryX += dx;
    carryY += dy;
    const LONG mx = static_cast<LONG>(carryX);
    const LONG my = static_cast<LONG>(carryY);
    carryX -= static_cast<float>(mx);
    carryY -= static_cast<float>(my);
    if (mx == 0 && my == 0) return;
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = mx;
    input.mi.dy = my;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(input));
}

inline float ComputeAimSmooth(float crossDistPx) {
    float base = (aimbotSmooth < 0.5f) ? 0.5f : aimbotSmooth;
    if (!aimbotAdaptiveSmooth) return base;
    float t = crossDistPx / (aimbotFov > 1.f ? aimbotFov : 1.f);
    if (t > 1.f) t = 1.f;
    return base * (1.6f - 1.05f * t);
}

inline bool g_localInVehicle = false;
inline void RefreshLocalInVehicle() {
    static bool s_state = false;
    static int s_streak = 0;
    bool now = false;
    if (LocalPtrs::Player) {
        uint64_t op = memory.read<uint64_t>(LocalPtrs::Player + offsets::PawnVehicleOperator);
        now = GetVehicleFromOperator(op) != 0;
    }
    if (now == s_state) s_streak = 0;
    else if (++s_streak >= 4) { s_state = now; s_streak = 0; }
    g_localInVehicle = s_state;
}
inline bool IsLocalInVehicle() { return g_localInVehicle; }

inline void Aimbot(std::vector<ActorInfo>& players) {
    currentTarget = {};
    if (!aimbotEnabled || !LocalPtrs::PlayerController) {
        isTargetLocked = false;
        lockedTarget = {};
        return;
    }

    const bool keyDown = (GetAsyncKeyState(aimbotKey) & 0x8000) != 0;
    if (!keyDown) {
        isTargetLocked = false;
        lockedTarget = {};
        if (!showAimLine) return;
    }

    Camera savedEspCam = vCamera;
    vCamera = vCameraLive;
    UpdateW2SCache();

    const float cx = Width * 0.5f;
    const float cy = Height * 0.5f;

    AimTarget best{};
    float bestScore = 1e12f;

    auto refreshAndAimPos = [&](ActorInfo& p, Vector3& outWorld, Vector2& outScr, float& outCross, float& outDistM) -> bool {
        if (!p.isPlayer || p.isVehicle || !p.addr) return false;
        if (teamCheckAim && p.isTeammate) return false;

        if (p.inVehicle || !p.hasHead)
            ResolvePlayerBones(p);
        else
            ReadActorWorldPos(p.addr, p.pos, &p.rootC2wOff);

        if (p.inVehicle && !p.hasHead && p.vehicleAddr) {
            Vector3 vpos{};
            if (ReadActorWorldPos(p.vehicleAddr, vpos)) {
                p.pos = vpos;
                p.head = Vector3(vpos.x, vpos.y, vpos.z + 110.0);
                p.chest = Vector3(vpos.x, vpos.y, vpos.z + 60.0);
                p.hasHead = true;
            }
        }

        static int s_hpTick = 0;
        if ((++s_hpTick & 3) == 0 || p.health < 0.f)
            ReadPawnHealth(p);
        if (p.isDead) return false;

        const bool wantHead = (aimbotBoneIndex == Bones::Head || aimbotBoneIndex == 110 || aimbotBoneIndex == 68 || aimbotBoneIndex == 9);
        if (wantHead && !p.hasHead) return false;

        Vector3 bone = GetAimWorldPos(p);
        if (!IsValidAimWorld(bone)) return false;

        double wx = bone.x - vCamera.Location.x;
        double wy = bone.y - vCamera.Location.y;
        double wz = bone.z - vCamera.Location.z;
        float distM = static_cast<float>(sqrt(wx * wx + wy * wy + wz * wz) / 100.0);

        Vector3 aimWorld = aimbotPrediction
            ? UpdateAndPredictAimPos(p.addr, bone, distM, p.inVehicle ? p.vehicleAddr : 0)
            : bone;
        if (!IsValidAimWorld(aimWorld)) aimWorld = bone;

        Vector2 scr = ProjectWorldToScreen(aimWorld);
        float crossDist = 0.f;
        if (!IsValidAimScreen(scr, cx, cy, &crossDist)) return false;

        outWorld = aimWorld;
        outScr = scr;
        outCross = crossDist;
        outDistM = distM;
        return true;
    };

    auto consider = [&](ActorInfo& p) {
        if (aimbotVisibleOnly && !p.isVisible) return;
        if (!EspBoxDrawnFor(p.addr)) return;

        Vector3 aimWorld{};
        Vector2 scr{};
        float crossDist = 0.f, worldDist = 0.f;
        if (!refreshAndAimPos(p, aimWorld, scr, crossDist, worldDist)) return;

        float score = (aimbotTargetPriority == 0)
            ? (worldDist + crossDist * 0.01f)
            : (crossDist + worldDist * 0.15f);

        if (score < bestScore) {
            bestScore = score;
            best.valid = true;
            best.screenPos = scr;
            best.worldPos = aimWorld;
            best.distance = worldDist;
            best.pawn = p.addr;
            best.mesh = p.mesh;
            best.playerId = static_cast<int>(p.addr & 0xFFFF);
            best.inVehicle = p.inVehicle;
        }
    };

    if (keyDown && aimbotLockTarget && isTargetLocked && lockedTarget.valid && lockedTarget.pawn) {
        bool stillThere = false;
        for (auto& p : players) {
            if (!p.isPlayer || p.addr != lockedTarget.pawn) continue;
            Vector3 aimWorld{};
            Vector2 scr{};
            float crossDist = 0.f, worldDist = 0.f;
            ResolvePlayerBones(p);
            if (teamCheckAim && p.isTeammate) break;
            if (!EspBoxDrawnFor(p.addr)) break;
            if (aimbotVisibleOnly && !p.isVisible) break;
            const bool wantHead = (aimbotBoneIndex == Bones::Head || aimbotBoneIndex == 110 || aimbotBoneIndex == 68 || aimbotBoneIndex == 9);
            if (p.inVehicle && !p.hasHead && p.vehicleAddr) {
                Vector3 vpos{};
                if (ReadActorWorldPos(p.vehicleAddr, vpos)) {
                    p.head = Vector3(vpos.x, vpos.y, vpos.z + 110.0);
                    p.chest = Vector3(vpos.x, vpos.y, vpos.z + 60.0);
                    p.hasHead = true;
                }
            }
            if (wantHead && !p.hasHead) break;
            Vector3 bone = GetAimWorldPos(p);
            if (!IsValidAimWorld(bone)) break;
            double wx = bone.x - vCamera.Location.x;
            double wy = bone.y - vCamera.Location.y;
            double wz = bone.z - vCamera.Location.z;
            worldDist = static_cast<float>(sqrt(wx * wx + wy * wy + wz * wz) / 100.0);
            aimWorld = aimbotPrediction
                ? UpdateAndPredictAimPos(p.addr, bone, worldDist, p.inVehicle ? p.vehicleAddr : 0)
                : bone;
            scr = ProjectWorldToScreen(aimWorld);
            if (scr.x == 0.f && scr.y == 0.f) break;
            if (!std::isfinite(scr.x) || !std::isfinite(scr.y)) break;
            if (scr.x < -80.f || scr.y < -80.f || scr.x > Width + 80.f || scr.y > Height + 80.f) break;
            lockedTarget.screenPos = scr;
            lockedTarget.worldPos = aimWorld;
            lockedTarget.distance = worldDist;
            lockedTarget.inVehicle = p.inVehicle;
            (void)crossDist;
            stillThere = true;
            break;
        }
        if (stillThere) best = lockedTarget;
        else { isTargetLocked = false; lockedTarget = {}; }
    }

    if (!best.valid) {
        for (auto& p : players) {
            if (!p.isPlayer) continue;
            consider(p);
        }
        if (best.valid && keyDown && aimbotLockTarget) {
            lockedTarget = best;
            isTargetLocked = true;
        }
    }

    if (!best.valid || !best.pawn) {
        isTargetLocked = false;
        lockedTarget = {};
        vCamera = savedEspCam;
        UpdateW2SCache();
        return;
    }
    currentTarget = best;

    if (!keyDown) {
        vCamera = savedEspCam;
        UpdateW2SCache();
        return;
    }

    float dx = best.screenPos.x - cx;
    float dy = best.screenPos.y - cy;
    float err = sqrtf(dx * dx + dy * dy);

    if (err <= aimbotDeadzone) {
        vCamera = savedEspCam;
        UpdateW2SCache();
        return;
    }

    float smooth = ComputeAimSmooth(err);
    if (smooth < 1.f) smooth = 1.f;
    if (best.inVehicle) {
        smooth *= 0.55f;
        if (smooth < 1.f) smooth = 1.f;
    }

    if (IsLocalInVehicle()) {
        float stepX = dx / smooth;
        float stepY = dy / smooth;
        float stepLen = sqrtf(stepX * stepX + stepY * stepY);
        const float maxStep = 45.f;
        if (stepLen > maxStep) {
            stepX = stepX * (maxStep / stepLen);
            stepY = stepY * (maxStep / stepLen);
        }
        MoveMouseAim(stepX, stepY);
        vCamera = savedEspCam;
        UpdateW2SCache();
        return;
    }

    FRotator desired = FindLookAtRotation(vCameraLive.Location, best.worldPos);
    sdk::FRotator cur = memory.read<sdk::FRotator>(LocalPtrs::PlayerController + offsets::ControllerControlRotation);
    auto normDelta = [](double a) {
        while (a > 180.0) a -= 360.0;
        while (a < -180.0) a += 360.0;
        return a;
    };
    cur.Pitch += normDelta(desired.pitch - cur.Pitch) / static_cast<double>(smooth);
    cur.Yaw += normDelta(desired.yaw - cur.Yaw) / static_cast<double>(smooth);
    cur.Roll = 0.0;
    memory.write<sdk::FRotator>(LocalPtrs::PlayerController + offsets::ControllerControlRotation, cur);

    vCamera = savedEspCam;
    UpdateW2SCache();
}

inline void DrawAimLine() {
    if (!showAimLine || !aimbotEnabled || !currentTarget.valid || !currentTarget.pawn) return;
    if ((GetAsyncKeyState(aimbotKey) & 0x8000) != 0) return;
    if (!IsValidAimWorld(currentTarget.worldPos)) return;
    if (!EspBoxDrawnFor(currentTarget.pawn)) return;
    const float cx = Width * 0.5f;
    const float cy = Height * 0.5f;
    currentTarget.screenPos = ProjectWorldToScreen(currentTarget.worldPos);
    if (!IsValidAimScreen(currentTarget.screenPos, cx, cy)) return;

    auto draw = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(
        (int)(aimLineColor[0] * 255), (int)(aimLineColor[1] * 255),
        (int)(aimLineColor[2] * 255), (int)(aimLineColor[3] * 255));
    draw->AddLine(ImVec2(cx, cy), ImVec2(currentTarget.screenPos.x, currentTarget.screenPos.y), col, aimLineThickness);
    draw->AddCircleFilled(ImVec2(currentTarget.screenPos.x, currentTarget.screenPos.y), 3.5f, col, 10);
}

inline uint64_t FindProjectileSubsystem() {
    static uint64_t cached = 0;
    static uintptr_t cachedWorld = 0;
    static int scanCooldown = 0;

    if (!LocalPtrs::Gworld || !memory.BaseAddress) return 0;
    if (LocalPtrs::Gworld != cachedWorld) {
        cached = 0;
        cachedWorld = LocalPtrs::Gworld;
        scanCooldown = 0;
    }

    const uint64_t cls = ObjectAtIndex(static_cast<int>(offsets::ProjectileSubsystemClassIndex));
    if (!cls) return 0;

    if (cached) {
        uint64_t c = memory.read<uint64_t>(cached + offsets::UObject::Class);
        if (c == cls) {
            uint64_t outer = memory.read<uint64_t>(cached + offsets::UObject::Outer);
            if (outer == LocalPtrs::Gworld) return cached;
        }
        cached = 0;
    }

    if ((scanCooldown++ % 30) != 0 && scanCooldown > 1) return 0;

    const int num = memory.read<int>(memory.BaseAddress + offsets::NumElements);
    if (num <= 0 || num > 5'000'000) return 0;

    const int start = (std::max)(0, static_cast<int>(offsets::ProjectileSubsystemClassIndex) - 2000);
    const int end = (std::min)(num, start + 50000);
    for (int i = start; i < end; ++i) {
        uint64_t obj = ObjectAtIndex(i);
        if (!obj || obj == cls) continue;
        if (memory.read<uint64_t>(obj + offsets::UObject::Class) != cls) continue;
        if (memory.read<uint64_t>(obj + offsets::UObject::Outer) != LocalPtrs::Gworld) continue;
        cached = obj;
        return cached;
    }

    for (int i = 0; i < num && i < 300000; i += 7) {
        uint64_t obj = ObjectAtIndex(i);
        if (!obj || obj == cls) continue;
        if (memory.read<uint64_t>(obj + offsets::UObject::Class) != cls) continue;
        if (memory.read<uint64_t>(obj + offsets::UObject::Outer) != LocalPtrs::Gworld) continue;
        cached = obj;
        return cached;
    }
    return 0;
}

inline bool RetargetProjectile(uint64_t inst, const sdk::FVector& target) {
    if (!inst) return false;
    if (memory.read<uint8_t>(inst + offsets::ProjectileState) != offsets::ProjectileStateFlying)
        return false;
    if (memory.read<uint8_t>(inst + offsets::ProjectileExternalMove) != 0)
        return false;

    const double ft = memory.read<double>(inst + offsets::ProjectileFlightTime);
    if (!std::isfinite(ft) || ft <= 0.0) return false;

    sdk::FVector loc = memory.read<sdk::FVector>(inst + offsets::ProjectileLocation);
    sdk::FVector vel = memory.read<sdk::FVector>(inst + offsets::ProjectileVelocity);
    if (!std::isfinite(loc.X) || !std::isfinite(vel.X)) return false;

    const double s = sqrt(vel.X * vel.X + vel.Y * vel.Y + vel.Z * vel.Z);
    if (s < 100.0 || s > 200000.0) return false;

    const double dx = target.X - loc.X;
    const double dy = target.Y - loc.Y;
    const double dz = target.Z - loc.Z;
    const double dist = sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 100.0) return false;

    const double inv = s / dist;
    sdk::FVector nvel{};
    nvel.X = dx * inv;
    nvel.Y = dy * inv;
    nvel.Z = dz * inv;
    memory.write<sdk::FVector>(inst + offsets::ProjectileVelocity, nvel);
    return true;
}

inline int RetargetAllProjectiles(uint64_t subsystem, const sdk::FVector& target) {
    if (!subsystem) return 0;
    const uint64_t pool = memory.read<uint64_t>(subsystem + offsets::ProjectilePoolData);
    const int32_t num = memory.read<int32_t>(subsystem + offsets::ProjectilePoolNum);
    if (!pool || num <= 0 || num > 4096) return 0;

    uint32_t bits[128]{};
    const uint32_t words = (static_cast<uint32_t>(num) + 31u) / 32u;
    const uint32_t useWords = (std::min)(words, 128u);
    const uint64_t heap = memory.read<uint64_t>(subsystem + offsets::ProjectilePoolBitsHeap);
    const uint64_t bitBase = heap ? heap : (subsystem + offsets::ProjectilePoolBitsInline);
    for (uint32_t w = 0; w < useWords; ++w)
        bits[w] = memory.read<uint32_t>(bitBase + static_cast<uint64_t>(w) * 4u);

    int hit = 0;
    for (int32_t i = 0; i < num; ++i) {
        const uint32_t wi = static_cast<uint32_t>(i) >> 5;
        if (wi >= useWords) break;
        if (((bits[wi] >> (i & 31)) & 1u) == 0) continue;
        const uint64_t inst = pool + static_cast<uint64_t>(i) * offsets::ProjectileInstanceSize;
        if (RetargetProjectile(inst, target)) ++hit;
    }
    return hit;
}

inline bool PickMagicBulletTarget(std::vector<ActorInfo>& players, sdk::FVector& out) {
    static uintptr_t stickyPawn = 0;
    static Vector3 stickyWorld{};

    const bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (!firing) {
        stickyPawn = 0;
        stickyWorld = {};
    }

    const float cx = Width * 0.5f;
    const float cy = Height * 0.5f;
    const float maxCross = (std::max)(Width, Height);

    auto tryPawn = [&](ActorInfo& p, Vector3& boneOut, float& crossOut) -> bool {
        if (!p.isPlayer || p.isVehicle || !p.addr) return false;
        if (teamCheckAim && p.isTeammate) return false;
        if (aimbotVisibleOnly && !p.isVisible) return false;
        ResolvePlayerBones(p);
        if (p.inVehicle && !p.hasHead && p.vehicleAddr) {
            Vector3 vpos{};
            if (ReadActorWorldPos(p.vehicleAddr, vpos)) {
                p.head = Vector3(vpos.x, vpos.y, vpos.z + 110.0);
                p.chest = Vector3(vpos.x, vpos.y, vpos.z + 60.0);
                p.hasHead = true;
            }
        }
        ReadPawnHealth(p);
        if (p.isDead) return false;
        Vector3 bone = GetAimWorldPos(p);
        if (!IsValidAimWorld(bone)) return false;
        double wx = bone.x - vCamera.Location.x;
        double wy = bone.y - vCamera.Location.y;
        double wz = bone.z - vCamera.Location.z;
        float distM = static_cast<float>(sqrt(wx * wx + wy * wy + wz * wz) / 100.0);
        if (aimbotPrediction)
            bone = UpdateAndPredictAimPos(p.addr, bone, distM, p.inVehicle ? p.vehicleAddr : 0);
        Vector2 scr = ProjectWorldToScreen(bone);
        if (scr.x == 0.f && scr.y == 0.f) return false;
        if (!std::isfinite(scr.x) || !std::isfinite(scr.y)) return false;
        if (scr.x < -40.f || scr.y < -40.f || scr.x > Width + 40.f || scr.y > Height + 40.f) return false;
        float dx = scr.x - cx;
        float dy = scr.y - cy;
        float cross = sqrtf(dx * dx + dy * dy);
        if (cross > maxCross) return false;
        boneOut = bone;
        crossOut = cross;
        return true;
    };

    if (firing && stickyPawn) {
        for (auto& p : players) {
            if (p.addr != stickyPawn) continue;
            Vector3 bone{};
            float cross = 0.f;
            if (tryPawn(p, bone, cross)) {
                stickyWorld = bone;
                out.X = bone.x; out.Y = bone.y; out.Z = bone.z;
                return true;
            }
            break;
        }
        stickyPawn = 0;
    }

    float bestCross = 1e12f;
    Vector3 bestWorld{};
    uintptr_t bestPawn = 0;
    for (auto& p : players) {
        Vector3 bone{};
        float cross = 0.f;
        if (!tryPawn(p, bone, cross)) continue;
        if (cross < bestCross) {
            bestCross = cross;
            bestWorld = bone;
            bestPawn = p.addr;
        }
    }
    if (!bestPawn) return false;

    stickyPawn = bestPawn;
    stickyWorld = bestWorld;
    out.X = bestWorld.x;
    out.Y = bestWorld.y;
    out.Z = bestWorld.z;
    return true;
}

inline void MagicBullet(std::vector<ActorInfo>& players) {
    if (!magicBullet) return;
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) return;

    sdk::FVector target{};
    if (!PickMagicBulletTarget(players, target)) return;

    const uint64_t sub = FindProjectileSubsystem();
    if (!sub) return;
    RetargetAllProjectiles(sub, target);
}

namespace radar {
    inline void RotatePoint(Vector2* screen, Vector3 origin, bool rotationMode) {
        Vector3 delta = origin - vCamera.Location;
        double angle = atan2(delta.y, delta.x);

        if (rotationMode) {
            double playerRadians = vCamera.Rotation.y * 3.14159265358979323846 / 180.0;
            angle -= playerRadians;
        }

        double distance = sqrt(delta.x * delta.x + delta.y * delta.y) * 0.01;
        double scaledDistance = (distance / radarRange) * (radarSize / 2.0);
        if (scaledDistance > radarSize / 2.0) scaledDistance = radarSize / 2.0;

        double x, y;
        if (rotationMode) {
            x = scaledDistance * sin(angle);
            y = -scaledDistance * cos(angle);
        } else {
            x = scaledDistance * cos(angle);
            y = scaledDistance * sin(angle);
        }

        float radarCenterX = radarPositionX + (radarSize / 2.0f);
        float radarCenterY = radarPositionY + (radarSize / 2.0f);

        screen->x = static_cast<float>(radarCenterX + x);
        screen->y = static_cast<float>(radarCenterY + y);
    }

    inline void DrawGrid(ImDrawList* drawList, ImVec2 radarPos, float radarSize) {
        float gridSpacing = radarSize / radarGridDivisions;
        ImU32 gridColor = IM_COL32(255, 255, 255, 30);

        for (int i = 1; i < radarGridDivisions; i++) {
            float x = radarPos.x + (i * gridSpacing);
            drawList->AddLine(ImVec2(x, radarPos.y), ImVec2(x, radarPos.y + radarSize), gridColor, 0.5f);
        }

        for (int i = 1; i < radarGridDivisions; i++) {
            float y = radarPos.y + (i * gridSpacing);
            drawList->AddLine(ImVec2(radarPos.x, y), ImVec2(radarPos.x + radarSize, y), gridColor, 0.5f);
        }
    }

    inline void DrawBackground() {
        if (!radarEnabled) return;

        auto drawList = ImGui::GetForegroundDrawList();
        ImVec2 radarPos(radarPositionX, radarPositionY);
        float radarSizeF = (float)radarSize;

        ImU32 bgColor = IM_COL32(0, 0, 0, (int)(radarOpacity * 0.5f));
        drawList->AddRectFilled(radarPos, ImVec2(radarPos.x + radarSizeF, radarPos.y + radarSizeF), bgColor);

        drawList->AddRect(radarPos, ImVec2(radarPos.x + radarSizeF, radarPos.y + radarSizeF), IM_COL32(5, 5, 5, 255), 0.0f, 0, 1.0f);

        float radarCenterX = radarPos.x + radarSizeF / 2.0f;
        float radarCenterY = radarPos.y + radarSizeF / 2.0f;
        ImVec2 radarCenter(radarCenterX, radarCenterY);

        drawList->AddLine(ImVec2(radarPos.x, radarPos.y - 2.0f), ImVec2(radarPos.x + radarSizeF, radarPos.y - 2.0f), IM_COL32(144, 154, 251, 255), 2.0f);

        if (radarGrid) {
            DrawGrid(drawList, radarPos, radarSizeF);
        } else {
            ImU32 lineColor = IM_COL32(5, 5, 5, 255);
            drawList->AddLine(ImVec2(radarPos.x, radarCenterY), ImVec2(radarPos.x + radarSizeF, radarCenterY), lineColor, 1.0f);
            drawList->AddLine(ImVec2(radarCenterX, radarPos.y), ImVec2(radarCenterX, radarPos.y + radarSizeF), lineColor, 1.0f);
        }

        float fov = vCamera.FieldOfView;
        float halfFovRadians = (fov / 2.0f) * 3.14159265358979323846f / 180.0f;
        float distanceToTop = radarCenterY - radarPos.y;
        float xOffset = distanceToTop * tanf(halfFovRadians);

        ImVec2 leftEnd(radarCenterX - xOffset, radarPos.y);
        ImVec2 rightEnd(radarCenterX + xOffset, radarPos.y);
        leftEnd.x = std::clamp(leftEnd.x, radarPos.x, radarPos.x + radarSizeF);
        rightEnd.x = std::clamp(rightEnd.x, radarPos.x, radarPos.x + radarSizeF);

        drawList->AddLine(radarCenter, leftEnd, IM_COL32(239, 187, 255, 255), 1.5f);
        drawList->AddLine(radarCenter, rightEnd, IM_COL32(239, 187, 255, 255), 1.5f);

        drawList->AddCircleFilled(radarCenter, 3.0f, IM_COL32(195, 177, 255, 255), 15);
    }

    inline void RenderPlayer(Vector3 playerPos, float distance, ImU32 color, bool isBot) {
        if (!radarEnabled) return;

        Vector2 radarScreen;
        RotatePoint(&radarScreen, playerPos, radarRotationMode);

        auto drawList = ImGui::GetForegroundDrawList();
        drawList->AddCircleFilled(ImVec2(radarScreen.x, radarScreen.y), 3.0f, color, 15);

        if (radarDistance) {
            char distText[32];
            snprintf(distText, sizeof(distText), "(%.0fm)", distance);
            ImVec2 textSize = ImGui::CalcTextSize(distText);
            ImVec2 textPos(radarScreen.x - textSize.x / 2.0f, radarScreen.y + 2.0f);

            ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
            for (float dx = -1.0f; dx <= 1.0f; dx++) {
                for (float dy = -1.0f; dy <= 1.0f; dy++) {
                    if (dx == 0.0f && dy == 0.0f) continue;
                    drawList->AddText(ImVec2(textPos.x + dx, textPos.y + dy), outlineColor, distText);
                }
            }
            drawList->AddText(textPos, color, distText);
        }
    }
}

inline uint32_t decrypt_count()
{
    return memory.read<uint32_t>(memory.BaseAddress + 0x19396E50) ^ 0x1F8663B0;
}

inline uint64_t decrypt_table()
{
    uint64_t encrypted = memory.read<uint64_t>(memory.BaseAddress + 0x19396E60);
    return std::rotl(encrypted, 32) ^ 0x30D1AC9BD3DEA28FULL;
}

inline void ActorLoop() {
    static uint32_t s_espFrame = 0;
    ++s_espFrame;

    RefreshLocalInVehicle();
    RefreshEspCamera(g_localInVehicle);

    DrawFovCircle();

    const bool doVis = visCheckEsp || aimbotVisibleOnly || drawHeadPrediction || showAimLine;
    const double worldSeconds = doVis ? ReadWorldTimeSeconds() : 0.0;
    std::vector<ActorInfo> local_cache;
    {
        std::lock_guard<std::mutex> lk(g_actor_cache_mutex);
        if (doVis) VisTick(g_actor_cache, worldSeconds);
        local_cache = g_actor_cache;
    }

    for (auto& p : local_cache) {
        if (p.isVehicle) {
            if (p.isTeammate && !teamVehicleEsp) continue;
            if (!p.isTeammate && !enemyVehicleEsp) continue;
            Vector3 fresh{};
            if (ReadActorWorldPos(p.addr, fresh, &p.rootC2wOff))
                p.pos = fresh;
            continue;
        }

        if (!p.isPlayer) continue;
        if (p.isTeammate && !showTeammates) continue;
        if (p.inVehicle && !vehicleOccupantEsp) continue;
        if (!ReadActorWorldPos(p.addr, p.pos, &p.rootC2wOff)) {
            p.pos = Vector3();
            continue;
        }

        double dx = p.pos.x - vCamera.Location.x;
        double dy = p.pos.y - vCamera.Location.y;
        double dz = p.pos.z - vCamera.Location.z;
        double distMeters = sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
        if (distMeters > maxEspDistance) continue;

        const bool wantSkel = boneEsp && distMeters <= maxBoneDistance;
        const bool wantBoxBones = boxEsp && distMeters <= maxEspDistance;
        if (wantSkel || wantBoxBones) {
            ResolvePlayerBones(p);
        } else {
            p.head = Vector3(p.pos.x, p.pos.y, p.pos.z + 96.0);
            p.chest = Vector3(p.pos.x, p.pos.y, p.pos.z + 40.0);
            p.hasHead = false;
        }

        if (((s_espFrame + static_cast<uint32_t>(p.addr)) & 3u) == 0u || p.health < 0.f)
            ReadPawnHealth(p);
    }

    RefreshEspCamera(g_localInVehicle);

    g_espBoxPawns.clear();
    for (auto& p : local_cache) {
        if (p.isVehicle) {
            if (p.isTeammate && !teamVehicleEsp) continue;
            if (!p.isTeammate && !enemyVehicleEsp) continue;
            if (p.pos.x == 0.0 && p.pos.y == 0.0 && p.pos.z == 0.0)
                continue;

            double vdx = p.pos.x - vCamera.Location.x;
            double vdy = p.pos.y - vCamera.Location.y;
            double vdz = p.pos.z - vCamera.Location.z;
            double vDist = sqrt(vdx * vdx + vdy * vdy + vdz * vdz) / 100.0;
            if (vDist > maxVehicleEspDistance) continue;

            float valpha = EspAlphaForDistance(static_cast<float>(vDist), maxVehicleEspDistance);
            const float* vcol = p.isTeammate ? teamVehicleEspColor : enemyVehicleEspColor;
            ImU32 vColor = WithAlpha(IM_COL32(
                (int)(vcol[0] * 255), (int)(vcol[1] * 255),
                (int)(vcol[2] * 255), 255), valpha);

            Vector2 sp = ProjectWorldToScreen(p.pos);
            if (sp.x == 0 && sp.y == 0) continue;
            float boxH = static_cast<float>((220.0 / (vDist + 1.0)) * 12.0);
            if (boxH < 10.f) boxH = 10.f;
            if (boxH > Height * 0.45f) boxH = Height * 0.45f;
            float boxW = boxH * 1.55f;
            float minX = sp.x - boxW * 0.5f;
            float minY = sp.y - boxH * 0.5f;
            float prevTh = espBoxThickness;
            espBoxThickness = vehicleEspThickness;
            DrawBox(minX, minY, boxW, boxH, vColor);
            espBoxThickness = prevTh;
            char label[96];
            if (p.occupantCount > 0)
                std::snprintf(label, sizeof(label), "%s  %.0fm  [%d]",
                    p.isTeammate ? "TEAM VEH" : "ENEMY VEH", vDist, p.occupantCount);
            else
                std::snprintf(label, sizeof(label), "%s  %.0fm",
                    p.isTeammate ? "TEAM VEH" : "ENEMY VEH", vDist);
            DrawTextOutlined(label, minX, minY - 16.0f, vColor);
            continue;
        }

        if (!p.isPlayer) continue;
        if (p.isTeammate && !showTeammates) continue;
        if (p.inVehicle && !vehicleOccupantEsp) continue;
        if (p.pos.x == 0.0 && p.pos.y == 0.0 && p.pos.z == 0.0) continue;

        double dx = p.pos.x - vCamera.Location.x;
        double dy = p.pos.y - vCamera.Location.y;
        double dz = p.pos.z - vCamera.Location.z;
        double distMeters = sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
        if (distMeters > maxEspDistance) continue;
        if (p.isDead) continue;

        g_espBoxPawns.push_back(p.addr);

        const bool wantSkel = boneEsp && distMeters <= maxBoneDistance;

        float alpha = EspAlphaForDistance(static_cast<float>(distMeters), maxEspDistance);
        if (visCheckEsp && !p.isVisible)
            alpha *= 0.35f;
        const float* col4 = p.isTeammate ? teammateEspColor : enemyEspColor;
        ImU32 color = WithAlpha(IM_COL32(
            (int)(col4[0] * 255),
            (int)(col4[1] * 255),
            (int)(col4[2] * 255),
            255), alpha);

        float boxMinX = 0, boxMinY = 0, boxW = 0, boxH = 0;
        bool haveBox = false;

        if (boxEsp) {
            constexpr double kCapsuleHalf = 96.0;
            Vector3 feet(p.pos.x, p.pos.y, p.pos.z - kCapsuleHalf);
            Vector3 head = p.hasHead
                ? p.head
                : Vector3(p.pos.x, p.pos.y, p.pos.z + kCapsuleHalf + 14.0);
            if (head.z < feet.z + 110.0)
                head.z = feet.z + 170.0;

            Vector2 sf = ProjectWorldToScreen(feet);
            Vector2 sh = ProjectWorldToScreen(head);
            if (!(sf.x == 0 && sf.y == 0) && !(sh.x == 0 && sh.y == 0)) {
                float h = fabsf(sf.y - sh.y);
                float pad = h * 0.06f;
                float top = (std::min)(sf.y, sh.y) - pad;
                h += pad * 2.f;
                float w = h * 0.55f;
                float midX = (sf.x + sh.x) * 0.5f;
                if (h >= 2.f && w >= 2.f && h < Height * 2.f) {
                    haveBox = true;
                    boxMinX = midX - w * 0.5f;
                    boxMinY = top;
                    boxW = w;
                    boxH = h;
                    DrawBox(boxMinX, boxMinY, boxW, boxH, color);
                    DrawHealthBar(boxMinX, boxMinY, boxH, p.health, p.maxHealth, alpha);

                    if (showDistance || p.inVehicle) {
                        char label[64];
                        if (p.inVehicle)
                            std::snprintf(label, sizeof(label), "%.0fm  SEATED", distMeters);
                        else
                            std::snprintf(label, sizeof(label), "%.0fm", distMeters);
                        DrawTextOutlined(label, boxMinX + boxW * 0.5f - 14.f, boxMinY - 16.f,
                            WithAlpha(IM_COL32(235, 238, 245, 255), alpha), 13.f);
                    }
                    if (healthEsp && p.maxHealth > 1.f) {
                        char hp[32];
                        std::snprintf(hp, sizeof(hp), "%.0f", p.health);
                        DrawTextOutlined(hp, boxMinX - 28.f, boxMinY + boxH * 0.5f - 6.f,
                            WithAlpha(IM_COL32(230, 230, 235, 255), alpha), 12.f);
                    }
                    DrawSnaplineTo(boxMinX + boxW * 0.5f, boxMinY + boxH, WithAlpha(color, alpha * 0.55f));
                }
            }
        }

        if (!haveBox && snaplines) {
            Vector2 foot = ProjectWorldToScreen(p.pos);
            if (!(foot.x == 0 && foot.y == 0))
                DrawSnaplineTo(foot.x, foot.y, WithAlpha(color, alpha * 0.55f));
        }

        if (wantSkel) {
            DrawPlayerSkeleton(p, color);
        }
    }

    if (aimbotEnabled)
        Aimbot(local_cache);
    if (drawHeadPrediction)
        DrawHeadPredictionDots(local_cache);
    if (magicBullet)
        MagicBullet(local_cache);
    DrawCrosshair();
    DrawAimLine();
}
