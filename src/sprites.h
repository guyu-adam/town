#pragma once

#include "raylib.h"

enum class AnimationState {
    Idle,
    Walk,
    Farm,
    Chop,
    Mine,
    Trade
};

struct CharacterSprite {
    Texture2D texture{};
    int frameWidth{32};
    int frameHeight{32};
    int currentFrame{0};
    int totalFrames{4};
    float frameTimer{0.0f};
    AnimationState state{AnimationState::Idle};
};

