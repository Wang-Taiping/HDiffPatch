
#ifndef HPATCHZ_H
#define HPATCHZ_H

#include "libHDiffPatch/HPatch/patch_types.h"

#ifdef HPATCHZ_EXPORT
#undef HPATCHZ_EXPORT
#endif // HPATCHZ_EXPORT
#ifdef HPATCHZ_BUILD
#define HPATCHZ_EXPORT __declspec(dllexport)
#else
#define HPATCHZ_EXPORT __declspec(dllimport)
#endif // HPATCHZ_BUILD

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	HPATCHZ_EXPORT int hpatch(const char* oldFileName, const char* diffFileName, const char* outNewFileName,
        hpatch_BOOL isLoadOldAll, size_t patchCacheSize, hpatch_StreamPos_t diffDataOffert,
        hpatch_StreamPos_t diffDataSize, hpatch_BOOL vcpatch_isChecksum, hpatch_BOOL vcpatch_isInMem);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !HPATCHZ_H
