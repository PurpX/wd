#pragma once

inline bool showMenu = true;

inline bool boxEsp = true;
inline bool boneEsp = true;
inline bool snaplines = true;
inline bool showDistance = true;
inline bool showPlatform = true;
inline bool showPlayerId = true;
inline float maxEspDistance = 350.0f;
inline float maxBoneDistance = 230.0f;
inline float espBoxThickness = 1.5f;
inline bool espCornerBox = true;
inline bool espFilledBox = false;
inline bool espDistanceFade = true;
inline bool healthEsp = true;
inline float enemyEspColor[4] = { 0.95f, 0.28f, 0.32f, 1.0f };
inline float botEspColor[4] = { 1.0f, 0.82f, 0.25f, 1.0f };
inline float teammateEspColor[4] = { 0.25f, 0.78f, 0.95f, 1.0f };
inline bool showTeammates = true;
inline bool teamCheckAim = true;
inline bool useWeaponBulletSpeed = true;

inline bool enemyVehicleEsp = true;
inline bool teamVehicleEsp = true;
inline bool vehicleOccupantEsp = true;
inline float maxVehicleEspDistance = 450.0f;
inline float enemyVehicleEspColor[4] = { 1.0f, 0.35f, 0.15f, 1.0f };
inline float teamVehicleEspColor[4] = { 0.25f, 0.85f, 0.55f, 1.0f };
inline float vehicleEspThickness = 1.5f;

inline bool lootEsp = false;
inline float maxLootDistance = 200.0f;
inline float lootEspColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f };

inline bool radarEnabled = true;
inline bool radarDistance = true;
inline float radarOpacity = 150.0f;
inline int radarPositionX = 100;
inline int radarPositionY = 500;
inline int radarSize = 220;
inline int radarRange = 100;
inline bool radarGrid = true;
inline int radarGridDivisions = 8;
inline bool radarLineOfSight = false;
inline bool radarLosFill = true;
inline bool radarRotationMode = true;
inline bool radarShowPickups = false;

inline float menuBackgroundColor[4] = { 0.07f, 0.075f, 0.085f, 0.96f };
inline float accentColor[4] = { 0.78f, 0.62f, 0.28f, 1.0f };

inline bool vsyncEnabled = false;
inline bool showFpsCounter = true;
inline bool showDebugOverlay = false;
inline float fpsTextSize = 28.0f;

inline float fpsColor[4] = { 0.78f, 0.62f, 0.28f, 1.0f };

inline bool aimbotEnabled = true;
inline float aimbotFov = 140.0f;
inline float aimbotSmooth = 4.0f;
inline bool aimbotAdaptiveSmooth = true;
inline float aimbotDeadzone = 1.5f;
inline bool aimbotPrediction = true;
inline float aimbotBulletSpeed = 800.0f;
inline float bulletGravityCm = 980.0f;
inline bool drawHeadPrediction = false;
inline float predictionDotRadius = 3.0f;
inline bool showPredictionDetails = false;
inline float headPredictionColor[4] = { 1.0f, 0.8f, 0.0f, 1.0f };
inline int aimbotTargetPriority = 1;
inline bool aimbotVisibleOnly = false;
inline bool visCheckEsp = false;
inline bool aimbotLockTarget = true;
inline int aimbotTargetPlayerId = -1;
inline bool drawFovCircle = true;
inline float fovCircleColor[4] = { 1.0f, 1.0f, 1.0f, 0.39f };
inline bool showCrosshair = true;
inline float crosshairSize = 8.0f;
inline float crosshairThickness = 1.5f;
inline float crosshairGap = 3.0f;
inline float crosshairColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
inline bool showAimLine = true;
inline float aimLineColor[4] = { 1.0f, 0.35f, 0.35f, 0.85f };
inline float aimLineThickness = 1.5f;
inline int aimbotKey = VK_RBUTTON;
inline int aimbotBoneIndex = 9;

inline bool magicBullet = false;
