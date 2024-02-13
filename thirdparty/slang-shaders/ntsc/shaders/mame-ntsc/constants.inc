// Useful Constants
const vec4 Zero = vec4(0.0);
const vec4 Half = vec4(0.5);
const vec4 One = vec4(1.0);
const vec4 Two = vec4(2.0);
const vec3 Gray = vec3(0.3, 0.59, 0.11);
const float Pi = 3.1415926535;
const float Pi2 = 6.283185307;

// NTSC Constants
const vec4 A = vec4(0.5);
const vec4 A2 = vec4(1.0);
const vec4 B = vec4(0.5);
const float P = 1.0;
const float CCFrequency = 3.59754545;
const float NotchUpperFrequency = 3.59754545 + 2.0;
const float NotchLowerFrequency = 3.59754545 - 2.0;
const float YFrequency = 6.0;
const float IFrequency = 1.2;
const float QFrequency = 0.6;
const float NotchHalfWidth = 2.0;
const float ScanTime = 52.6;
const float Pi2ScanTime = 6.283185307 * 52.6;
const float MaxC = 2.1183;
const vec4 YTransform = vec4(0.299, 0.587, 0.114, 0.0);
const vec4 ITransform = vec4(0.595716, -0.274453, -0.321263, 0.0);
const vec4 QTransform = vec4(0.211456, -0.522591, 0.311135, 0.0);
const vec3 YIQ2R = vec3(1.0, 0.956, 0.621);
const vec3 YIQ2G = vec3(1.0, -0.272, -0.647);
const vec3 YIQ2B = vec3(1.0, -1.106, 1.703);
const vec4 MinC = vec4(-1.1183);
const vec4 CRange = vec4(3.2366);
const vec4 InvCRange = vec4(1.0/3.2366);
const float Pi2Length = Pi2 / 63.0;
const vec4 NotchOffset = vec4(0.0, 1.0, 2.0, 3.0);
const vec4 W = vec4(Pi2 * CCFrequency * ScanTime);

// Color Convolution Constants
const vec3 RedMatrix = vec3(1.0, 0.0, 0.0);
const vec3 GrnMatrix = vec3(0.0, 1.0, 0.0);
const vec3 BluMatrix = vec3(0.0, 0.0, 1.0);
const vec3 DCOffset = vec3(0.0, 0.0, 0.0);
const vec3 ColorScale = vec3(0.95, 0.95, 0.95);
const float Saturation = 1.4;

// Deconverge Constants
const vec3 ConvergeX = vec3(-0.4,  0.0, 0.2);
const vec3 ConvergeY = vec3( 0.0, -0.4, 0.2);
const vec3 RadialConvergeX = vec3(1.0, 1.0, 1.0);
const vec3 RadialConvergeY = vec3(1.0, 1.0, 1.0);

// Scanline/Pincushion Constants
const float PincushionAmount = 0.015;
const float CurvatureAmount = 0.015;
//const float ScanlineAmount = 0.175; <- move to parameter
const float ScanlineScale = 1.0;
const float ScanlineHeight = 1.0;
const float ScanlineBrightScale = 1.0;
const float ScanlineBrightOffset = 0.0;
const float ScanlineOffset = 0.0;
const vec3 Floor = vec3(0.05, 0.05, 0.05);

// 60Hz Bar Constants
const float SixtyHertzRate = (60.0 / 59.97 - 1.0); // Difference between NTSC and line frequency
const float SixtyHertzScale = 0.1;