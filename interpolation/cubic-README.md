# Cubic Interpolation

This is a method of interpolation that uses cubic polynomials to connect real pixels and estimate pixels in between to fill the screen (generally it's used for upscaling, though not necessarily). In two dimensions, a cubic interpolation is called bicubic. A Cubic polynomial is a curve of third degree of this general form:

```sh
f(x) = Ax^3 + Bx^2 + Cx + D
```
When the polynomials are piecewise linear, which means they're smooth (second derivative is zero on each side of a real pixel connecting two polynomials), then these cubics are called cubic splines. Cubic splines are the best curves to interpolate using cubic polynomials, because the smooth transition between polynomials through a real pixel looks seamless to the observer.

There are many methods developed to implement bicubic (2-dimensions) interpolation. The most famous bicubic implementation are the BC-spline proposed by Mitchell-Netravali:

https://www.cs.utexas.edu/~fussell/courses/cs384g-fall2013/lectures/mitchell/Mitchell.pdf

They proposed a family of cubic spline interpolators parameterized (B and C parameters) that can be used to perform some known cubic spline interpolations:

```sh
B=1.0, C=0.0 --> B-Spline
B=0.0, C=0.5 --> Catmull-Rom
B=1.0/3.0, C=1.0/3.0 --> Mitchell-Netravali or "vanilla" bicubic
```

Other cubic splines were proposed by Helmut Dersch (see below). He developed the spline16 and spline36 interpolators.

https://www.panotools.org/dersch/interpolator/interpolator.html

b-spline-fast
--------------

This shader implements b-spline interpolation from Mitchell-Netravali method. It's a multipass shader that separates interpolation in horizontal and vertical directions, reducing the number of texture fetches from 16 to only 8. This shader produces a very smooth interpolation that ends up being too blurry if used to upscale by a scale factor bigger than 2x. Because of its blurry nature, it doesn't overshoot at edge pixels points and, hence, doesn't need an anti-ringing as other shaders. It's good for small upscalings and very good for downscaling.

bicubic-fast
--------------

This shader implements classical Mitchell-Netravali method recommendation (B=C=1.0/3.0). It's a multipass shader that separates interpolation in horizontal and vertical directions, reducing the number of texture fetches from 16 to only 8. This shader produces an interpolation that balances very well blurriness, pixelization and ringing. Being a very good method to upscale/downscale using any scale factor. It produces a bit of ringing, so in this implementation there's an anti-ringing code enabled.

bicubic
--------------

This shader implements classical Mitchell-Netravali method recommendation (B=C=1.0/3.0). It was the first bicubic implementation in libretro and is an standalone shader, so it's slower than the multipass version described earlier. It doesn't have anti-ringing code, so the overshoot can be seen mainly in huds of cartoon games.

catmull-rom-fast
--------------

This shader implements catmull-rom interpolation from Mitchell-Netravali method using B=0.0 and C=0.5. It's a multipass shader that separates interpolation in horizontal and vertical directions, reducing the number of texture fetches from 16 to only 8. This shader produces an interpolation that enphasizes sharpness instead pixelization and blurriness. This way it overshoots strongly produncing very noticeable rings around edges. This implementation has an anti-ringing code enabled to minimize this bad effect. It's a very good shader to upscale to any scale factor.

catmull-rom
--------------

This shader implements catmull-rom interpolation from Mitchell-Netravali method using B=0.0 and C=0.5. It's a standalone shader that uses only 9 texture fetches in linear space, so it's very fast even being standalone. This shader produces an interpolation that enphasizes sharpness instead pixelization and blurriness. This way it overshoots strongly produncing very noticeable rings around edges. Unfortunately, this implementation doesn't allow an easy way to anti-ringing, so it doesn't have one. It's a very good shader to upscale to any scale factor.

spline16-fast
--------------

This shader implements spline16 interpolation from Helmut Dersch method for cubic splines. It's a multipass shader that separates interpolation in horizontal and vertical directions, reducing the number of texture fetches from 16 to only 8. This shader produces an interpolation that enphasizes sharpness instead pixelization and blurriness. It's even sharper than Catmull-Rom shader, rivaling with lanczos2 interpolation. It overshoots strongly producing a very noticeable rings around edges. This implementation has an anti-ringing code enabled to minimize this bad effect. It's a very good shader to upscale to any scale factor.

spline36-fast
--------------

This shader implements spline36 interpolation from Helmut Dersch method for cubic splines. It's a multipass shader that separates interpolation in horizontal and vertical directions, reducing the number of texture fetches from 36 to only 12. This shader produces an interpolation that enphasizes sharpness instead pixelization and blurriness. It's even sharper than spline16 shader, rivaling with lanczos3 interpolation. It overshoots very strongly producing a very noticeable rings around edges. This implementation has an anti-ringing code enabled to minimize this bad effect. It's a very good shader to upscale to any scale factor.
