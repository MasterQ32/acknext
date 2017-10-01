#ifndef _ACKNEXT_EXT_ACKGUI_H_
#define _ACKNEXT_EXT_ACKGUI_H_

#include <acknext.h>

#define ACKGUI_LAYER 10000

#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <imgui.h>
#pragma GCC diagnostic pop

#else
#warning Note that dear imgui only runs with C++, not with C!
#endif

#ifdef __cplusplus
extern "C"
{
#endif



void ackgui_init();

VIEW * ackgui_getView();


#ifdef __cplusplus
}
#endif

#endif // _ACKNEXT_EXT_ACKGUI_H_
