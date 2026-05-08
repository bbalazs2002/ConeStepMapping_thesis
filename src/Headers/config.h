#pragma once

// model - modelBase adapters
#define MODEL2MODELBASE ModelBaseParams{params.shaderPrograms,params.name,params.show,params.drawMode}
#define RAYMARCHEDSURFACE2MODEL ModelParams{params.shaderPrograms,params.name,params.show,params.wireFrame,params.drawMode}

// for <math.h>
#define _USE_MATH_DEFINES