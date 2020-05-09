# PathTracer

## Description
Creating a small path tracer. It is quite similar to the PBRT path tracer, since I primarily learned by looking at that code, and reading snippets from the PBRT book.

## Features
- Spatial data structure: BVH using axis aligned bounding boxes. Defaults to using the surface area heuristic during construction.
- For the path tracer, just diffuse Lambertian surfaces currently
- Importance sampling for the next ray direction, and the direct lighting estimation
- Supported shapes: triangle, sphere
- Supported lights: point, directional, area
- 3D model loading via Assimp
- LDR environment cubemaps
- Tonemapping (Reinhard or Uncharted2) and gamma correction
- Diffuse textures (no texture filtering/anti-aliasing yet)

## Configuring
Tested on GCC9 and MSVC 2019
```
git clone https://github.com/LiamTyler/PathTracer.git
cd PathTracer
mkdir build
cd build
(linux) cmake -DCMAKE_BUILD_TYPE=[Debug/Release] ..
(windows) cmake -G "Visual Studio 16 2016" -A x64 ..
```

## Building
Tested on GCC9 and MSVC 2019

Building from terminal (any platform):
```
cmake --build . --config [Debug/Release]
```

Linux specifc:
```
make -j6
```
Note: With GCC9 a full parallel build (make -j) freezes during the Assimp build on my machine. Just lower the number if it does freeze

Windows specifc:<br>
Just open build/PathTracer.sln and build there

## Running
```
./bin/pathTracer[_debug] <path to scene file>
(example: ./bin/pathTracer ../resources/scenes/cornell.json)
```

## Example Results:
All tests done on an Intel 8700k cpu.<br>
resources/scenes/cornell.json
![Alt text](docs_and_images/cornell_spp_comparison.png)

resources/scenes/sponza.json
![Alt text](docs_and_images/sponza_spp_comparison.png)

## Method
The primary function is `path_tracer.cpp::Li`, which returns the estimated radiance along the given ray. It unrolls the recursion for performance reasons, but the first thing it does is check to see if the ray intersects the scene. If it does, it possibly adds the emitted light of whatever surface it hit (area lights are just regular surfaces with emission 'Ke' > 0):
```
// emitted light of current surface
if ( bounce == 0 && glm::dot( hitData.wo, hitData.normal ) > 0 )
{
    L += pathThroughput * hitData.material->Ke;
}
```
To avoid double counting emitted light, it can only be considered for objects hit by the initial camera rays (bounce == 0). The dot product checks to see if the front side of the area light was hit. After that, we evaluate the BRDF and estimate how much direct light is reaching the current surface from all of the lights:
```
BRDF brdf = hitData.material->ComputeBRDF( &hitData ); 

// estimate direct
glm::vec3 Ld = LDirect( hitData, scene, brdf );
L += pathThroughput * Ld;
```
`LDirect` just loops over all lights, adding the estimated radiance from each of them using this function:

```
glm::vec3 EstimateSingleDirect( Light* light, const IntersectionData& hitData, Scene* scene, const BRDF& brdf )
{
    Interaction it{ hitData.position, hitData.normal };
    glm::vec3 wi;
    float lightPdf;

    // get incoming radiance, and how likely it was to sample that direction on the light
    glm::vec3 Li = light->Sample_Li( it, wi, lightPdf, scene );
    if ( lightPdf == 0 )
    {
        return glm::vec3( 0 );
    }

    return brdf.F( hitData.wo, wi ) * Li * AbsDot( hitData.normal, wi ) / lightPdf;
}
```

The return statement here is where the core rendering equation is. For point and directional lights, the PDF is always 1, but for area lights, a position is randomly sampled on its surface, and probability of sampling that point with respect to the solid angle is calculated. This direct lighting speeds convergence up immensely, and allows for delta (non-hittable) lights such as point and direction lights. It however, is why area lights would be double counted if not for the previous `bounce == 0` check.
Once the direct lighting estimation is done, we sample a new direction from the BRDF, and update the throughput. Throughput just keeps track of how much energy has been absorbed from the hit surfaces:
```
// sample the BRDF to get the next ray's direction (wi)
float pdf;
glm::vec3 wi;
glm::vec3 F = brdf.Sample_F( hitData.wo, wi, pdf );

if ( pdf == 0.f || F == glm::vec3( 0 ) )
{
    break;
}

pathThroughput *= F * AbsDot( wi, hitData.normal ) / pdf;
```
Since the path tracer only supports Lambertian surfaces currently, the `Sample_F` function just samples from a cosine-weighted hemisphere:
```
glm::vec3 BRDF::Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, float& pdf ) const
{
    glm::vec3 localWi = CosineSampleHemisphere( Random::Rand(), Random::Rand() );
    worldSpace_wi     = T * localWi.x + B * localWi.y + N * localWi.z;
    pdf               = Pdf( worldSpace_wo, worldSpace_wi );
    return F( worldSpace_wo, worldSpace_wi );
}
```


## BVH Timings
While the final BVH currently is very similar to PBRT's, it actually went through several iterations back when this was just a **ray tracer** (I implemented a ray tracer first before moving to the path tracer. See past github releases). At each iteration I recorded the timings for two scenes:
1. The Stanford dragon (871k triangles)
2. Sponza (262k triangles)

The first BVH I implemented used decided its splits by cutting the longest axis in half for each AABB successively. I was curious if splitting all the way down to a single shape was the best strategy, or a handful. The children were kept track of as pointers, and the intersection tests were recursive. Here were the results:
```
Middle Split with goal of 1 shape per leaf:
------------------------------
dragon: 1.03
sponza: 100.15

Middle Split with goal of 2 shapes per leaf:
------------------------------
dragon: 1.02
sponza: 101.45

Middle Split with goal of 3 shapes per leaf:
------------------------------
dragon: 1.03
sponza: 101.38
```

That didn't make any difference, I settled on just always aiming for 1 and the performance for Sponza was terrible. It turned out that sometimes the middle split criteria can't split anything, and puts 1000+ triangles into a single leaf. This can happen when a large triangle basically decides the entire AABB bounds, and all the smaller triangles are on one side. To guarantee a no large leaf nodes like this, I switched to splitting the triangles equally into the sub AABBs whenver the middle failed.
```
Middle Split with Equal Counts fallback:
------------------------------
dragon: 1.06
sponza: 3.09
```

That worked a lot better, but PBRT didn't use pointers/recursion, but rather they flattened the BVH into an array, and used a stack based traversal method for intersection.
```
Middle Split with Equal Counts fallback + Flattened:
------------------------------
dragon: 0.58
sponza: 1.78
```

2x performance gain. I also switched to reordering the list of shapes so that the BVH leaf node structs did not need a `std::vector< Shape* >`, but rather a `startIndex` and `numShapes` since they were all sequential.
```
Middle Split with Equal Counts fallback + Flattened + Reordered Shape list:
------------------------------
dragon: 0.57
sponza: 1.74
```

Small, but consistent improvements. The thing I tried was using the Surface Area Heuristic for the splitting decision.
```
SAH + Flattened + Reordered Shape list:
------------------------------
dragon: 0.55
sponza: 0.53
```

Lastly with some minor optimizations to the ray-triangle intersection test and stack traversal, the final results I got where:
```
SAH + Flattened + Reordered Shape list + Optimized tests:
------------------------------
dragon: 0.49
sponza: 0.32
```

## Issues
One of the main issues is trying to not double count area light contributions. Any time a ray hits a surface, a direct lighting estimation is done by looping over all the lights and estimating the incoming radiance of each light. For area lights, you can't actually add their contribution after the first intersection from the primary camera ray. This is because that light's radiance would have already been accounted for along that same path, during the last surface's direct lighting estimation. This however, relies on the fact that the dot product between the area light's normal and M is zero, where M = ray_intersection - randomly_sampled_point_on_area_light. Due to numerical issues and biasing, this isn't always zero. I instead check to see if it is less than 0.002, but sometimes there is still double counting, leading to bright spots on on the image.

## Future Work
The main thing I haven't had time for yet is going beyond Lambertian surfaces. I would like to implement an energy conserving Phong model, and then try for a microfacet model like Cook-Torrence. I would also like to implement Ray Differentials for texture anti-aliasing, normal mapping, and redo the BVH timings with the actual path tracer, not ray tracer.