#include "Easing.h"

float Easing::Ease(EasingType type, float t)
{
    switch (type)
    {
    case EasingType::Linear:
        return Linear(t);

        break;
    case EasingType::EaseInCubic:
        return EaseInCubic(t);

        break;
    case EasingType::EaseOutCubic:
        return EaseOutCubic(t);

        break;
    case EasingType::EaseInOutCubic:
        return EaseInOutCubic(t);

        break;
    case EasingType::EaseOutBack:
        return EaseOutBack(t);

        break;
    case EasingType::EaseOutBounce:
        return EaseOutBounce(t);

        break;
    }
}

float Easing::Linear(float t)
{
    return t;
}

float Easing::EaseInCubic(float t)
{
    return t * t * t;
}

float Easing::EaseOutCubic(float t)
{
    float f = 1.0f - t;
    return 1 - (f * f * f);
}

float Easing::EaseInOutCubic(float t)
{
    float f = -2.0f * t + 2.0f;
    return t < 0.5f ? 4 * t * t * t : 1 - (f * f * f) / 2.0f;
}

float Easing::EaseOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c2 = c1 + 1;
    float f = t - 1.0f;

    return 1 + c2 * (f * f * f) + c1 * (f * f);
}

float Easing::EaseOutBounce(float t)
{
    const float n1 = 7.5625;
    const float d1 = 2.75;

    if (t < 1.0f / d1)
    {
        return n1 * t * t;
    }
    else if (t < 2.0f / d1)
    {
        return n1 * (t -= 1.5 / d1) * t + 0.75;
    }
    else if (t < 2.5f / d1)
    {
        return n1 * (t -= 2.25 / d1) * t + 0.9375;
    }
    else
    {
        return n1 * (t -= 2.625 / d1) * t + 0.984375;
    }
}
