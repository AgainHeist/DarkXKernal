#pragma once
#include <cstdint>

namespace Offsets {

    // ═══════════════════════════════════════════════════════════
    //  GLOBAL GAME FACADE
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t GameFacade       = (0xAD68E18);   // 2.133.1
    static const uintptr_t GameFacade_P2    = (0xB8);

    // ═══════════════════════════════════════════════════════════
    //  GAME ENGINE CLASS HIERARCHY
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t BaseGame_Match    = (0x90);
    static const uintptr_t CurrentObserve    = (0x100);
    static const uintptr_t ObserverPlayer    = (0x30);
    static const uintptr_t BaseGame_Timer    = (0x20);
    static const uintptr_t GhostHack         = (0x7A0);
    static const uintptr_t Match_PlayerDict  = (0xC0);
    static const uintptr_t Match_LocalPlayer = (0xD8);

    // ═══════════════════════════════════════════════════════════
    //  C# MONO DICTIONARY INTERNAL LAYOUT
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t Dict_Entries      = (0x18);
    static const uintptr_t Dict_Count        = (0x20);
    static const uintptr_t Dict_ArrayHeader  = (0x20);
    static const uintptr_t Dict_EntryStride  = (0x18);
    static const uintptr_t Dict_EntryValue   = (0x10);

    // ═══════════════════════════════════════════════════════════
    //  PLAYER / ENEMY BASE OFFSETS
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t Player_Rotation   = (0x214);
    static const uintptr_t Player_HeadTF     = (0x6a8);
    static const uintptr_t Player_FootTF     = (0x6d0);
    static const uintptr_t Player_Camera     = (0x698);
    static const uintptr_t Player_IsFiring   = (0x850);
    static const uintptr_t Player_HP         = (0x70);
    static const uintptr_t Player_IsDead     = (0x7C);
    static const uintptr_t Player_ColWrite   = (0x80);
    static const uintptr_t Player_Avatar     = (0x778);
    static const uintptr_t Player_HedColider = (0x740);
    static const uintptr_t Player_DeathInfo  = (0x1d08);
    static const uintptr_t Player_IsBot      = (0x4a8);
    static const uintptr_t Player_Name       = (0x4a0);

    // ═══════════════════════════════════════════════════════════
    //  AVATAR COMPONENTS
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t Avatar_Uma        = (0x138);
    static const uintptr_t Uma_Visible       = (0x101);
    static const uintptr_t Uma_Data          = (0x28);
    static const uintptr_t UmaData_Skip      = (0x81);

    // ═══════════════════════════════════════════════════════════
    //  CAMERA CLASS OFFSETS
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t Camera_Follow     = (0x30);
    static const uintptr_t Camera_IntPtr     = (0x10);
    static const uintptr_t Camera_Matrix     = (0x100);

    // ═══════════════════════════════════════════════════════════
    //  HP COMPONENT TRAVERSAL
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t HP_List           = (0x10);
    static const uintptr_t HP_Entry          = (0x20);
    static const uintptr_t HP_Value          = (0x18);

    // ═══════════════════════════════════════════════════════════
    //  SILENT AIM / BULLET PROJECTILE
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t Player_BulletInfo = (0xe58);
    static const uintptr_t Bullet_SpawnPos   = (0x4C);
    static const uintptr_t Bullet_DirWrite   = (0x40);

    // ═══════════════════════════════════════════════════════════
    //  FALLBACK / MISC
    // ═══════════════════════════════════════════════════════════
    static const uintptr_t MainCameraFallback = (0x3f0);
    static const uintptr_t WeaponInstance     = (0x608);
    static const uintptr_t ReloadInstance     = (0x770);
    static const uintptr_t InventoryManager   = (0x6E0);
    static const uintptr_t StatusStruct       = (0x1d60);

    // ═══════════════════════════════════════════════════════════
    //  NESTED NAMESPACES (DUAL-COMPATIBILITY)
    // ═══════════════════════════════════════════════════════════

    namespace Engine {
        static const uintptr_t BaseGamePtr      = (0xB8);
        static const uintptr_t Match            = (0x90);
        static const uintptr_t Dictionary       = (0xC0);
        static const uintptr_t EntityList       = (0x18);
        static const uintptr_t TimeService      = (0x20);
        static const uintptr_t TimeServiceSpeed = (0x2C);
    }

    namespace Match {
        static const uintptr_t MatchRunning = (0xCC);
        static const uintptr_t LocalPlayer  = (0xD8);
    }

    namespace Camera {
        static const uintptr_t FollowCamera  = (0x698);
        static const uintptr_t CameraObject  = (0x30);
        static const uintptr_t RawCameraData = (0x10);
        static const uintptr_t ViewMatrix    = (0x100);
    }

    namespace Transform {
        static const uintptr_t TransformToObj = (0x10);
        static const uintptr_t ObjToIndex     = (0x40);
        static const uintptr_t ObjToMatrix    = (0x38);
        static const uintptr_t MatrixList     = (0x18);
        static const uintptr_t MatrixIndices  = (0x20);
        static const uintptr_t MatrixSize     = (0x30);
        static const uintptr_t IndexSize      = (0x4);
    }

    namespace Player {
        static const uintptr_t MainCamera          = (0x3f0);
        static const uintptr_t WeaponInstance      = (0x608);
        static const uintptr_t ReloadInstance      = (0x770);
        static const uintptr_t AvatarManager       = (0x778);
        static const uintptr_t StatusStruct        = (0x1d60);
        static const uintptr_t IsSpectating        = (0x7C);
        static const uintptr_t HeadCollider        = (0x740);
        static const uintptr_t AimTargetWrite      = (0x80);
        static const uintptr_t FiringState         = (0x850);
        static const uintptr_t ViewRotation        = (0x214);
        static const uintptr_t ClientBotCheck      = (0x4a8);
        static const uintptr_t NamePtr             = (0x4a0);
        static const uintptr_t InventoryManager    = (0x6E0);
        static const uintptr_t AimingInfo          = (0xe58);
        static const uintptr_t AimingInfoStartPos  = (0x4C);
        static const uintptr_t AimingInfoDirection = (0x40);
        static const uintptr_t HealthChain1        = (0x70);
        static const uintptr_t HealthChain2        = (0x10);
        static const uintptr_t HealthChain3        = (0x20);
        static const uintptr_t HealthValue         = (0x18);
        static const uintptr_t IsDead              = (0x7C);
    }

    namespace Weapon {
        static const uintptr_t WeaponData         = (0x88);
        static const uintptr_t RecoilValue        = (0x18);
        static const uintptr_t FastReloadTrigger  = (0xD9);
    }

    namespace Status {
        static const uintptr_t StatusData = (0x20);
        static const uintptr_t StateID    = (0x10);
    }

    namespace Avatar {
        static const uintptr_t UmaSimple = (0x138);
        static const uintptr_t IsVisible = (0x101);
        static const uintptr_t UmaData   = (0x28);
        static const uintptr_t IsDead    = (0x81);
    }

    namespace Inventory {
        static const uintptr_t ActiveItem      = (0xA0);
        static const uintptr_t WeaponComponent = (0x80);
        static const uintptr_t NoRecoil        = (0x18);
        static const uintptr_t ItemData        = (0x28);
        static const uintptr_t WeaponId        = (0x40);
    }

    namespace Bones {
        static const uintptr_t Head          = (0x6a8);
        static const uintptr_t Chest         = (0x6b8);
        static const uintptr_t Hip           = (0x6b0);
        static const uintptr_t Feet          = (0x6d0);
        static const uintptr_t Neck          = (0x658);
        static const uintptr_t Spine         = (0x650);

        static const uintptr_t LeftShoulder  = (0x710);
        static const uintptr_t RightShoulder = (0x718);
        static const uintptr_t LeftHand      = (0x728);
        static const uintptr_t RightHand     = (0x720);
        static const uintptr_t LeftAnkle     = (0x6e0);
        static const uintptr_t RightAnkle    = (0x6e8);
    }

} // namespace Offsets