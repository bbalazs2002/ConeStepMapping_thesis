// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

float binomialCoeff(int n, int k) {
    float result = 1.0;
    for (int i = 1; i <= k; i++) {
        result *= float(n - (k - i)) / float(i);
    }
    return result;
}

float ipow(float base, int exponent) {
    float result = 1.0;
    for (int i = 0; i < exponent; i++)
        result *= base;
    return result;
}