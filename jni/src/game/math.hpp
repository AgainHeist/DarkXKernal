#pragma once
#include <cmath>
#include "../other/memory.hpp"
#include "../Unity/Vector3.hpp"
#include "../Unity/Quaternion.hpp"
#include "../Unity/Vector2.hpp"
#include "offsets.h"

struct Vector4 {
    float X;
    float Y;
    float Z;
    float W;
};

struct Matrix {
    Vector4 Position;
    Quaternion Rotation;
    Vector4 Scale;
};

struct D3DMatrix {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

inline Vector3 GetPosition(uint64_t Transform) {
    auto pos = Vector3::Zero();

    auto transformObjValue = rpm<uint64_t>(Transform + Offsets::Transform::TransformToObj);
    if (transformObjValue == 0) {
        return pos;
    }

    auto indexValue = rpm<uint64_t>(transformObjValue + Offsets::Transform::ObjToIndex);
    auto matrixValue = rpm<uint64_t>(transformObjValue + Offsets::Transform::ObjToMatrix);

    auto matrixListValue = rpm<uint64_t>(matrixValue + Offsets::Transform::MatrixList);
    auto matrixIndicesValue = rpm<uint64_t>(matrixValue + Offsets::Transform::MatrixIndices);

    auto resultValue = rpm<Vector3>(matrixListValue + (indexValue * Offsets::Transform::MatrixSize));

    auto maxTries = 50;
    auto tries = 0;

    auto transformIndexValue = rpm<int>(matrixIndicesValue + (indexValue * Offsets::Transform::IndexSize));

    while (transformIndexValue >= 0 && tries < maxTries) {
        tries++;

        auto tMatrixValue = rpm<Matrix>(matrixListValue + (transformIndexValue * Offsets::Transform::MatrixSize));

        auto rotX = tMatrixValue.Rotation.X;
        auto rotY = tMatrixValue.Rotation.Y;
        auto rotZ = tMatrixValue.Rotation.Z;
        auto rotW = tMatrixValue.Rotation.W;

        auto scaleX = resultValue.X * tMatrixValue.Scale.X;
        auto scaleY = resultValue.Y * tMatrixValue.Scale.Y;
        auto scaleZ = resultValue.Z * tMatrixValue.Scale.Z;

        resultValue.X = tMatrixValue.Position.X + scaleX +
                        (scaleX * ((rotY * rotY * -2.0) - (rotZ * rotZ * 2.0))) +
                        (scaleY * ((rotW * rotZ * -2.0) - (rotY * rotX * -2.0))) +
                        (scaleZ * ((rotZ * rotX * 2.0) - (rotW * rotY * -2.0)));
        resultValue.Y = tMatrixValue.Position.Y + scaleY +
                        (scaleX * ((rotX * rotY * 2.0) - (rotW * rotZ * -2.0))) +
                        (scaleY * ((rotZ * rotZ * -2.0) - (rotX * rotX * 2.0))) +
                        (scaleZ * ((rotW * rotX * -2.0) - (rotZ * rotY * -2.0)));
        resultValue.Z = tMatrixValue.Position.Z + scaleZ +
                        (scaleX * ((rotW * rotY * -2.0) - (rotX * rotZ * -2.0))) +
                        (scaleY * ((rotY * rotZ * 2.0) - (rotW * rotX * -2.0))) +
                        (scaleZ * ((rotX * rotX * -2.0) - (rotY * rotY * 2.0)));

        transformIndexValue = rpm<int>(matrixIndicesValue + (transformIndexValue * Offsets::Transform::IndexSize));
    }

    if (tries < maxTries) {
        pos = resultValue;
    }

    return pos;
}

inline Vector3 GetNodePosition(uint64_t nodeTransform) {
    auto transformValue = rpm<uint64_t>(nodeTransform + Offsets::Transform::TransformToObj);
    if (transformValue == 0) {
        return Vector3::Zero();
    }
    return GetPosition(transformValue);
}

inline uint64_t GetPlayerHeadTF(uint64_t player) {
    auto ptr = rpm<uint64_t>(player + Offsets::Bones::Head);
    return ptr;
}

inline uint64_t GetPlayerPeTF(uint64_t player) {
    auto ptr = rpm<uint64_t>(player + Offsets::Bones::Feet);
    return ptr;
}

// Global screen size variables need to be accessed
extern float g_sw;
extern float g_sh;

inline Vector3 WorldToScreenPoint(const D3DMatrix& viewMatrix, const Vector3& ScreenPos) {
    float v9 = (ScreenPos.X * viewMatrix._11) + (ScreenPos.Y * viewMatrix._21) + (ScreenPos.Z * viewMatrix._31) + viewMatrix._41;
    float v10 = (ScreenPos.X * viewMatrix._12) + (ScreenPos.Y * viewMatrix._22) + (ScreenPos.Z * viewMatrix._32) + viewMatrix._42;
    float v12 = (ScreenPos.X * viewMatrix._14) + (ScreenPos.Y * viewMatrix._24) + (ScreenPos.Z * viewMatrix._34) + viewMatrix._44;

    if (v12 >= 0.001f) {
        float v13 = g_sw / 2.0f;
        float v14 = g_sh / 2.0f;

        return Vector3(
            v13 + (v13 * v9) / v12,
            v14 - (v14 * v10) / v12,
            0
        );
    }
    return Vector3(-1, -1, -1);
}
