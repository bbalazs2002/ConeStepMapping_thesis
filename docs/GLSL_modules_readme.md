# Shader Modules Documentation

## Shader Module Structure and Conventions

To ensure efficient inclusion and usage of shader modules, each module is organized into separate files. This design allows only the necessary components to be loaded when a module is used. All modules adhere to the following conventions:

### 1. Module File Structure
Each module consists of **two files**:

- **`[module]_uniforms.glsl`** – Contains all uniforms, buffers, and preprocessor macros required for the module to function.  
  - If a uniform is a non-primitive type, its definition is included in this file.

- **`[module].glsl`** – Contains the module’s functions.  
  - If a function’s parameters or return type are non-primitive types, their definitions are included here.

### 2. Buffer Binding Conventions
If a module uses any buffers, the corresponding **binding points must be defined as preprocessor macros**. This ensures consistency and allows easy configuration when including the module in shaders.

---

## Table of Contents

- [Camera Module](#camera-module)
- [Color Module](#color-module)
- [Material Module](#material-module)
- [Light Module](#light-module)
- [Math Module](#math-module)
- [Transform Module](#transform-module)

---

## Camera Module

The **Camera** module transforms vertices from world space into screen space.

### Include path
- `./Camera/Camera_uniforms.glsl`
- `./Camera/Camera.glsl`

### Structs
**CameraUniforms**
- viewProj : mat4
- at : vec3
- up : vec3
- eye : vec3

### Uniform Instances
- `cameraData` : `CameraUniforms`

### Functions
- `CameraViewProj(pos : vec4) : vec4`

---

## Color Module

The **Color** module is a simple utility.  
The `Color()` function returns the color provided through its uniform.

### Include path
- `./Color/Color_uniforms.glsl`
- `./Color/Color.glsl`

### Structs
**ColorUniforms**
- color : vec3

### Uniform Instances
- `colorData` : `ColorUniforms`

### Functions
- `Color() : vec4`

---

## Material Module

The **Material** module handles receiving and preparing a model's material for use by the **Light** module, including texturing.

### Functionality
- The `MaterialPrepare()` function returns data in a layout that matches exactly what the `LightCalculate()` function in the **Light** module expects.

### Include path
- `./Material/Material_uniforms.glsl`
- `./Material/Material.glsl`

### Structs
**MaterialUniforms**
- diffuseColorTex : vec4
- specularColorTex : vec4
- ambientColorEmissionTex : vec4
- shininess : float
- hasNormalTex : int

### Uniform Instances
- `materialDiffuseTex` : `sampler2D`
- `materialSpecularTex` : `sampler2D`
- `materialEmissionTex` : `sampler2D`
- `materialNormalTex` : `sampler2D`
- `materialData` : `MaterialUniforms`

### Functions
- MaterialPrepare(texCoord : vec2) : float[13]
  - \[0-2\]:  ambient color
  - \[3-5\]:  diffuse color
  - \[6-8\]:  specular color
  - \[9-11\]: emission color
  - \[12\]: shininess

---

## Light Module

The **Light** module is responsible for computing the illumination of each model.  
It reads light sources from an SSBO and evaluates the color of each fragment using the **Blinn–Phong lighting model**.

### Functionality

- The `LightCalculate` function expects the material data of the model as a `float[13]` array. This array must follow the exact layout produced by the `MaterialPrepare` function from the **Material** module.
- Additionally, preprocessor macros can be used to define which numeric identifiers correspond to each light source type in the SSBO.

### Include path
- `./Light/Light_uniforms.glsl`
- `./Light/Light.glsl`

### Structs
**Light**
- La_const : vec4
- Ld_linear : vec4
- Ls_quadratic : vec4
- direction : vec4
- position : vec4
- flags_angle_plane : vec4
  - x: bitflags (`LIGHT_FLAG_IS_DIR` / `LIGHT_FLAG_IS_POINT` / `LIGHT_FLAG_IS_SPOT` / `LIGHT_FLAG_CASTS_SHADOW` — reserved, unused)
  - y: inner angle (spot) / near plane (point)
  - z: outer angle (spot) / far plane (point)
  - w: padding

**LightCalculateContributionParams**
- light : Light
- position : vec3
- norm : vec3
- viewDir : vec3
- diffuseColor : vec3
- specularColor : vec3
- shininess : float

**LightCalculateParams**
- norm : vec3
- viewDir : vec3
- position : vec3
- material : float[13]
  - \[0-2\]:  ambient color
  - \[3-5\]:  diffuse color
  - \[6-8\]:  specular color
  - \[9-11\]: emission color
  - \[12\]: shininess

**LightUniforms**
- lightCount : int

### Uniform Instances
- `lightData` : `LightUniforms`

### Functions
- `LightCalculateContribution(params : LightCalculateContributionParams) : vec3`
- `LightCalculate(params : LightCalculateParams) : vec3`

### Preprocessor Macros
- `LIGHT_LIGHTS_SSBO`
- `LIGHT_FLAG_IS_DIR = 1u << 0`
- `LIGHT_FLAG_IS_POINT = 1u << 1`
- `LIGHT_FLAG_IS_SPOT = 1u << 2`
- `LIGHT_FLAG_CASTS_SHADOW = 1u << 3` — reserved for future use; shadow mapping is not implemented in this project

---

## Math Module

The **Math** module does not use any uniforms, so the `Math_uniforms.glsl` file does not exist.  
This module is simple and provides utility functions, primarily to support computations related to the Bernstein basis.

### Include path
- `./Math.glsl`

### Functions
- `binomialCoeff(n : int, k : int) : float`
- `ipow(base : float, exponent : int) : float`

## Transform Module

The **Transform** module is responsible for positioning the model within the scene.  

### Structs
**TransformUniforms**
- world : mat4

### Uniform Instances
- `transformData` : `TransformUniforms`

### Functions
- `Transform(pos : vec4) : vec4`

---
