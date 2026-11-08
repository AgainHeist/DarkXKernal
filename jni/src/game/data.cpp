#define IMGUI_DEFINE_MATH_OPERATORS // <-- NDK Error Fix
#include "data.hpp"
#include "utils.hpp"
#include "math.hpp"
#include "../other/memory.hpp"
#include "../Unity/Quaternion.hpp"
#include "../ui/cfg.hpp"
#include "offsets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>
#include <map>
#include <sys/prctl.h>
#include "Android_touch/Touch.hpp"

// ==================== CRASH PREVENTION SYSTEM ====================
inline bool IsValidPointer(uintptr_t ptr) {
    return (ptr >= 0x10000 && ptr < 0x7FFFFFFFFFFF);
}
// =================================================================

bool g_MatchRunning = false;

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

bool isOutsideScreen(const Vec2& pos, const Vec2& screen) {
    if (pos.y < 0) return true;
    if (pos.x > screen.x) return true;
    if (pos.y > screen.y) return true;
    return pos.x < 0;
}

Vec2 pushToScreenBorder(const Vec2& Pos, const Vec2& screen, float offset) {
    float x = Pos.x, y = Pos.y;
    if (Pos.y < 0) y = -offset;
    if (Pos.x > screen.x) x = screen.x + offset;
    if (Pos.y > screen.y) y = screen.y + offset;
    if (Pos.x < 0) x = -offset;
    return Vec2(x, y);
}

void Draw360Alert(ImDrawList* draw, Vector3 headPos, float distance, float sw, float sh, bool isKnocked) {
    if (!cfg::esp::alert_360) return;
    if (headPos.X < 0 || headPos.Y < 0) return;

    Vec2 screen(sw, sh);
    Vec2 headPos2D(headPos.X, headPos.Y);

    if (isOutsideScreen(headPos2D, screen)) {
        Vec2 hintDotRenderPos = pushToScreenBorder(headPos2D, screen, -50);
        float radius = 35.0f;
        ImU32 circleColor = isKnocked ? IM_COL32(255, 0, 0, 200) : IM_COL32(255, 0, 0, 200); 

        for (int i = 1; i <= 3; i++) {
            float alpha = 0.15f / i;
            draw->AddCircleFilled(ImVec2(hintDotRenderPos.x, hintDotRenderPos.y), radius + i, IM_COL32(255, 0, 0, (int)(alpha * 255)));
        }
        draw->AddCircleFilled(ImVec2(hintDotRenderPos.x, hintDotRenderPos.y), radius, circleColor);
        draw->AddCircle(ImVec2(hintDotRenderPos.x, hintDotRenderPos.y), radius, IM_COL32(255, 255, 255, 255), 0, 2.5f);
        
        std::string strDistance = "[" + std::to_string((int)distance) + "M]";
        ImVec2 textSize = ImGui::CalcTextSize(strDistance.c_str());
        draw->AddText(NULL, 18.0f, ImVec2(hintDotRenderPos.x - textSize.x / 2, hintDotRenderPos.y - textSize.y / 2), IM_COL32(255, 255, 255, 255), strDistance.c_str());
        
        float angle = atan2(headPos2D.y - sh/2, headPos2D.x - sw/2);
        float arrowX = hintDotRenderPos.x + cos(angle) * (radius + 10);
        float arrowY = hintDotRenderPos.y + sin(angle) * (radius + 10);
        draw->AddTriangleFilled(ImVec2(arrowX, arrowY), ImVec2(arrowX - 8, arrowY - 5), ImVec2(arrowX - 8, arrowY + 5), IM_COL32(255, 255, 255, 255));
    }
}

namespace data {

    // ===== 0-DETECTION CACHING SYSTEM =====
    struct EntityCache {
        uintptr_t address;
        char name[64];
        bool isBot;
        std::chrono::steady_clock::time_point lastSeen;
    };
    static std::vector<EntityCache> g_activeEntities;
    static std::chrono::steady_clock::time_point g_lastListRefresh;
    static std::mutex g_cacheMutex;

    // ===== CACHE SYSTEM FOR BLINKING FIX =====
    struct CachedPlayerData {
        Vector3 Head;
        Vector3 Toe;
        float BoxX;
        float BoxY;
        float BoxWidth;
        float BoxHeight;
        float Distance;
        int Health;
        bool IsKnocked;
        bool IsBot;
        char PlayerName[64];
        Vector3 sHead, sChest, sHip, sLeftShoulder, sRightShoulder;
        Vector3 sLeftHand, sRightHand, sLeftAnkle, sRightAnkle;
        bool isValid;
    };

    static CachedPlayerData g_cachedPlayers[100];
    static int g_cachedPlayerCount = 0;
    static std::chrono::steady_clock::time_point g_lastUpdateTime;
    static bool g_hasValidCache = false;
    static const int CACHE_EXPIRY_MS = 150; // Tuned for "Makhan" smoothness (low ghosting, high persistence)

    bool IsCacheExpired() {
        if (!g_hasValidCache) return true;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastUpdateTime).count();
        return elapsed > CACHE_EXPIRY_MS;
    }

    void UpdatePlayerCache(const std::vector<PlayerData>& tempPlayers) {
        g_cachedPlayerCount = 0;
        int maxPlayers = std::min((int)tempPlayers.size(), 100);

        for (int i = 0; i < maxPlayers; i++) {
            const auto& p = tempPlayers[i];
            g_cachedPlayers[i].Head = p.Head;
            g_cachedPlayers[i].Toe = p.Toe;
            g_cachedPlayers[i].BoxX = p.BoxX;
            g_cachedPlayers[i].BoxY = p.BoxY;
            g_cachedPlayers[i].BoxWidth = p.BoxWidth;
            g_cachedPlayers[i].BoxHeight = p.BoxHeight;
            g_cachedPlayers[i].Distance = p.Distance;
            g_cachedPlayers[i].Health = p.Health;
            g_cachedPlayers[i].IsKnocked = p.IsKnocked;
            g_cachedPlayers[i].IsBot = p.IsBot;
            strncpy(g_cachedPlayers[i].PlayerName, p.PlayerName, 63);
            g_cachedPlayers[i].PlayerName[63] = '\0';
            g_cachedPlayers[i].sHead = p.sHead;
            g_cachedPlayers[i].sChest = p.sChest;
            g_cachedPlayers[i].sHip = p.sHip;
            g_cachedPlayers[i].sLeftShoulder = p.sLeftShoulder;
            g_cachedPlayers[i].sRightShoulder = p.sRightShoulder;
            g_cachedPlayers[i].sLeftHand = p.sLeftHand;
            g_cachedPlayers[i].sRightHand = p.sRightHand;
            g_cachedPlayers[i].sLeftAnkle = p.sLeftAnkle;
            g_cachedPlayers[i].sRightAnkle = p.sRightAnkle;
            g_cachedPlayers[i].isValid = true;
            g_cachedPlayerCount++;
        }

        g_hasValidCache = true;
        g_lastUpdateTime = std::chrono::steady_clock::now();
    }

    void ClearPlayerCache() {
        for (int i = 0; i < 100; i++) {
            g_cachedPlayers[i].isValid = false;
        }
        g_cachedPlayerCount = 0;
        g_hasValidCache = false;
    }

    std::vector<PlayerData> players;
    std::mutex players_mutex;

    std::atomic<uint64_t> g_LocalPlayer{0};
    std::atomic<uint64_t> g_CurrentTargetObj{0};
    std::atomic<bool> g_HasData{false};
    std::atomic<bool> track{true};
    D3DMatrix SharedViewMatrix{};

    int realPlayerCount = 0;
    int botCount = 0;
    uintptr_t currentBestTarget = 0;
    Vector3 currentBestPos;

    bool g_InGame = false;

    struct Matrix { Vector4 Position; Quaternion Rotation; Vector4 Scale; };


    static auto GetPosition(uintptr_t Transform) {
        auto pos = Vector3::Zero();
        if (!IsValidPointer(Transform)) return pos;

        auto transformObjValue = rpm<uintptr_t>(Transform + Offsets::Transform::TransformToObj);
        if (!IsValidPointer(transformObjValue)) return pos;

        auto indexValue = rpm<uintptr_t>(transformObjValue + Offsets::Transform::ObjToIndex);
        auto matrixValue = rpm<uintptr_t>(transformObjValue + Offsets::Transform::ObjToMatrix);
        if (!IsValidPointer(matrixValue)) return pos;

        auto matrixListValue = rpm<uintptr_t>(matrixValue + Offsets::Transform::MatrixList);
        auto matrixIndicesValue = rpm<uintptr_t>(matrixValue + Offsets::Transform::MatrixIndices);
        if (!IsValidPointer(matrixListValue) || !IsValidPointer(matrixIndicesValue)) return pos;

        auto resultValue = rpm<Vector3>(matrixListValue + (indexValue * Offsets::Transform::MatrixSize));
        auto maxTries = 30; // 🔥 Optimized from 50 to 30 (Plenty for Unity, much faster)
        auto tries = 0;
        auto transformIndexValue = rpm<int>(matrixIndicesValue + (indexValue * Offsets::Transform::IndexSize));

        while (transformIndexValue >= 0 && tries < maxTries) {
            tries++;
            uintptr_t matrixItemPtr = matrixListValue + (transformIndexValue * Offsets::Transform::MatrixSize);
            if (!IsValidPointer(matrixItemPtr)) break;

            auto tMatrixValue = rpm<Matrix>(matrixItemPtr);
            auto rotX = tMatrixValue.Rotation.X; auto rotY = tMatrixValue.Rotation.Y;
            auto rotZ = tMatrixValue.Rotation.Z; auto rotW = tMatrixValue.Rotation.W;
            auto scaleX = resultValue.X * tMatrixValue.Scale.X;
            auto scaleY = resultValue.Y * tMatrixValue.Scale.Y;
            auto scaleZ = resultValue.Z * tMatrixValue.Scale.Z;

            resultValue.X = tMatrixValue.Position.X + scaleX + (scaleX * ((rotY * rotY * -2.0) - (rotZ * rotZ * 2.0))) + (scaleY * ((rotW * rotZ * -2.0) - (rotY * rotX * -2.0))) + (scaleZ * ((rotZ * rotX * 2.0) - (rotW * rotY * -2.0)));
            resultValue.Y = tMatrixValue.Position.Y + scaleY + (scaleX * ((rotX * rotY * 2.0) - (rotW * rotZ * -2.0))) + (scaleY * ((rotZ * rotZ * -2.0) - (rotX * rotX * 2.0))) + (scaleZ * ((rotW * rotX * -2.0) - (rotZ * rotY * -2.0)));
            resultValue.Z = tMatrixValue.Position.Z + scaleZ + (scaleX * ((rotW * rotY * -2.0) - (rotX * rotZ * -2.0))) + (scaleY * ((rotY * rotZ * 2.0) - (rotW * rotX * -2.0))) + (scaleZ * ((rotX * rotX * -2.0) - (rotY * rotY * 2.0)));

            uintptr_t indexItemPtr = matrixIndicesValue + (transformIndexValue * Offsets::Transform::IndexSize);
            if (!IsValidPointer(indexItemPtr)) break;
            transformIndexValue = rpm<int>(indexItemPtr);
        }
        if (tries < maxTries) pos = resultValue;
        return pos;
    }

    static auto GetNodePosition(uintptr_t nodeTransform) {
        if (!IsValidPointer(nodeTransform)) return Vector3::Zero();
        auto transformValue = rpm<uintptr_t>(nodeTransform + Offsets::Transform::TransformToObj);
        return IsValidPointer(transformValue) ? GetPosition(transformValue) : Vector3::Zero();
    }

    Vector3 GetBonePosition(uintptr_t player, uintptr_t offset) {
        if (!IsValidPointer(player)) return Vector3::Zero();
        uintptr_t bone = rpm<uintptr_t>(player + offset);
        return IsValidPointer(bone) ? GetNodePosition(bone) : Vector3::Zero();
    }

    Vector3 GetTargetPosition(uintptr_t enemy) {
        // 🔥 MAKHAAN: Reduced RPMs by assuming head only
        if (!IsValidPointer(enemy)) return Vector3::Zero();
        uintptr_t bone = rpm<uintptr_t>(enemy + Offsets::Bones::Head);
        if (!IsValidPointer(bone)) return Vector3::Zero();
        uintptr_t trans = rpm<uintptr_t>(bone + Offsets::Transform::TransformToObj);
        return IsValidPointer(trans) ? GetPosition(trans) : Vector3::Zero();
    }

    uintptr_t GetPlayerHeadTF(uintptr_t player) { return IsValidPointer(player) ? rpm<uintptr_t>(player + Offsets::Bones::Head) : 0; }
    uintptr_t GetPlayerPeTF(uintptr_t player) { return IsValidPointer(player) ? rpm<uintptr_t>(player + Offsets::Bones::Feet) : 0; }
    uintptr_t GetPlayerMainCamera(uintptr_t player) { return IsValidPointer(player) ? rpm<uintptr_t>(player + Offsets::Player::MainCamera) : 0; }

    Quaternion GetRotationToTheLocation(Vector3 Target, float Height, Vector3 MyEnemy) {
        return Quaternion::LookRotation((Target + Vector3(0, Height, 0)) - MyEnemy, Vector3(0, 1, 0));
    }

    static auto HookW2S(D3DMatrix viewMatrix, Vector3 ScreenPos) {
        auto result = Vector3(-1, -1, -1);
        auto v9 = (ScreenPos.X * viewMatrix._11) + (ScreenPos.Y * viewMatrix._21) + (ScreenPos.Z * viewMatrix._31) + viewMatrix._41;
        auto v10 = (ScreenPos.X * viewMatrix._12) + (ScreenPos.Y * viewMatrix._22) + (ScreenPos.Z * viewMatrix._32) + viewMatrix._42;
        auto v12 = (ScreenPos.X * viewMatrix._14) + (ScreenPos.Y * viewMatrix._24) + (ScreenPos.Z * viewMatrix._34) + viewMatrix._44;

        if (v12 >= 0.001f) {
            auto v13 = (float)g_sw / 2.0f;
            auto v14 = (float)g_sh / 2.0f;

            result.X = v13 + (v13 * v9) / v12;
            result.Y = v14 - (v14 * v10) / v12;
            result.Z = v12; 
        }
        return result;
    }

    // ========================================================================
    // 🎯 SILENT AIM WITH PREDICTION & JITTER
    // ========================================================================
    struct TargetHistory {
        Vector3 lastPos;
        std::chrono::steady_clock::time_point lastTime;
        Vector3 velocity;
    };
    static std::map<uintptr_t, TargetHistory> g_targetHistory;

    void SilentAimThread() {
        // 🔥 Hide thread from scanners
        prctl(PR_SET_NAME, oxorany("rcu_preempt"), 0, 0, 0);

        static uint64_t lockedEnemy = 0;
        static bool isHeadshot = true;
        static auto lastSwitchTime = std::chrono::steady_clock::now();

        while (track.load()) {
            if (!cfg::other::activate_menu || !g_InGame || !cfg::aim::silent || !g_LocalPlayer.load() || !currentBestTarget) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            uintptr_t lp = g_LocalPlayer.load();
            if (!lp) continue;

            uint64_t aimingInfo = rpm<uint64_t>(lp + Offsets::Player::AimingInfo);
            if (IsValidPointer(aimingInfo)) {
                uint64_t currentTarget = currentBestTarget;

                int hitRate = (int)cfg::aim::silent_headshot_rate;
                auto now = std::chrono::steady_clock::now();
                if (currentTarget != lockedEnemy || std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSwitchTime).count() > 200) {
                    isHeadshot = (rand() % 100) < hitRate;
                    lockedEnemy = currentTarget;
                    lastSwitchTime = now;
                }

                uintptr_t targetNode = isHeadshot ? rpm<uintptr_t>(currentTarget + Offsets::Bones::Head) : rpm<uintptr_t>(currentTarget + Offsets::Bones::Chest);

                if (IsValidPointer(targetNode)) {
                    Vector3 currentPos = GetNodePosition(targetNode);
                    if (currentPos.X != 0) {
                        Vector3 targetPos = currentPos;
                        auto& history = g_targetHistory[currentTarget];

                        if (history.lastTime.time_since_epoch().count() != 0) {
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - history.lastTime).count();
                            if (duration > 0 && duration < 200) {
                                float dt = duration / 1000.0f;
                                Vector3 vel;
                                vel.X = (currentPos.X - history.lastPos.X) / dt;
                                vel.Y = (currentPos.Y - history.lastPos.Y) / dt;
                                vel.Z = (currentPos.Z - history.lastPos.Z) / dt;
                                history.velocity.X = history.velocity.X * 0.7f + vel.X * 0.3f;
                                history.velocity.Y = history.velocity.Y * 0.7f + vel.Y * 0.3f;
                                history.velocity.Z = history.velocity.Z * 0.7f + vel.Z * 0.3f;
                                float leadTime = 0.12f;
                                targetPos.X += history.velocity.X * leadTime;
                                targetPos.Y += history.velocity.Y * leadTime;
                                targetPos.Z += history.velocity.Z * leadTime;
                            }
                        }
                        history.lastPos = currentPos;
                        history.lastTime = now;

                        float jX = ((rand() % 100) - 50) / 3000.0f;
                        float jY = ((rand() % 100) - 50) / 3000.0f;
                        float jZ = ((rand() % 100) - 50) / 3000.0f;

                        if (isHeadshot) targetPos.Y += (0.045f + jY); else targetPos.Y -= (0.085f + jY);
                        targetPos.X += jX; targetPos.Z += jZ;

                        Vector3 startPos = rpm<Vector3>(aimingInfo + Offsets::Player::AimingInfoStartPos);
                        Vector3 dir;
                        dir.X = targetPos.X - startPos.X; dir.Y = targetPos.Y - startPos.Y; dir.Z = targetPos.Z - startPos.Z;

                        wpm<Vector3>(aimingInfo + Offsets::Player::AimingInfoDirection, dir);
                    }
                }
            }
            std::this_thread::yield();
        }
    }

    void ClearData() {
        std::lock_guard<std::mutex> lock(players_mutex);
        players.clear();
        realPlayerCount = 0;
        currentBestTarget = 0;
        g_HasData.store(false);

        g_InGame = false;
        // 🔥 FIXED: Don't clear cache here, let it expire naturally to prevent blinking
        // ClearPlayerCache();
    }

    void update() {
        if (!proc::pid || !proc::lib) {
            ClearData();
            return;
        }

        // 🔥 MAKHAAN UPDATE: Removed 8ms sleep. Thread control is now in main.cpp
        // This ensures the highest possible refresh rate for ESP.

        static bool threadsStarted = false;
        if (!threadsStarted) {
            std::thread(SilentAimThread).detach();
            threadsStarted = true;
        }

        auto GameFacade_c = rpm<uintptr_t>(proc::lib + Offsets::GameFacade);
        if (!IsValidPointer(GameFacade_c)) { ClearData(); return; }

        auto P2 = rpm<uintptr_t>(GameFacade_c + Offsets::GameFacade_P2);
        if (!IsValidPointer(P2)) { ClearData(); return; }
        auto BaseGame = rpm<uintptr_t>(P2);
        if (!IsValidPointer(BaseGame)) { ClearData(); return; }

        auto m_Match = rpm<uintptr_t>(BaseGame + Offsets::Engine::Match);
        static int failCounter = 0;
        if (!IsValidPointer(m_Match)) {
            failCounter++;
            if (failCounter > 15) { // 15 frames of safety
                g_MatchRunning = false;
                ClearData();
            }
            return;
        }

        int MatchIsRunning = rpm<int>(m_Match + Offsets::Match::MatchRunning);
        if (MatchIsRunning != 1) { 
            failCounter++;
            if (failCounter > 15) {
                if (g_MatchRunning) {
                    cfg::other::activate_menu = false;
                }
                g_MatchRunning = false;
                ClearData();
            }
            return;
        } 
        failCounter = 0; // Reset on success
        if (!g_MatchRunning) {
            cfg::other::activate_menu = true;
        }
        g_MatchRunning = true;

        auto localPlayer = rpm<uintptr_t>(m_Match + Offsets::Match::LocalPlayer);
        if (!IsValidPointer(localPlayer)) {
            auto CurrentObserve = rpm<uintptr_t>(m_Match + Offsets::CurrentObserve);
            if (IsValidPointer(CurrentObserve)) {
                localPlayer = rpm<uintptr_t>(CurrentObserve + Offsets::ObserverPlayer);
            }
            if(!IsValidPointer(localPlayer)) { ClearData(); return; }
        }

        if (rpm<bool>(localPlayer + Offsets::Player::IsDead) != 0) {
            ClearData(); 
            return; 
        }

        g_InGame = true;

        int isFiring = rpm<int>(localPlayer + Offsets::Player::FiringState);

        // 🔥 SAME TO SAME SPEED HACK LOGIC (Directly from A_FF-MAX-STREMER-SRC)
        static float lastSpeedValue = -1.0f;
        auto TimeServiceRapid = rpm<uintptr_t>(BaseGame + Offsets::Engine::TimeService);
        if (IsValidPointer(TimeServiceRapid)) {
            if (cfg::mods::speed_hack && cfg::other::activate_menu && isFiring <= 0) {
                // Speed ON - Fast (Exact value from reference)
                float targetSpeed = 0.05900000036f;
                if (lastSpeedValue != targetSpeed) {
                    wpm<float>(TimeServiceRapid + Offsets::Engine::TimeServiceSpeed, targetSpeed);
                    lastSpeedValue = targetSpeed;
                }
            } else {
                // Speed OFF - Normal (Exact value from reference)
                float targetSpeed = 0.03299999982f;
                if (lastSpeedValue != targetSpeed) {
                    wpm<float>(TimeServiceRapid + Offsets::Engine::TimeServiceSpeed, targetSpeed);
                    lastSpeedValue = targetSpeed;
                }
            }
        }

        // 🔥 MAKHAAN OPTIMIZATION: Separate Entity Scan from Position Update
        // Scanning 1000 entities is heavy. We do it in a background "Task" style.
        auto now = std::chrono::steady_clock::now();
        static auto lastRefresh = std::chrono::steady_clock::now();
        static bool isScanning = false;

        // 🔥 MAKHAAN: Initial scan or periodic refresh
        bool listEmpty = false;
        {
            std::lock_guard<std::mutex> lock(g_cacheMutex);
            listEmpty = g_activeEntities.empty();
        }

        if ((listEmpty || std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefresh).count() > 3000) && !isScanning) {
            isScanning = true;
            std::thread([BaseGame, localPlayer]() {
                auto dictionary = rpm<uintptr_t>(BaseGame + Offsets::Engine::Dictionary);
                auto entitylist = rpm<uintptr_t>(dictionary + Offsets::Engine::EntityList);
                if (IsValidPointer(entitylist)) {
                    std::vector<EntityCache> newList;
                    for (int i = 0; i < 600; i++) { // 🔥 Optimized scan range
                        auto ent = rpm<uintptr_t>(entitylist + i * 8);
                        if (IsValidPointer(ent) && ent != localPlayer) {
                            auto AvatarManager = rpm<uintptr_t>(ent + Offsets::Player::AvatarManager);
                            if (IsValidPointer(AvatarManager)) {
                                EntityCache ec;
                                ec.address = ent;
                                ec.isBot = (rpm<int>(ent + Offsets::Player::ClientBotCheck) == 1);
                                if (ec.isBot) strcpy(ec.name, oxorany("Enemy"));
                                else {
                                    uintptr_t NamePtr = rpm<uintptr_t>(ent + Offsets::Player::NamePtr);
                                    if (NamePtr > 0x10000000) getUTF8(ec.name, NamePtr);
                                    if (ec.name[0] == '\0') strcpy(ec.name, oxorany("Enemy"));
                                }
                                newList.push_back(ec);
                            }
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(g_cacheMutex);
                        g_activeEntities = newList;
                    }
                }
                lastRefresh = std::chrono::steady_clock::now();
                isScanning = false;
            }).detach();
        }

        uintptr_t activeCamOwner = localPlayer;
        bool cameraFound = false;

        auto FollowCamera = rpm<uintptr_t>(localPlayer + Offsets::Camera::FollowCamera);
        if (IsValidPointer(FollowCamera)) {
            auto Camera = rpm<uintptr_t>(FollowCamera + Offsets::Camera::CameraObject);
            if (IsValidPointer(Camera)) {
                auto IntPtrCam = rpm<uintptr_t>(Camera + Offsets::Camera::RawCameraData);
                if (IsValidPointer(IntPtrCam)) {
                    SharedViewMatrix = rpm<D3DMatrix>(IntPtrCam + Offsets::Camera::ViewMatrix);
                    cameraFound = true;
                }
            }
        }

        if(!cameraFound) { ClearData(); return; }

        Vector3 LocalPosition = GetNodePosition(GetPlayerHeadTF(activeCamOwner));

        std::vector<PlayerData> tempPlayers;
        int tReal = 0; int tBot = 0;

        uintptr_t bestTarget = 0;
        Vector3 bestPos = Vector3::Zero();
        float bestDist = FLT_MAX;

        float aimFovRadius = (cfg::aim::fov / 360.0f) * (g_sh / 2.0f);

        // 🔥 USE CACHED ENTITIES: Loop through ~50 instead of 1000!
        std::vector<EntityCache> localEntities;
        {
            std::lock_guard<std::mutex> lock(g_cacheMutex);
            localEntities = g_activeEntities;
        }

        for (const auto& cache : localEntities) {
            auto enemy = cache.address;
            if (!IsValidPointer(enemy)) continue;

            auto AvatarManager = rpm<uintptr_t>(enemy + Offsets::Player::AvatarManager);
            if (!IsValidPointer(AvatarManager)) continue;
            auto UmaAvatarSimple = rpm<uintptr_t>(AvatarManager + Offsets::Avatar::UmaSimple);
            if (!IsValidPointer(UmaAvatarSimple)) continue;

            if (!rpm<bool>(UmaAvatarSimple + Offsets::Avatar::IsVisible)) continue;

            auto UmaData = rpm<uintptr_t>(UmaAvatarSimple + Offsets::Avatar::UmaData);
            if (!IsValidPointer(UmaData)) continue;
            if (rpm<bool>(UmaData + Offsets::Avatar::IsDead)) continue;

            int Health = 0;
            uintptr_t hp_step1 = rpm<uintptr_t>(enemy + Offsets::Player::HealthChain1);
            if (IsValidPointer(hp_step1)) {
                uintptr_t hp_step2 = rpm<uintptr_t>(hp_step1 + Offsets::Player::HealthChain2);
                if (IsValidPointer(hp_step2)) {
                    uintptr_t hp_step3 = rpm<uintptr_t>(hp_step2 + Offsets::Player::HealthChain3);
                    if (IsValidPointer(hp_step3)) Health = rpm<int>(hp_step3 + Offsets::Player::HealthValue);
                }
            }
            if (Health <= 0 || Health > 500) continue;

            bool isKnocked = false;
            auto statusStruct = rpm<uintptr_t>(enemy + Offsets::Player::StatusStruct);
            if (IsValidPointer(statusStruct)) {
                auto maybeDead = rpm<uintptr_t>(statusStruct + Offsets::Status::StatusData);
                if (IsValidPointer(maybeDead) && rpm<int>(maybeDead + Offsets::Status::StateID) == 8) isKnocked = true;
            }

            Vector3 EnemyPos = GetTargetPosition(enemy);
            if (EnemyPos != Vector3::Zero()) {
                auto screen = HookW2S(SharedViewMatrix, EnemyPos);
                if (screen.Z >= 0.001f) {
                    bool canAim = true;
                    if (cfg::aim::visibility_check && !rpm<bool>(UmaAvatarSimple + Offsets::Avatar::IsVisible)) canAim = false;
                    if (cfg::aim::ignore_knock && isKnocked) canAim = false;

                    if (canAim) {
                        float screenDist = sqrt((screen.X - g_sw/2)*(screen.X - g_sw/2) + (screen.Y - g_sh/2)*(screen.Y - g_sh/2));
                        if (screenDist < aimFovRadius && screenDist < bestDist) {
                            bestDist = screenDist;
                            bestTarget = enemy;
                            bestPos = EnemyPos;
                        }
                    }
                }
            }

            auto HeadTF = GetPlayerHeadTF(enemy);
            auto FeetTF = GetPlayerPeTF(enemy);
            if (!IsValidPointer(HeadTF) || !IsValidPointer(FeetTF)) continue;

            auto EnemyPosition = GetNodePosition(HeadTF);
            auto EnemyPositionPe = GetNodePosition(FeetTF);
            if (EnemyPosition == Vector3::Zero()) continue;

            auto LocationHeadBox = HookW2S(SharedViewMatrix, EnemyPosition);
            auto LocationToeBox = HookW2S(SharedViewMatrix, EnemyPositionPe);
            if (LocationHeadBox.Z < 0.001f) continue;

            PlayerData data;
            data.Head = LocationHeadBox;
            data.Toe = LocationToeBox;
            data.Distance = Vector3::Distance(LocalPosition, EnemyPosition);
            if (data.Distance < 0.1f) continue;

            data.IsKnocked = isKnocked;
            data.Health = Health;
            data.IsBot = cache.isBot;
            strcpy(data.PlayerName, cache.name);

            if (data.IsBot) tBot++; else tReal++;

            float headToFeetHeight = abs(data.Toe.Y - data.Head.Y);
            data.BoxHeight = headToFeetHeight * 1.15f; 
            data.BoxWidth = data.BoxHeight * 0.48f;
            data.BoxX = data.Head.X - (data.BoxWidth / 2.0f);
            data.BoxY = data.Head.Y - (data.BoxHeight * 0.12f);

            if (cfg::esp::skeleton) {
                data.sHead = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::Head));
                data.sChest = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::Chest));
                data.sHip = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::Hip));
                data.sLeftShoulder = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::LeftShoulder));
                data.sRightShoulder = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::RightShoulder));
                data.sLeftHand = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::LeftHand));
                data.sRightHand = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::RightHand));
                data.sLeftAnkle = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::LeftAnkle));
                data.sRightAnkle = HookW2S(SharedViewMatrix, GetBonePosition(enemy, Offsets::Bones::RightAnkle));
            }
            tempPlayers.push_back(data);
        }

        if (!tempPlayers.empty()) {
            UpdatePlayerCache(tempPlayers);
        }

        if (bestTarget) {
            g_LocalPlayer.store(localPlayer);
            g_CurrentTargetObj.store(bestTarget);
            currentBestPos = HookW2S(SharedViewMatrix, bestPos);
            currentBestTarget = bestTarget;
            g_HasData.store(true);
        } else {
            g_CurrentTargetObj.store(0);
            g_HasData.store(false);
            currentBestTarget = 0;
        }

        {
            std::lock_guard<std::mutex> lock(players_mutex);
            players = tempPlayers;
            realPlayerCount = tReal;
            botCount = tBot;
            // currentBestTarget handled above
        }
    }

    // ========================================================================
    // ─── ADVANCED ESP RENDERING WITH CACHE (BLINKING FIXED) ──────────
    // ========================================================================
    ImU32 GetHealthColor(int health) {
        if (health >= 160) return IM_COL32(0, 255, 0, 255);
        if (health >= 100) return IM_COL32(255, 255, 0, 255);
        if (health >= 50) return IM_COL32(255, 165, 0, 255);
        return IM_COL32(255, 0, 0, 255);
    }

    ImU32 GetDistanceColor(float distance) {
        if (distance < 50) return IM_COL32(255, 0, 0, 255);
        if (distance < 150) return IM_COL32(255, 255, 0, 255);
        return IM_COL32(0, 0, 255, 255);
    }

    ImU32 GetRainbowColor(float offset = 0.0f) {
        static float hue = 0.0f;
        hue += 0.005f; if (hue > 1.0f) hue -= 1.0f;
        ImColor color = ImColor::HSV(fmodf(hue + offset, 1.0f), 1.0f, 1.0f);
        return color;
    }

    ImU32 GetBoxColor(int health, float distance, bool isKnocked) {
        if (isKnocked) return IM_COL32(255, 0, 0, 255);
        return ImGui::ColorConvertFloat4ToU32(cfg::esp::box_color);
    }

    ImU32 GetLineColor(int health, float distance, bool isKnocked) {
        if (isKnocked) return IM_COL32(255, 0, 0, 255);
        return ImGui::ColorConvertFloat4ToU32(cfg::esp::line_color);
    }

    void DrawCornerBox(ImDrawList* draw, float x, float y, float width, float height, ImU32 color, float thickness) {
        float cornerLen = width * 0.25f;
        draw->AddLine(ImVec2(x, y), ImVec2(x + cornerLen, y), color, thickness);
        draw->AddLine(ImVec2(x, y), ImVec2(x, y + cornerLen), color, thickness);
        draw->AddLine(ImVec2(x + width - cornerLen, y), ImVec2(x + width, y), color, thickness);
        draw->AddLine(ImVec2(x + width, y), ImVec2(x, y + cornerLen), color, thickness);
        draw->AddLine(ImVec2(x, y + height), ImVec2(x + cornerLen, y + height), color, thickness);
        draw->AddLine(ImVec2(x, y + height - cornerLen), ImVec2(x, y + height), color, thickness);
        draw->AddLine(ImVec2(x + width - cornerLen, y + height), ImVec2(x + width, y + height), color, thickness);
        draw->AddLine(ImVec2(x + width, y + height - cornerLen), ImVec2(x + width, y + height), color, thickness);
    }

    void DrawAdvancedBox(ImDrawList* draw, float x, float y, float width, float height, int health, float distance, bool isKnocked) {
        ImU32 finalColor = GetBoxColor(health, distance, isKnocked);
        ImU32 highlightCol = (finalColor & 0x00FFFFFF) | ((int)(cfg::esp::box_glow_intensity * 255) << 24);
        float th = cfg::esp::box_thickness;

        auto draw_rect_sharpened = [&](ImVec2 min, ImVec2 max, ImU32 col, float rounding, ImDrawFlags flags, float thick) {
            draw->AddRect(min, max, highlightCol, rounding, flags, thick + 1.0f);
            draw->AddRect(min, max, col, rounding, flags, thick);
        };

        switch (cfg::esp::box_style) {
            case 0: draw_rect_sharpened(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, 0.0f, 0, th); break;
            case 1: 
                draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), (finalColor & 0x00FFFFFF) | ((int)(cfg::esp::box_fill_alpha * 255) << 24));
                draw_rect_sharpened(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, 0.0f, 0, th); break;
            case 2: 
                draw->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + width + 1, y + height + 1), highlightCol, 0.0f, 0, th + 0.5f);
                draw->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, 0.0f, 0, th); break;
            case 3: 
                draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), (finalColor & 0x00FFFFFF) | ((int)(0.2f * 255) << 24));
                draw_rect_sharpened(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, 0.0f, 0, th); break;
            case 4:
                DrawCornerBox(draw, x, y, width, height, highlightCol, th + 1.0f);
                DrawCornerBox(draw, x, y, width, height, finalColor, th); break;
            case 6: 
                for (float tx = x; tx < x + width; tx += 16.f) {
                    draw->AddLine(ImVec2(tx, y), ImVec2(ImMin(tx + 8.f, x + width), y), highlightCol, th + 1.0f);
                    draw->AddLine(ImVec2(tx, y), ImVec2(ImMin(tx + 8.f, x + width), y), finalColor, th);
                    draw->AddLine(ImVec2(tx, y + height), ImVec2(ImMin(tx + 8.f, x + width), y + height), highlightCol, th + 1.0f);
                    draw->AddLine(ImVec2(tx, y + height), ImVec2(ImMin(tx + 8.f, x + width), y + height), finalColor, th);
                }
                for (float ty = y; ty < y + height; ty += 16.f) {
                    draw->AddLine(ImVec2(x, ty), ImVec2(x, ImMin(ty + 8.f, y + height)), highlightCol, th + 1.0f);
                    draw->AddLine(ImVec2(x, ty), ImVec2(x, ImMin(ty + 8.f, y + height)), finalColor, th);
                    draw->AddLine(ImVec2(x + width, ty), ImVec2(x + width, ImMin(ty + 8.f, y + height)), highlightCol, th + 1.0f);
                    draw->AddLine(ImVec2(x + width, ty), ImVec2(x + width, ImMin(ty + 8.f, y + height)), finalColor, th);
                }
                break;
            case 8: draw_rect_sharpened(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, cfg::esp::box_corner_round, 0, th); break;
            default: draw_rect_sharpened(ImVec2(x, y), ImVec2(x + width, y + height), finalColor, 0.0f, 0, th); break;
        }
    }

    void DrawAdvancedHealthBar(ImDrawList* draw, float x, float y, float width, float height, int health, bool isKnocked) {
        float maxHealth = 200.0f;
        float healthPercent = ImClamp(health / maxHealth, 0.0f, 1.0f);
        float fillHeight = height * healthPercent;
        float fillY = y + (height - fillHeight);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), IM_COL32(0, 0, 0, 150), cfg::esp::health_rounding);

        ImU32 hpColor;
        if(isKnocked) hpColor = IM_COL32(255, 0, 0, 255);
        else hpColor = cfg::esp::health_gradient ? GetHealthColor(health) : IM_COL32(0, 255, 0, 255);

        draw->AddRectFilled(ImVec2(x, fillY), ImVec2(x + width, y + height), hpColor, cfg::esp::health_rounding);
    }

    void DrawAdvancedLine(ImDrawList* draw, ImVec2 from, ImVec2 to, float distance, int health, bool isKnocked) {
        ImU32 col = GetLineColor(health, distance, isKnocked);
        ImU32 highlightCol = (col & 0x00FFFFFF) | ((int)(cfg::esp::line_glow_intensity * 255) << 24);
        float th = cfg::esp::line_thickness;

        if (cfg::esp::line_style == 1) {
            float len = sqrt(pow(to.x - from.x, 2) + pow(to.y - from.y, 2));
            for (float t = 0; t < len; t += 8.0f) {
                ImVec2 pos = ImVec2(from.x + (to.x - from.x) * (t / len), from.y + (to.y - from.y) * (t / len));
                draw->AddCircleFilled(pos, 2.5f, highlightCol);
                draw->AddCircleFilled(pos, 2.0f, col);
            }
        } else {
            draw->AddLine(from, to, highlightCol, th + 1.0f);
            draw->AddLine(from, to, col, th);
        }
    }

    void DrawSafeBoneLine(ImDrawList* draw, Vector3 p1, Vector3 p2, ImU32 color, float thick) {
        if (p1.X <= 0 || p1.Y <= 0 || p2.X <= 0 || p2.Y <= 0) return;
        ImU32 highlightCol = (color & 0x00FFFFFF) | ((int)(cfg::esp::line_glow_intensity * 255) << 24);
        draw->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), highlightCol, thick + 1.0f);
        draw->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, thick);
    }

    void RenderESP(ImDrawList* draw, float sw, float sh) {
        if (!cfg::other::activate_menu) return; // 🔥 GLOBAL SHIELD 🔥

        std::vector<PlayerData> playersCopy;
        {
            std::lock_guard<std::mutex> lock(players_mutex);
            playersCopy = players;
        }

        static auto matchStartTime = std::chrono::steady_clock::now();
        static bool wasInMatch = false;

        if (g_MatchRunning && !wasInMatch) {
            matchStartTime = std::chrono::steady_clock::now();
        }
        wasInMatch = g_MatchRunning;

        if (cfg::esp::enable && cfg::esp::show_timer) {
            float timerY = sh - 120.0f;
            float timerX = sw / 2.0f;

            std::string timerText;
            ImU32 timerColor = IM_COL32(255, 255, 255, 255);

            if (!g_MatchRunning) {
                timerText = "NOT IN GAME";
                timerColor = IM_COL32(180, 180, 180, 255);
            } else {
                auto now = std::chrono::steady_clock::now();
                int elapsedSecs = std::chrono::duration_cast<std::chrono::seconds>(now - matchStartTime).count();
                int remainingSecs = 190 - elapsedSecs;

                if (remainingSecs > 0) {
                    int m = remainingSecs / 60;
                    int s = remainingSecs % 60;
                    char buf[64];
                    sprintf(buf, "MATCH TIMER: %d:%02d", m, s);
                    timerText = buf;
                    timerColor = IM_COL32(0, 255, 255, 255);
                } else {
                    timerText = "FINISH THE GAME";
                    timerColor = IM_COL32(255, 50, 50, 255);
                }
            }

            float fontSize = 39.0f;
            ImVec2 tSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, timerText.c_str());

            draw->AddRectFilled(ImVec2(timerX - tSize.x/2 - 23, timerY), ImVec2(timerX + tSize.x/2 + 23, timerY + 60.0f), IM_COL32(12, 8, 10, 235), 12.0f);
            draw->AddRect(ImVec2(timerX - tSize.x/2 - 23, timerY), ImVec2(timerX + tSize.x/2 + 23, timerY + 60.0f), timerColor, 12.0f, 0, 2.5f);
            draw->AddText(NULL, fontSize, ImVec2(timerX - tSize.x/2, timerY + 10.0f), timerColor, timerText.c_str());
        }

        if (!g_InGame) {
            static auto lastGameTime = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastGameTime).count() > 5) {
                if (g_hasValidCache) ClearPlayerCache();
            }
            return;
        }

        if (cfg::aim::show_fov) {
            float fovRad = (cfg::aim::fov / 360.0f) * (sh / 2.0f);
            draw->AddCircle(ImVec2(sw/2, sh/2), fovRad, ImGui::ColorConvertFloat4ToU32(cfg::aim::fov_color), 100, 2.5f);
        }

        if (cfg::aim::show_aimline && currentBestTarget != 0) {
            if (currentBestPos.X > 0 && currentBestPos.Y > 0) {
                draw->AddLine(ImVec2(sw/2, sh/2), ImVec2(currentBestPos.X, currentBestPos.Y), IM_COL32(0, 255, 255, 255), 2.5f);
            }
        }

        if (!cfg::esp::enable) return;

        bool hasFreshData = !playersCopy.empty();
        bool hasCache = g_hasValidCache && !IsCacheExpired();

        // ===== DRAW ENEMY COUNTER =====
        if (cfg::esp::count) {
            int displayReal = hasFreshData ? realPlayerCount : (hasCache ? realPlayerCount : 0);

            char eText[32];
            sprintf(eText, "ENEMIES: %d", displayReal);

            float totalW = 240.0f;
            float boxX = (sw - totalW) / 2.0f; float boxY = 90.0f;
            ImVec2 pMin = ImVec2(boxX, boxY); ImVec2 pMax = ImVec2(boxX + totalW, boxY + 60.0f);

            draw->AddRectFilled(pMin, pMax, IM_COL32(12, 8, 10, 235), 18.0f);
            for (int i = 1; i <= 5; i++) draw->AddRect(ImVec2(pMin.x-i*1.5f, pMin.y-i*1.5f), ImVec2(pMax.x+i*1.5f, pMax.y+i*1.5f), IM_COL32(0, 255, 255, 60/i), 18.f+i, 0, 2.5f);
            draw->AddRect(pMin, pMax, IM_COL32(0, 255, 255, 255), 18.0f, 0, 3.5f);

            ImVec2 tSize = ImGui::GetFont()->CalcTextSizeA(35.f, FLT_MAX, 0.0f, eText);
            draw->AddText(NULL, 35.f, ImVec2(boxX + (totalW - tSize.x)/2.0f, boxY + 12), IM_COL32(0, 255, 255, 255), eText);
        }

        if (!hasFreshData && !hasCache) {
            return;
        }

        int playerCount = 0;
        bool useFresh = hasFreshData;

        if (useFresh) {
            playerCount = playersCopy.size();
        } else if (hasCache) {
            playerCount = g_cachedPlayerCount;
        } else {
            return;
        }

        for (int i = 0; i < playerCount; i++) {
            Vector3 headPos, toePos;
            float boxX, boxY, boxWidth, boxHeight, distance;
            int health;
            bool isKnocked, isBot;
            char playerName[64] = "";
            Vector3 sHead, sChest, sHip, sLeftShoulder, sRightShoulder;
            Vector3 sLeftHand, sRightHand, sLeftAnkle, sRightAnkle;

            if (useFresh) {
                const auto& p = playersCopy[i];
                headPos = p.Head; toePos = p.Toe;
                boxX = p.BoxX; boxY = p.BoxY; boxWidth = p.BoxWidth; boxHeight = p.BoxHeight;
                distance = p.Distance; health = p.Health; isKnocked = p.IsKnocked; isBot = p.IsBot;
                strncpy(playerName, p.PlayerName, 63); playerName[63] = '\0';
                sHead = p.sHead; sChest = p.sChest; sHip = p.sHip; sLeftShoulder = p.sLeftShoulder;
                sRightShoulder = p.sRightShoulder; sLeftHand = p.sLeftHand; sRightHand = p.sRightHand;
                sLeftAnkle = p.sLeftAnkle; sRightAnkle = p.sRightAnkle;
            } else {
                const auto& p = g_cachedPlayers[i];
                if (!p.isValid) continue;
                headPos = p.Head; toePos = p.Toe;
                boxX = p.BoxX; boxY = p.BoxY; boxWidth = p.BoxWidth; boxHeight = p.BoxHeight;
                distance = p.Distance; health = p.Health; isKnocked = p.IsKnocked; isBot = p.IsBot;
                strncpy(playerName, p.PlayerName, 63); playerName[63] = '\0';
                sHead = p.sHead; sChest = p.sChest; sHip = p.sHip; sLeftShoulder = p.sLeftShoulder;
                sRightShoulder = p.sRightShoulder; sLeftHand = p.sLeftHand; sRightHand = p.sRightHand;
                sLeftAnkle = p.sLeftAnkle; sRightAnkle = p.sRightAnkle;
            }

            if (cfg::esp::skeleton) {
                ImU32 sCol = isKnocked ? IM_COL32(255, 0, 0, 255) : ImGui::ColorConvertFloat4ToU32(cfg::esp::skeleton_color);

                float th = 1.8f;
                DrawSafeBoneLine(draw, sHead, sChest, sCol, th);
                DrawSafeBoneLine(draw, sChest, sHip, sCol, th);
                DrawSafeBoneLine(draw, sChest, sLeftShoulder, sCol, th);
                DrawSafeBoneLine(draw, sChest, sRightShoulder, sCol, th);
                DrawSafeBoneLine(draw, sLeftShoulder, sLeftHand, sCol, th);
                DrawSafeBoneLine(draw, sRightShoulder, sRightHand, sCol, th);
                DrawSafeBoneLine(draw, sHip, sLeftAnkle, sCol, th);
                DrawSafeBoneLine(draw, sHip, sRightAnkle, sCol, th);
            }

            if (cfg::esp::box) DrawAdvancedBox(draw, boxX, boxY, boxWidth, boxHeight, health, distance, isKnocked);
            if (cfg::esp::health && health > 0) DrawAdvancedHealthBar(draw, boxX - 8.f, boxY, cfg::esp::health_width, boxHeight, health, isKnocked);
            if (cfg::esp::line) DrawAdvancedLine(draw, ImVec2(sw/2.f, 80.f), ImVec2(headPos.X, headPos.Y), distance, health, isKnocked);
            if (cfg::esp::head_dot) draw->AddCircleFilled(ImVec2(headPos.X, headPos.Y), cfg::esp::head_dot_radius, IM_COL32(255,0,0,255));

            ImFont* font = NULL;

            // 1. Health text: Above head
            if (cfg::esp::health_text && health > 0) {
                float fSize = cfg::esp::hp_text_size;
                ImU32 textColor = ImGui::ColorConvertFloat4ToU32(cfg::esp::hp_text_color);
                char hText[16]; sprintf(hText, "%d", health);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fSize, FLT_MAX, 0.0f, hText);
                ImVec2 pos = ImVec2(boxX + (boxWidth - textSize.x) / 2.0f, boxY - fSize - 6.0f);

                for(float offX = -1.0f; offX <= 1.0f; offX += 1.0f) {
                    for(float offY = -1.0f; offY <= 1.0f; offY += 1.0f) {
                        if(offX == 0 && offY == 0) continue;
                        draw->AddText(font, fSize, ImVec2(pos.x + offX, pos.y + offY), IM_COL32(0, 0, 0, 255), hText);
                    }
                }
                draw->AddText(font, fSize, pos, textColor, hText);
            }

            // 2. Name text: Above health
            if (cfg::esp::name) {
                float fSize = cfg::esp::name_size;
                ImU32 textColor = ImGui::ColorConvertFloat4ToU32(cfg::esp::name_color);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fSize, FLT_MAX, 0.0f, playerName);
                float yOffset = (cfg::esp::health_text && health > 0) ? (cfg::esp::hp_text_size + fSize + 11.f) : (fSize + 6.0f);
                ImVec2 pos = ImVec2(boxX + (boxWidth - textSize.x) / 2.0f, boxY - yOffset);

                for(float offX = -1.0f; offX <= 1.0f; offX += 1.0f) {
                    for(float offY = -1.0f; offY <= 1.0f; offY += 1.0f) {
                        if(offX == 0 && offY == 0) continue;
                        draw->AddText(font, fSize, ImVec2(pos.x + offX, pos.y + offY), IM_COL32(0, 0, 0, 255), playerName);
                    }
                }
                draw->AddText(font, fSize, pos, textColor, playerName);
            }

            // 3. Distance text: Below leg
            if (cfg::esp::distance) {
                float fSize = cfg::esp::distance_size;
                ImU32 textColor = ImGui::ColorConvertFloat4ToU32(cfg::esp::distance_color);
                char dText[16]; sprintf(dText, "%.0fm", distance);
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fSize, FLT_MAX, 0.0f, dText);
                ImVec2 pos = ImVec2(boxX + (boxWidth - textSize.x) / 2.0f, boxY + boxHeight + 2.0f);

                for(float offX = -1.0f; offX <= 1.0f; offX += 1.0f) {
                    for(float offY = -1.0f; offY <= 1.0f; offY += 1.0f) {
                        if(offX == 0 && offY == 0) continue;
                        draw->AddText(font, fSize, ImVec2(pos.x + offX, pos.y + offY), IM_COL32(0, 0, 0, 255), dText);
                    }
                }
                draw->AddText(font, fSize, pos, textColor, dText);
            }

            if (cfg::esp::alert_360 && !isKnocked && headPos.X > 0 && headPos.Y > 0) {
                Draw360Alert(draw, Vector3(headPos.X, headPos.Y, distance), distance, sw, sh, isKnocked);
            }
        }
    }
}
