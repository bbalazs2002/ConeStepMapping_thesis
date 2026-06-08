#pragma once

#include "../Headers/Types.h"

interface IDrawable{
	virtual void Render(const RenderParams& p) = 0;
};