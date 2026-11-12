#pragma once
#include <cstdint>

namespace offsets {
    inline constexpr uint64_t GWorldSlot              = 0x0D0E0E98;
    inline constexpr uint64_t GNamesSlot              = 0x0CE81F40;
    inline constexpr uint64_t GObjectsSlot            = 0x0CF54760;
    inline constexpr uint64_t GObjectsNumElementsSlot = 0x0CF54774;
    inline constexpr uint64_t GEngineSlot             = 0x0D0E4150;
    inline constexpr uint64_t GNames = GNamesSlot;
    inline constexpr uint64_t GWorld = GWorldSlot;
    inline constexpr uint64_t GObjects = GObjectsSlot;
    inline constexpr uint64_t NumElements = GObjectsNumElementsSlot;
    inline constexpr uint64_t GEngine = GEngineSlot;

    inline constexpr std::uint64_t PlayerBaseClassIndex = 6060;
    inline constexpr std::uint64_t VehicleBaseClassIndex = 3650;
    inline constexpr std::uint64_t ModularVehicleClassIndex = 3346;
    inline constexpr std::uint64_t ProjectileSubsystemClassIndex = 6119;

    inline constexpr std::uint64_t UObjectClass = 0x10;
    inline constexpr std::uint64_t UObjectNameId = 0x18;
    inline constexpr std::uint64_t UObjectFlags = 0x8;

    namespace NamePool {
        inline constexpr uint64_t kBlockIndexBias  = 2;
        inline constexpr uint64_t kNameEntryStride = 8;
        inline constexpr uint64_t kHeaderOffset    = 8;
        inline constexpr uint64_t kKindOffset      = 10;
        inline constexpr uint64_t kDataOffset      = 12;
        inline constexpr uint8_t  kFixedKind       = 2;
        inline constexpr uint64_t kFixedKindLen    = 22;
        inline constexpr uint64_t NamePoolBlocks = 0x00;
        inline constexpr uint64_t NameEntryHeader = 0x0;
        inline constexpr uint64_t NameEntryData = 0x2;
    }

    namespace UObject {
        inline constexpr uint64_t Flags   = 0x08;
        inline constexpr uint64_t Index   = 0x0C;
        inline constexpr uint64_t Class   = 0x10;
        inline constexpr uint64_t Name    = 0x18;
        inline constexpr uint64_t Outer   = 0x20;
    }

    namespace UStruct {
        inline constexpr uint64_t Super   = 0x40;
    }

    namespace UObjectArray {
        inline constexpr uint64_t ItemStride     = 0x18;
        inline constexpr uint64_t ItemObjectPtr  = 0x08;
        inline constexpr uint64_t ItemObjectAlt  = 0x10;
        inline constexpr uint64_t ObjectsPerChunk = 0x10000;
        inline constexpr uint64_t NumChunksOff    = 0x1C;
    }

    namespace World {
        inline constexpr uint64_t PersistentLevel     = 0x0030;
        inline constexpr uint64_t Levels              = 0x01C8;
        inline constexpr uint64_t LevelsAlt1          = 0x0120;
        inline constexpr uint64_t LevelsAlt2          = 0x0130;
        inline constexpr uint64_t GameState           = 0x01B0;
        inline constexpr uint64_t OwningGameInstance  = 0x0228;
        inline constexpr uint64_t TimeSeconds         = 0x0840;
        inline constexpr uint64_t DeltaTimeSeconds    = 0x0864;
        inline constexpr uint64_t TimeSecondsPadBegin = 0x03A8;
        inline constexpr uint64_t TimeSecondsPadEnd   = 0x08A0;
        inline constexpr uint64_t TimeSecondsCandidates[] = {
            0x0840, 0x0680, 0x0690, 0x06A0, 0x06B0, 0x06C0, 0x0700, 0x0720, 0x0740, 0x0780, 0x07A0
        };
    }

    namespace Level {
        inline constexpr uint64_t Actors              = 0x00A0;
    }

    namespace GameInstance {
        inline constexpr uint64_t LocalPlayers        = 0x0038;
        inline constexpr uint64_t GameInstanceAlt     = 0x12C8;
    }

    namespace LocalPlayer {
        inline constexpr uint64_t PlayerController    = 0x0030;
    }

    namespace GameStateBase {
        inline constexpr uint64_t PlayerArray         = 0x02D0;
        inline constexpr uint64_t PlayerArrayCount    = 0x02D8;
        inline constexpr uint64_t ReplicatedWorldTimeSecondsDouble = 0x02E8;
    }

    inline constexpr uint64_t RootComponent = 0x1C0;
    inline constexpr uint64_t RelativeLocation = 0x168;
    inline constexpr uint64_t RelativeRotation = 0x180;
    inline constexpr uint64_t ComponentVelocity = 0x1B0;
    inline constexpr uint64_t CharacterMesh = 0x338;
    inline constexpr uint64_t MoverMesh = 0x390;
    inline constexpr uint64_t MeshLastRenderTime = 0x358;
    inline constexpr uint64_t MeshLastSubmitTime = 0x354;
    inline constexpr uint64_t MeshLastRenderTimeOnScreen = 0x35C;
    inline constexpr uint64_t MeshComponentToWorld = 0x1D0;
    inline constexpr uint64_t MeshComponentToWorldAlt1 = 0x210;
    inline constexpr uint64_t MeshComponentToWorldAlt2 = 0x1E0;
    inline constexpr uint64_t SkinnedAsset = 0x5B0;
    inline constexpr uint64_t SkinnedAssetFallback = 0x5A8;
    inline constexpr uint64_t BoneArray = 0x620;
    inline constexpr uint64_t BoneArrayCache = 0x630;
    inline constexpr uint64_t BoneArrayReflected = 0xA18;
    inline constexpr uint64_t PoseArrayCompact = 0xA08;
    inline constexpr uint64_t RawRefBoneInfo = 0x2E0;
    inline constexpr uint64_t MeshBoneInfoSize = 0x0C;
    inline constexpr uint64_t FTransformSize = 0x60;
    inline constexpr uint64_t FTransformTranslation = 0x20;
    inline constexpr uint64_t DirectBoneArrayFallbackCount = 256;
    inline constexpr uint64_t AnimStanceFlags = 0xC44;

    inline constexpr uint64_t PawnPlayerState = 0x2D8;
    inline constexpr uint64_t ControllerPawn = 0x2F8;
    inline constexpr uint64_t ControllerPawnAlt = 0x308;
    inline constexpr uint64_t ControllerPlayerState = 0x2C0;
    inline constexpr uint64_t PlayerStatePawn = 0x330;
    inline constexpr uint64_t PlayerStateName = 0x350;
    inline constexpr uint64_t SavedNetworkAddress = 0x308;
    inline constexpr uint64_t ControllerPawnCurrent = 0x360;
    inline constexpr uint64_t ControllerControlRotation = 0x330;

    inline constexpr uint64_t PlayerStateFactionComponent = 0x548;
    inline constexpr uint64_t PlayerStateSquadComponent = 0x550;
    inline constexpr uint64_t IsDeveloper = 0x619;
    inline constexpr uint64_t IsAdmin = 0x61A;
    inline constexpr uint64_t PlayerStateLifeTag1 = 0x61C;
    inline constexpr uint64_t PlayerStateLifeTag2 = 0x628;
    inline constexpr uint64_t FactionComponentTag = 0x110;
    inline constexpr uint64_t FactionComponentTagAlt = 0x100;
    inline constexpr uint64_t FactionComponentFaction = 0x128;
    inline constexpr uint64_t FactionData = 0x2C8;
    inline constexpr uint64_t FactionTag = 0x30;
    inline constexpr uint64_t GenericTeamId = 0x108;
    inline constexpr uint64_t SquadId = 0x140;

    inline constexpr uint64_t VehicleSeatComponent = 0x748;
    inline constexpr uint64_t VehicleMesh = 0x750;
    inline constexpr uint64_t VehiclePlayerState = 0x848;
    inline constexpr uint64_t SeatOperatorsA = 0x108;
    inline constexpr uint64_t SeatOperatorsB = 0x118;
    inline constexpr uint64_t PawnVehicleOperator = 0x758;
    inline constexpr uint64_t OperatorSeatInfo = 0x198;
    inline constexpr uint64_t OperatorSeatInfoAlt = 0x188;
    inline constexpr uint64_t VehicleFactionWheeled = 0xBD8;
    inline constexpr uint64_t VehicleFactionAirplane = 0xBA8;
    inline constexpr uint64_t VehicleFactionRotary = 0xB88;
    inline constexpr uint64_t VehicleFactionTracked = 0xBE8;
    inline constexpr uint64_t VehicleFactionStationary = 0xAC8;

    inline constexpr uint64_t PawnVitalityComponent = 0x738;
    inline constexpr uint64_t VitalityHealth = 0x130;
    inline constexpr uint64_t VitalityMaxHealth = 0x134;
    inline constexpr uint64_t BleedoutState = 0x810;
    inline constexpr uint64_t PawnHealthFallback = 0x808;
    inline constexpr uint64_t PawnLifeTag = 0x7FC;
    inline constexpr uint64_t PawnWeaponBehavior = 0x770;
    inline constexpr uint64_t WeaponAdsMultiplier = 0x49C;
    inline constexpr uint64_t WeaponAdsBaseMultiplier = 0x4A0;
    inline constexpr uint64_t WeaponAimingHint = 0x25C;
    inline constexpr uint64_t CharacterAimingAlpha = 0x81C;
    inline constexpr uint64_t WeaponBehaviorStatsData = 0x440;
    inline constexpr uint64_t WeaponStatsStruct = 0x0;
    inline constexpr uint64_t WeaponStatsTypicalSpeed = 0xDC;
    inline constexpr uint64_t WeaponStatsTypicalSpeedAlt = 0xDD8;
    inline constexpr uint64_t PlayerInventory = 0x748;
    inline constexpr uint64_t InventoryHeldItem = 0x8C8;
    inline constexpr uint64_t InventoryActiveItem = 0xA78;

    inline constexpr uint64_t ProjectilePoolData = 0x108;
    inline constexpr uint64_t ProjectilePoolNum = 0x110;
    inline constexpr uint64_t ProjectilePoolBitsInline = 0x118;
    inline constexpr uint64_t ProjectilePoolBitsHeap = 0x128;
    inline constexpr uint64_t ProjectileInstanceSize = 0x320;
    inline constexpr uint64_t ProjectileLocation = 0x08;
    inline constexpr uint64_t ProjectileVelocity = 0x20;
    inline constexpr uint64_t ProjectileState = 0xC8;
    inline constexpr uint64_t ProjectileExternalMove = 0x128;
    inline constexpr uint64_t ProjectileFlightTime = 0x290;
    inline constexpr uint8_t  ProjectileStateFlying = 2;

    inline constexpr uint64_t CameraManager = 0x370;
    inline constexpr uint64_t CurrentCameraCache = 0x1560;
    inline constexpr uint64_t CameraCachePrivate = 0x1560;
    inline constexpr uint64_t LastFrameCameraCache = 0x1E40;
    inline constexpr uint64_t ViewTarget = 0x350;
    inline constexpr uint64_t CameraPov = 0x10;
    inline constexpr uint64_t CameraCacheLocation = 0x10;
    inline constexpr uint64_t CameraCacheRotation = 0x28;
    inline constexpr uint64_t CameraCacheFov = 0x40;

    inline constexpr uint64_t LevelActorsContainer = 0xE0;
    inline constexpr uint64_t LevelActorsArray = 0x28;
    inline constexpr uint64_t LevelActorsCount = 0x30;
    inline constexpr uint64_t LevelActorsDirect = 0xA0;

    inline uintptr_t UWorld = static_cast<uintptr_t>(GWorld);
}
