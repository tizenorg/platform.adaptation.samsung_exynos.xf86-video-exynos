/*
 * gc.c
 *
 *  Created on: Nov 18, 2013
 *      Author: rmarchenko
 *
 *  fakes function for graphic content
 */


void Gc_funcs_ChangeClip (GCPtr pGC, int type,pointer pvalue ,int nrects ) {

}

void Gc_ops_CopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy, int w, int h, int dstx, int dsty) {

}

void Gc_funcs_DestroyClip (GCPtr pGc) {

}

GCFuncs gcFuncs = {
	.ChangeClip = Gc_funcs_ChangeClip,
	.DestroyClip = Gc_funcs_DestroyClip
};

GCOps gcOps = {
	.CopyArea = Gc_ops_CopyArea
};

struct _GC globalGC = {
	.funcs = &gcFuncs,
    .ops = &gcOps
};

typedef struct _TestGC
{
	int doCrashGetScratchGC;
	int createScratchCount;
} TestGC;

TestGC testGC;


GCPtr GetScratchGC(unsigned d/*depth */, ScreenPtr p/*pScreen */ )
{
	testGC.createScratchCount++;
	return (testGC.doCrashGetScratchGC) ? NULL : &globalGC;
}

int ChangeGC(ClientPtr c/*client */ ,
                              GCPtr gc/*pGC */ ,
                              BITS32 b/*mask */ ,
                              ChangeGCValPtr v/*pCGCV */ )
{
}

void ValidateGC(DrawablePtr pd/*pDraw */ ,GCPtr pGC/*pGC */ )
{
	//only for test null pointer in "pGC"
	return pGC->serialNumber;
}

void FreeScratchGC(GCPtr pGC/*pGC */ )
{
	testGC.createScratchCount--;
}
