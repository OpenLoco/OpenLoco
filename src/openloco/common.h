#pragma once

#if defined(__GNUC__)
#define FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))
#endif

#ifndef FORCE_ALIGN_ARG_POINTER
#define FORCE_ALIGN_ARG_POINTER
#endif
