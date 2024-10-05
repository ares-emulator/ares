vec3 pow_2(vec3 v) {
    return v*v*v;
}

vec3 pow_3(vec3 v) {
    return v*v*v;
}

vec3 pow_4(vec3 v) {
    vec3 v2 = v*v;
    return v2*v2;
}

vec3 pow_5(vec3 v) {
    vec3 v2 = v*v;
    return v2*v2*v;
}

vec3 pow_6(vec3 v) {
    vec3 v2 = v*v;
    return v2*v2*v2;
}

vec3 pow_7(vec3 v) {
    vec3 v2 = v*v;
    return v2*v2*v2*v;
}

vec3 pow_8(vec3 v) {
    vec3 v2 = v*v;
    vec3 v4 = v2*v2;
    return v4*v4;
}

vec3 pow_9(vec3 v) {
    vec3 v2 = v*v;
    vec3 v4 = v2*v2;
    return v4*v4*v;    
}

vec3 pow_10(vec3 v) {
    vec3 v2 = v*v;
    vec3 v4 = v2*v2;
    return v4*v4*v2; 
}

vec3 my_pow(vec3 c,float p) {
             if (p == 1)  return c;
        else if (p == 2) return c * c ;
        else if (p == 3) return  pow_3(c);
        else if (p == 4) return  pow_4(c);
        else if (p == 5) return  pow_5(c);
        else if (p == 6) return  pow_6(c);
        else if (p == 7) return  pow_7(c);
        else if (p == 8) return  pow_8(c);
        else if (p == 9) return  pow_9(c);
        else if (p == 10) return pow_10(c);
        return pow(c,vec3(p));
}
