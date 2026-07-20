## Visual debug

### SSBO structure

| name          | type   | comment                              |
| ---           | ---    | ---                                  |
| count         | int    | number of steps (+2 if in/out shown) |
| texture2scene | mat4x4 |                                      |
| eye           | vec3   | camera position (in scene space)     |
| direction     | vec3   | view direction (in scene space)      |
| in            | vec3   | enter point (in texture space)       |
| out           | vec3   | exit point (in texture space)        |
| steps         | vec3[] | steps (in texture space)             |

Direction is represented by a point (eye + V)

## Numerical debug

### SSBO structure

| name          | type   | comment                                  |
| ---           | ---    | ---                                      |
| count         | int    | number of steps                          |
| flags         | int    | termination flags                        |
| eye           | vec3   | camera position (in scene space)         |
| direction     | vec3   | view direction (in scene space)          |
| scene2texture | mat4x4 |                                          |
| M_eye         | vec4   | camera position (in texture space)       |
| scene2unit    | mat4x4 |                                          |
| T_eye         | vec4   | camera position (in unit space)          |
| in            | vec3   | enter point (in texture space)           |
| out           | vec3   | exit point (in texture space)            |
| v             | vec3   | vector from in to out (in texture space) |
| steps         | Step[] |                                          |

### Steps structure

| name                 | type | comment                             |
| ---                  | ---  | ---                                 |
| (ti, t, height, tan) | vec4 |                                     |
| ui                   | vec3 | step coordinates (in texture space) |

ti - parameter from u_n to u_(n+1) \
t - parameter from u_0 to u_(n+1) \
height - texture height value below u_n \
tan - tangent from texture below u_n