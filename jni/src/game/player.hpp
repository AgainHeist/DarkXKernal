#pragma once
#include "../Unity/Vector3.hpp"

struct PlayerData {
    Vector3 Head;
    Vector3 Toe;
    
    // Skeleton Points
    Vector3 sHead, sChest, sHip, sLeftShoulder, sRightShoulder, sLeftHand, sRightHand, sLeftAnkle, sRightAnkle;
    
    bool IsKnocked;
    float Distance;
    int Health;
    bool IsBot;
    char PlayerName[64];
    char WeaponName[32];
    
    // Box calc data
    float BoxX, BoxY, BoxWidth, BoxHeight;

    // Default constructor
    PlayerData() : Head(Vector3::Zero()), Toe(Vector3::Zero()), IsKnocked(false), Distance(0), Health(0), IsBot(false) {
        PlayerName[0] = '\0';
        WeaponName[0] = '\0';
        BoxX = BoxY = BoxWidth = BoxHeight = 0.0f;
    }
};