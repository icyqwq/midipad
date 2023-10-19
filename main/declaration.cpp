#include "declaration.h"

LGFX_Sprite base_canvas(&M5.Display);

void declaration_components_init()
{
	base_canvas.setBuffer(heap_caps_malloc(320 * 240 * 3, MALLOC_CAP_SPIRAM), 240, 320, lgfx::rgb565_2Byte);
    base_canvas.fillSprite(0);
}