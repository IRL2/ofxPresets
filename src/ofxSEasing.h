/// <summary>
/// Simple easing functions for interpolation
/// 
/// Based on gre's simple easing functions
/// https://gist.github.com/gre/1650294
/// </summary>

class ofxSEeasing {
public:

    /// <summary>
    /// Map a value from one range to another and apply an easing function
    /// </summary>
    /// <param name="t">The value to map</param>
    /// <param name="start">The start of the input range</param>
    /// <param name="end">The end of the input range</param>
    /// <param name="min">The start of the output range</param>
    /// <param name="max">The end of the output range</param>
    /// <param name="ease">The easing function to apply</param>
    /// <returns>The mapped and eased value</returns>
    static float map_clamp(float t, float start, float end, float min, float max, float easeOutput) {
        t = std::clamp((t - start) / (end - start), 0.0f, 1.0f);
        return min + (max - min) * easeOutput;
    }
    static float map_clamp(float t, float start, float end, float min, float max, float(*ease)(float)) {
        t = std::clamp((t - start) / (end - start), 0.0f, 1.0f);
        return min + (max - min) * ease(t);
    }

    static float linear(float t) {
        return t;
    }

    static float easeInQuad(float t) {
        return t * t;
    }

    static float easeOutQuad(float t) {
        return t * (2 - t);
    }

    static float easeInOutQuad(float t) {
        if (t < 0.5)
            return 2 * t * t;
        else
            return -1 + (4 - 2 * t) * t;
    }

    static float easeInCubic(float t) {
        return t * t * t;
    }

    static float easeOutCubic(float t) {
        t--;
        return t * t * t + 1;
    }

    static float easeInOutCubic(float t) {
        if (t < 0.5)
            return 4 * t * t * t;
        else {
            return (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
        }
    }

    static float easeInQuart(float t) {
        return t * t * t * t;
    }

    static float easeOutQuart(float t) {
        t--;
        return 1 - (t * t * t * t);
    }

    static float easeInOutQuart(float t) {
        if (t < 0.5)
            return 8 * t * t * t * t;
        else {
            t--;
            return 1 - (8 * t * t * t * t);
        }
    }

    static float easeInQuint(float t) {
        return t * t * t * t * t;
    }

    static float easeOutQuint(float t) {
        t--;
        return 1 + (t * t * t * t * t);
    }

    static float easeInOutQuint(float t) {
        if (t < 0.5)
            return 16 * t * t * t * t * t;
        else {
            t--;
            return 1 + 16 * t * t * t * t * t;
        }
    }
};