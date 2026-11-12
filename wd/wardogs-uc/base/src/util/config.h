#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "settings.h"

namespace config {
    inline std::string CONFIG_FOLDER = "C:\\brazzer cheats\\striped";
    inline std::string currentConfig = "config1.json";
    inline bool settingsChanged = false;
    inline float autoSaveTimer = 0.0f;
    const float AUTO_SAVE_INTERVAL = 2.0f;

    inline void SaveConfig();

    inline bool EnsureConfigFolder() {
        try {
            std::filesystem::create_directories(CONFIG_FOLDER);
            return true;
        } catch (...) {
            return false;
        }
    }

    inline std::vector<std::string> GetConfigList() {
        std::vector<std::string> configs;
        if (!EnsureConfigFolder()) return configs;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(CONFIG_FOLDER)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    configs.push_back(entry.path().filename().string());
                }
            }
        } catch (...) {}
        return configs;
    }

    inline void CreateDefaultConfig() {
        if (!EnsureConfigFolder()) return;
        std::vector<std::string> configs = GetConfigList();
        if (configs.empty()) {
            currentConfig = "config1.json";
            SaveConfig();
        } else {
            currentConfig = configs[0];
        }
    }

    inline std::string EscapeJsonString(const std::string& str) {
        std::ostringstream o;
        for (size_t i = 0; i < str.length(); ++i) {
            switch (str[i]) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= str[i] && str[i] <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)str[i];
                    } else {
                        o << str[i];
                    }
            }
        }
        return o.str();
    }

    inline void SaveConfig() {
        if (!EnsureConfigFolder()) return;
        std::string filePath = CONFIG_FOLDER + "\\" + currentConfig;
        std::ofstream file(filePath);
        if (!file.is_open()) return;

        file << "{\n";
        file << "  \"showMenu\": " << (showMenu ? "true" : "false") << ",\n";
        file << "  \"boxEsp\": " << (boxEsp ? "true" : "false") << ",\n";
        file << "  \"boneEsp\": " << (boneEsp ? "true" : "false") << ",\n";
        file << "  \"snaplines\": " << (snaplines ? "true" : "false") << ",\n";
        file << "  \"showDistance\": " << (showDistance ? "true" : "false") << ",\n";
        file << "  \"showPlatform\": " << (showPlatform ? "true" : "false") << ",\n";
        file << "  \"showPlayerId\": " << (showPlayerId ? "true" : "false") << ",\n";
        file << "  \"maxEspDistance\": " << maxEspDistance << ",\n";
        file << "  \"maxBoneDistance\": " << maxBoneDistance << ",\n";
        file << "  \"espBoxThickness\": " << espBoxThickness << ",\n";
        file << "  \"espCornerBox\": " << (espCornerBox ? "true" : "false") << ",\n";
        file << "  \"espFilledBox\": " << (espFilledBox ? "true" : "false") << ",\n";
        file << "  \"espDistanceFade\": " << (espDistanceFade ? "true" : "false") << ",\n";
        file << "  \"healthEsp\": " << (healthEsp ? "true" : "false") << ",\n";
        file << "  \"useWeaponBulletSpeed\": " << (useWeaponBulletSpeed ? "true" : "false") << ",\n";
        file << "  \"enemyEspColor\": [" << enemyEspColor[0] << ", " << enemyEspColor[1] << ", " << enemyEspColor[2] << ", " << enemyEspColor[3] << "],\n";
        file << "  \"teammateEspColor\": [" << teammateEspColor[0] << ", " << teammateEspColor[1] << ", " << teammateEspColor[2] << ", " << teammateEspColor[3] << "],\n";
        file << "  \"botEspColor\": [" << botEspColor[0] << ", " << botEspColor[1] << ", " << botEspColor[2] << ", " << botEspColor[3] << "],\n";
        file << "  \"showTeammates\": " << (showTeammates ? "true" : "false") << ",\n";
        file << "  \"teamCheckAim\": " << (teamCheckAim ? "true" : "false") << ",\n";
        file << "  \"enemyVehicleEsp\": " << (enemyVehicleEsp ? "true" : "false") << ",\n";
        file << "  \"teamVehicleEsp\": " << (teamVehicleEsp ? "true" : "false") << ",\n";
        file << "  \"vehicleOccupantEsp\": " << (vehicleOccupantEsp ? "true" : "false") << ",\n";
        file << "  \"maxVehicleEspDistance\": " << maxVehicleEspDistance << ",\n";
        file << "  \"enemyVehicleEspColor\": [" << enemyVehicleEspColor[0] << ", " << enemyVehicleEspColor[1] << ", " << enemyVehicleEspColor[2] << ", " << enemyVehicleEspColor[3] << "],\n";
        file << "  \"teamVehicleEspColor\": [" << teamVehicleEspColor[0] << ", " << teamVehicleEspColor[1] << ", " << teamVehicleEspColor[2] << ", " << teamVehicleEspColor[3] << "],\n";
        file << "  \"vehicleEspThickness\": " << vehicleEspThickness << ",\n";
        file << "  \"lootEsp\": " << (lootEsp ? "true" : "false") << ",\n";
        file << "  \"maxLootDistance\": " << maxLootDistance << ",\n";
        file << "  \"lootEspColor\": [" << lootEspColor[0] << ", " << lootEspColor[1] << ", " << lootEspColor[2] << ", " << lootEspColor[3] << "],\n";
        file << "  \"radarEnabled\": " << (radarEnabled ? "true" : "false") << ",\n";
        file << "  \"radarDistance\": " << (radarDistance ? "true" : "false") << ",\n";
        file << "  \"radarOpacity\": " << radarOpacity << ",\n";
        file << "  \"radarPositionX\": " << radarPositionX << ",\n";
        file << "  \"radarPositionY\": " << radarPositionY << ",\n";
        file << "  \"radarSize\": " << radarSize << ",\n";
        file << "  \"radarRange\": " << radarRange << ",\n";
        file << "  \"radarGrid\": " << (radarGrid ? "true" : "false") << ",\n";
        file << "  \"radarGridDivisions\": " << radarGridDivisions << ",\n";
        file << "  \"radarLineOfSight\": " << (radarLineOfSight ? "true" : "false") << ",\n";
        file << "  \"radarLosFill\": " << (radarLosFill ? "true" : "false") << ",\n";
        file << "  \"radarRotationMode\": " << (radarRotationMode ? "true" : "false") << ",\n";
        file << "  \"radarShowPickups\": " << (radarShowPickups ? "true" : "false") << ",\n";
        file << "  \"vsyncEnabled\": " << (vsyncEnabled ? "true" : "false") << ",\n";
        file << "  \"showFpsCounter\": " << (showFpsCounter ? "true" : "false") << ",\n";
        file << "  \"showDebugOverlay\": " << (showDebugOverlay ? "true" : "false") << ",\n";
        file << "  \"fpsTextSize\": " << fpsTextSize << ",\n";
        file << "  \"fpsColor\": [" << fpsColor[0] << ", " << fpsColor[1] << ", " << fpsColor[2] << ", " << fpsColor[3] << "],\n";
        file << "  \"menuBackgroundColor\": [" << menuBackgroundColor[0] << ", " << menuBackgroundColor[1] << ", " << menuBackgroundColor[2] << ", " << menuBackgroundColor[3] << "],\n";
        file << "  \"accentColor\": [" << accentColor[0] << ", " << accentColor[1] << ", " << accentColor[2] << ", " << accentColor[3] << "],\n";
        file << "  \"aimbotEnabled\": " << (aimbotEnabled ? "true" : "false") << ",\n";
        file << "  \"aimbotFov\": " << aimbotFov << ",\n";
        file << "  \"aimbotSmooth\": " << aimbotSmooth << ",\n";
        file << "  \"aimbotAdaptiveSmooth\": " << (aimbotAdaptiveSmooth ? "true" : "false") << ",\n";
        file << "  \"aimbotDeadzone\": " << aimbotDeadzone << ",\n";
        file << "  \"aimbotPrediction\": " << (aimbotPrediction ? "true" : "false") << ",\n";
        file << "  \"aimbotBulletSpeed\": " << aimbotBulletSpeed << ",\n";
        file << "  \"bulletGravityCm\": " << bulletGravityCm << ",\n";
        file << "  \"drawHeadPrediction\": " << (drawHeadPrediction ? "true" : "false") << ",\n";
        file << "  \"predictionDotRadius\": " << predictionDotRadius << ",\n";
        file << "  \"showPredictionDetails\": " << (showPredictionDetails ? "true" : "false") << ",\n";
        file << "  \"headPredictionColor\": [" << headPredictionColor[0] << ", " << headPredictionColor[1] << ", " << headPredictionColor[2] << ", " << headPredictionColor[3] << "],\n";
        file << "  \"aimbotTargetPriority\": " << aimbotTargetPriority << ",\n";
        file << "  \"aimbotVisibleOnly\": " << (aimbotVisibleOnly ? "true" : "false") << ",\n";
        file << "  \"visCheckEsp\": " << (visCheckEsp ? "true" : "false") << ",\n";
        file << "  \"aimbotLockTarget\": " << (aimbotLockTarget ? "true" : "false") << ",\n";
        file << "  \"aimbotTargetPlayerId\": " << aimbotTargetPlayerId << ",\n";
        file << "  \"drawFovCircle\": " << (drawFovCircle ? "true" : "false") << ",\n";
        file << "  \"fovCircleColor\": [" << fovCircleColor[0] << ", " << fovCircleColor[1] << ", " << fovCircleColor[2] << ", " << fovCircleColor[3] << "],\n";
        file << "  \"showCrosshair\": " << (showCrosshair ? "true" : "false") << ",\n";
        file << "  \"crosshairSize\": " << crosshairSize << ",\n";
        file << "  \"crosshairThickness\": " << crosshairThickness << ",\n";
        file << "  \"crosshairGap\": " << crosshairGap << ",\n";
        file << "  \"crosshairColor\": [" << crosshairColor[0] << ", " << crosshairColor[1] << ", " << crosshairColor[2] << ", " << crosshairColor[3] << "],\n";
        file << "  \"showAimLine\": " << (showAimLine ? "true" : "false") << ",\n";
        file << "  \"aimLineColor\": [" << aimLineColor[0] << ", " << aimLineColor[1] << ", " << aimLineColor[2] << ", " << aimLineColor[3] << "],\n";
        file << "  \"aimLineThickness\": " << aimLineThickness << ",\n";
        file << "  \"aimbotKey\": " << aimbotKey << ",\n";
        file << "  \"aimbotBoneIndex\": " << aimbotBoneIndex << ",\n";
        file << "  \"magicBullet\": " << (magicBullet ? "true" : "false") << ",\n";
        file << "  \"configFolder\": \"" << EscapeJsonString(CONFIG_FOLDER) << "\"\n";
        file << "}\n";
        file.close();
        settingsChanged = false;
    }

    inline bool ParseBool(const std::string& str) {
        return str == "true" || str == "1";
    }

    inline float ParseFloat(const std::string& str) {
        return (float)std::atof(str.c_str());
    }

    inline int ParseInt(const std::string& str) {
        return std::atoi(str.c_str());
    }

    inline std::string ExtractValue(const std::string& line) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) return "";
        std::string value = line.substr(colonPos + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value = value.substr(1);
        while (!value.empty() && (value.back() == ',' || value.back() == ' ' || value.back() == '\t' || value.back() == '\n' || value.back() == '\r' || value.back() == '"')) value.pop_back();
        if (!value.empty() && value.front() == '"') value = value.substr(1);
        return value;
    }

    inline void ParseArray(const std::string& value, float* arr, int count) {
        std::string cleaned = value;
        if (cleaned.front() == '[') cleaned = cleaned.substr(1);
        if (cleaned.back() == ']') cleaned.pop_back();
        std::istringstream iss(cleaned);
        std::string token;
        int idx = 0;
        while (std::getline(iss, token, ',') && idx < count) {
            while (!token.empty() && (token[0] == ' ' || token[0] == '\t')) token = token.substr(1);
            arr[idx++] = ParseFloat(token);
        }
    }

    inline void LoadConfig(const std::string& configName) {
        if (!EnsureConfigFolder()) return;
        std::string filePath = CONFIG_FOLDER + "\\" + configName;
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        currentConfig = configName;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty() || line.find("//") == 0) continue;

            if (line.find("\"showMenu\"") != std::string::npos) showMenu = ParseBool(ExtractValue(line));
            else if (line.find("\"boxEsp\"") != std::string::npos) boxEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"boneEsp\"") != std::string::npos) boneEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"snaplines\"") != std::string::npos) snaplines = ParseBool(ExtractValue(line));
            else if (line.find("\"showDistance\"") != std::string::npos) showDistance = ParseBool(ExtractValue(line));
            else if (line.find("\"showPlatform\"") != std::string::npos) showPlatform = ParseBool(ExtractValue(line));
            else if (line.find("\"showPlayerId\"") != std::string::npos) showPlayerId = ParseBool(ExtractValue(line));
            else if (line.find("\"maxEspDistance\"") != std::string::npos) maxEspDistance = ParseFloat(ExtractValue(line));
            else if (line.find("\"maxBoneDistance\"") != std::string::npos) maxBoneDistance = ParseFloat(ExtractValue(line));
            else if (line.find("\"espBoxThickness\"") != std::string::npos) espBoxThickness = ParseFloat(ExtractValue(line));
            else if (line.find("\"espCornerBox\"") != std::string::npos) espCornerBox = ParseBool(ExtractValue(line));
            else if (line.find("\"espFilledBox\"") != std::string::npos) espFilledBox = ParseBool(ExtractValue(line));
            else if (line.find("\"espDistanceFade\"") != std::string::npos) espDistanceFade = ParseBool(ExtractValue(line));
            else if (line.find("\"healthEsp\"") != std::string::npos) healthEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"useWeaponBulletSpeed\"") != std::string::npos) useWeaponBulletSpeed = ParseBool(ExtractValue(line));
            else if (line.find("\"enemyEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), enemyEspColor, 4);
            else if (line.find("\"teammateEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), teammateEspColor, 4);
            else if (line.find("\"botEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), botEspColor, 4);
            else if (line.find("\"showTeammates\"") != std::string::npos) showTeammates = ParseBool(ExtractValue(line));
            else if (line.find("\"teamCheckAim\"") != std::string::npos) teamCheckAim = ParseBool(ExtractValue(line));
            else if (line.find("\"enemyVehicleEsp\"") != std::string::npos) enemyVehicleEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"teamVehicleEsp\"") != std::string::npos) teamVehicleEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"vehicleOccupantEsp\"") != std::string::npos) vehicleOccupantEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"maxVehicleEspDistance\"") != std::string::npos) maxVehicleEspDistance = ParseFloat(ExtractValue(line));
            else if (line.find("\"enemyVehicleEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), enemyVehicleEspColor, 4);
            else if (line.find("\"teamVehicleEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), teamVehicleEspColor, 4);
            else if (line.find("\"vehicleEspThickness\"") != std::string::npos) vehicleEspThickness = ParseFloat(ExtractValue(line));
            else if (line.find("\"lootEsp\"") != std::string::npos) lootEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"maxLootDistance\"") != std::string::npos) maxLootDistance = ParseFloat(ExtractValue(line));
            else if (line.find("\"lootEspColor\"") != std::string::npos) ParseArray(ExtractValue(line), lootEspColor, 4);
            else if (line.find("\"radarEnabled\"") != std::string::npos) radarEnabled = ParseBool(ExtractValue(line));
            else if (line.find("\"radarDistance\"") != std::string::npos) radarDistance = ParseBool(ExtractValue(line));
            else if (line.find("\"radarOpacity\"") != std::string::npos) radarOpacity = ParseFloat(ExtractValue(line));
            else if (line.find("\"radarPositionX\"") != std::string::npos) radarPositionX = ParseInt(ExtractValue(line));
            else if (line.find("\"radarPositionY\"") != std::string::npos) radarPositionY = ParseInt(ExtractValue(line));
            else if (line.find("\"radarSize\"") != std::string::npos) radarSize = ParseInt(ExtractValue(line));
            else if (line.find("\"radarRange\"") != std::string::npos) radarRange = ParseInt(ExtractValue(line));
            else if (line.find("\"radarGrid\"") != std::string::npos) radarGrid = ParseBool(ExtractValue(line));
            else if (line.find("\"radarGridDivisions\"") != std::string::npos) radarGridDivisions = ParseInt(ExtractValue(line));
            else if (line.find("\"radarLineOfSight\"") != std::string::npos) radarLineOfSight = ParseBool(ExtractValue(line));
            else if (line.find("\"radarLosFill\"") != std::string::npos) radarLosFill = ParseBool(ExtractValue(line));
            else if (line.find("\"radarRotationMode\"") != std::string::npos) radarRotationMode = ParseBool(ExtractValue(line));
            else if (line.find("\"radarShowPickups\"") != std::string::npos) radarShowPickups = ParseBool(ExtractValue(line));
            else if (line.find("\"vsyncEnabled\"") != std::string::npos) vsyncEnabled = ParseBool(ExtractValue(line));
            else if (line.find("\"showFpsCounter\"") != std::string::npos) showFpsCounter = ParseBool(ExtractValue(line));
            else if (line.find("\"showDebugOverlay\"") != std::string::npos) showDebugOverlay = ParseBool(ExtractValue(line));
            else if (line.find("\"fpsTextSize\"") != std::string::npos) fpsTextSize = ParseFloat(ExtractValue(line));
            else if (line.find("\"fpsColor\"") != std::string::npos) ParseArray(ExtractValue(line), fpsColor, 4);
            else if (line.find("\"menuBackgroundColor\"") != std::string::npos) ParseArray(ExtractValue(line), menuBackgroundColor, 4);
            else if (line.find("\"accentColor\"") != std::string::npos) ParseArray(ExtractValue(line), accentColor, 4);
            else if (line.find("\"aimbotEnabled\"") != std::string::npos) aimbotEnabled = ParseBool(ExtractValue(line));
            else if (line.find("\"aimbotFov\"") != std::string::npos) aimbotFov = ParseFloat(ExtractValue(line));
            else if (line.find("\"aimbotSmooth\"") != std::string::npos) aimbotSmooth = ParseFloat(ExtractValue(line));
            else if (line.find("\"aimbotAdaptiveSmooth\"") != std::string::npos) aimbotAdaptiveSmooth = ParseBool(ExtractValue(line));
            else if (line.find("\"aimbotDeadzone\"") != std::string::npos) aimbotDeadzone = ParseFloat(ExtractValue(line));
            else if (line.find("\"aimbotPrediction\"") != std::string::npos) aimbotPrediction = ParseBool(ExtractValue(line));
            else if (line.find("\"aimbotBulletSpeed\"") != std::string::npos) aimbotBulletSpeed = ParseFloat(ExtractValue(line));
            else if (line.find("\"bulletGravityCm\"") != std::string::npos) bulletGravityCm = ParseFloat(ExtractValue(line));
            else if (line.find("\"drawHeadPrediction\"") != std::string::npos) drawHeadPrediction = ParseBool(ExtractValue(line));
            else if (line.find("\"predictionDotRadius\"") != std::string::npos) predictionDotRadius = ParseFloat(ExtractValue(line));
            else if (line.find("\"showPredictionDetails\"") != std::string::npos) showPredictionDetails = ParseBool(ExtractValue(line));
            else if (line.find("\"headPredictionColor\"") != std::string::npos) ParseArray(ExtractValue(line), headPredictionColor, 4);
            else if (line.find("\"aimbotTargetPriority\"") != std::string::npos) {
                aimbotTargetPriority = ParseInt(ExtractValue(line));
                if (aimbotTargetPriority < 0) aimbotTargetPriority = 0;
                if (aimbotTargetPriority > 1) aimbotTargetPriority = 1;
            }
            else if (line.find("\"aimbotVisibleOnly\"") != std::string::npos) aimbotVisibleOnly = ParseBool(ExtractValue(line));
            else if (line.find("\"visCheckEsp\"") != std::string::npos) visCheckEsp = ParseBool(ExtractValue(line));
            else if (line.find("\"aimbotLockTarget\"") != std::string::npos) aimbotLockTarget = ParseBool(ExtractValue(line));
            else if (line.find("\"aimbotTargetPlayerId\"") != std::string::npos) aimbotTargetPlayerId = ParseInt(ExtractValue(line));
            else if (line.find("\"drawFovCircle\"") != std::string::npos) drawFovCircle = ParseBool(ExtractValue(line));
            else if (line.find("\"fovCircleColor\"") != std::string::npos) ParseArray(ExtractValue(line), fovCircleColor, 4);
            else if (line.find("\"showCrosshair\"") != std::string::npos) showCrosshair = ParseBool(ExtractValue(line));
            else if (line.find("\"crosshairSize\"") != std::string::npos) crosshairSize = ParseFloat(ExtractValue(line));
            else if (line.find("\"crosshairThickness\"") != std::string::npos) crosshairThickness = ParseFloat(ExtractValue(line));
            else if (line.find("\"crosshairGap\"") != std::string::npos) crosshairGap = ParseFloat(ExtractValue(line));
            else if (line.find("\"crosshairColor\"") != std::string::npos) ParseArray(ExtractValue(line), crosshairColor, 4);
            else if (line.find("\"showAimLine\"") != std::string::npos) showAimLine = ParseBool(ExtractValue(line));
            else if (line.find("\"aimLineColor\"") != std::string::npos) ParseArray(ExtractValue(line), aimLineColor, 4);
            else if (line.find("\"aimLineThickness\"") != std::string::npos) aimLineThickness = ParseFloat(ExtractValue(line));
            else if (line.find("\"aimbotKey\"") != std::string::npos) aimbotKey = ParseInt(ExtractValue(line));
            else if (line.find("\"aimbotBoneIndex\"") != std::string::npos) aimbotBoneIndex = ParseInt(ExtractValue(line));
            else if (line.find("\"magicBullet\"") != std::string::npos) magicBullet = ParseBool(ExtractValue(line));
            else if (line.find("\"configFolder\"") != std::string::npos) {
                std::string path = ExtractValue(line);
                if (!path.empty()) {
                    for (size_t i = 0; (i = path.find("\\\\", i)) != std::string::npos; ) {
                        path.replace(i, 2, "\\");
                        i += 1;
                    }
                    CONFIG_FOLDER = path;
                }
            }
        }
        file.close();
        settingsChanged = false;
    }

    inline void CreateNewConfig(const std::string& configName) {
        if (!EnsureConfigFolder()) return;
        std::string filePath = CONFIG_FOLDER + "\\" + configName;
        if (std::filesystem::exists(filePath)) return;
        currentConfig = configName;
        SaveConfig();
    }

    inline void MarkSettingsChanged() {
        settingsChanged = true;
        autoSaveTimer = AUTO_SAVE_INTERVAL;
    }

    inline void UpdateAutoSave(float deltaTime) {
        if (settingsChanged && autoSaveTimer > 0.0f) {
            autoSaveTimer -= deltaTime;
            if (autoSaveTimer <= 0.0f) SaveConfig();
        }
    }
}
