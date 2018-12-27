// dir_patch.c
// hdiffz dir diff
//
/*
 The MIT License (MIT)
 Copyright (c) 2012-2019 HouSisong
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */
#include "dir_patch.h"
#include <stdio.h>
#include <string.h>
#include "../../libHDiffPatch/HPatch/patch.h"
#include "../../libHDiffPatch/HPatch/patch_private.h"
#include "../../file_for_patch.h"

static const char* kVersionType="DirDiff19&";

#define TUInt hpatch_StreamPos_t

#define  check(value) { \
    if (!(value)){ printf(#value" ERROR!\n");  \
        result=hpatch_FALSE; assert(hpatch_FALSE); goto clear; } }

#define unpackUIntTo(puint,sclip) \
    check(_TStreamCacheClip_unpackUIntWithTag(sclip,puint,0))


hpatch_BOOL getDirDiffInfoByFile(const char* diffFileName,TDirDiffInfo* out_info){
    hpatch_BOOL          result=hpatch_TRUE;
    TFileStreamInput     diffData;
    TFileStreamInput_init(&diffData);
    
    check(TFileStreamInput_open(&diffData,diffFileName));
    result=getDirDiffInfo(&diffData.base,out_info);
    check(TFileStreamInput_close(&diffData));
clear:
    return result;
}

typedef struct _THDirDiffHead {
    TUInt       newPathCount;
    TUInt       oldPathCount;
    TUInt       sameFileCount;
    TUInt       newRefFileCount;
    TUInt       oldRefFileCount;
    TUInt       headDataSize;
    TUInt       headDataCompressedSize;
    TStreamInputClip    headData;
    TStreamInputClip    hdiffData;
} _THDirDiffHead;

static hpatch_BOOL _read_dirdiff_head(TDirDiffInfo* out_info,_THDirDiffHead* out_head,
                                      const hpatch_TStreamInput* dirDiffFile){
    hpatch_BOOL result=hpatch_TRUE;
    check(out_info!=0);
    out_info->isDirDiff=hpatch_FALSE;
    
    TStreamCacheClip  _headClip;
    TStreamCacheClip* headClip=&_headClip;
    TByte             temp_cache[hpatch_kStreamCacheSize];
    _TStreamCacheClip_init(headClip,dirDiffFile,0,dirDiffFile->streamSize,
                           temp_cache,hpatch_kStreamCacheSize);
    
    {//VersionType
        const size_t kVersionTypeLen=strlen(kVersionType);
        if (dirDiffFile->streamSize<kVersionTypeLen)
            return result;//not is dirDiff data
        //assert(tagSize<=hpatch_kStreamCacheSize);
        const TByte* versionType=_TStreamCacheClip_readData(headClip,kVersionTypeLen);
        check(versionType!=0);
        if (0!=memcmp(versionType,kVersionType,kVersionTypeLen))
            return result;//not is dirDiff data
        out_info->isDirDiff=hpatch_TRUE;
    }
    {//read compressType
        const TByte* compressType;
        size_t       compressTypeLen;
        size_t readLen=hpatch_kMaxCompressTypeLength+1;
        if (readLen>_TStreamCacheClip_streamSize(headClip))
            readLen=(size_t)_TStreamCacheClip_streamSize(headClip);
        compressType=_TStreamCacheClip_accessData(headClip,readLen);
        check(compressType!=0);
        compressTypeLen=strnlen((const char*)compressType,readLen);
        check(compressTypeLen<readLen);
        memcpy(out_info->hdiffInfo.compressType,compressType,compressTypeLen+1);
        _TStreamCacheClip_skipData_noCheck(headClip,compressTypeLen+1);
    }
    {
        const TByte kPatchMode =0;
        TUInt savedValue;
        unpackUIntTo(&savedValue,headClip);
        check(savedValue==kPatchMode); //now only support
        unpackUIntTo(&savedValue,headClip);  check(savedValue<=1);
        out_info->newPathIsDir=(hpatch_BOOL)savedValue;
        unpackUIntTo(&savedValue,headClip);  check(savedValue<=1);
        out_info->oldPathIsDir=(hpatch_BOOL)savedValue;
        
        unpackUIntTo(&out_head->newPathCount,headClip);
        unpackUIntTo(&out_head->oldPathCount,headClip);
        unpackUIntTo(&out_head->sameFileCount,headClip);
        unpackUIntTo(&out_head->newRefFileCount,headClip);
        unpackUIntTo(&out_head->oldRefFileCount,headClip);
        unpackUIntTo(&out_head->headDataSize,headClip);
        unpackUIntTo(&out_head->headDataCompressedSize,headClip);
        out_info->dirDataIsCompressed=(out_head->headDataCompressedSize>0);
        unpackUIntTo(&savedValue,headClip);
        check(savedValue==(size_t)savedValue);
        out_info->externDataSize=(size_t)savedValue;
    }
    TUInt curPos=headClip->streamPos-_TStreamCacheClip_cachedSize(headClip);
    TUInt headDataSavedSize=(out_head->headDataCompressedSize>0)?
                                out_head->headDataCompressedSize:out_head->headDataSize;
    streamInputClip_init(&out_head->headData,dirDiffFile,curPos,curPos+headDataSavedSize);
    curPos+=headDataSavedSize;
    check(curPos==(size_t)curPos);
    out_info->externDataOffset=(size_t)curPos;
    curPos+=out_info->externDataSize;
    streamInputClip_init(&out_head->hdiffData,dirDiffFile,curPos,dirDiffFile->streamSize);
    check(getCompressedDiffInfo(&out_info->hdiffInfo,&out_head->hdiffData.base));
clear:
    return result;
}

hpatch_BOOL getDirDiffInfo(const hpatch_TStreamInput* diffFile,TDirDiffInfo* out_info){
    _THDirDiffHead head;
    return _read_dirdiff_head(out_info,&head,diffFile);
}

TDirPatchResult dir_patch(const hpatch_TStreamOutput* out_newData,
                          const char* oldPatch,const hpatch_TStreamInput*  diffData,
                          hpatch_TDecompress* decompressPlugin,
                          hpatch_BOOL isLoadOldAll,size_t patchCacheSize){
    return 1;
}
