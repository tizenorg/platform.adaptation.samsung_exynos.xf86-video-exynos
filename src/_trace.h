#ifndef __EXYNOS_TRACE_H__
#define __EXYNOS_TRACE_H__




#define TRC_COUNTER(modul) static int enterCnt##modul = sizeof(tab) - 1;\
    static char empty[51] = "                                                  ";\
    static char tab[11] = "\t\t\t\t\t\t\t\t\t\t";



#define TRC_BEGIN(modul, shift)\
	     XDBG_ERROR(M##modul,"%s|%sstarted...\n", &empty[sizeof(empty) - shift + sizeof(__FUNCTION__)], &tab[enterCnt##modul--])

#define TRC_END(modul, shift)\
		XDBG_ERROR(M##modul,"%s|%sfinished:%d\n", &empty[sizeof(empty) - shift + sizeof(__FUNCTION__)], &tab[++enterCnt##modul], __LINE__)


#if 0
TRC_COUNTER(DRI2)
#define DRI2_BEGIN	TRC_BEGIN(DRI2,33)
#define DRI2_END	TRC_END(DRI2,33)
#else
#define DRI2_BEGIN ;
#define DRI2_END ;
#endif

#if 0
TRC_COUNTER(CRTC)
#define CRTC_BEGIN TRC_BEGIN(CRTC, 35)
#define CRTC_END TRC_BEGIN(CRTC, 35)
#else
#define CRTC_BEGIN ;
#define CRTC_END ;
#endif

#if 0
#define DISP_BEGIN XDBG_ERROR(MDISP,"started...\n") 
#define DISP_END XDBG_ERROR(MDISP,"finished\n") 
#else
#define DISP_BEGIN ;
#define DISP_END ;
#endif

#if 0
#define EXA_BEGIN XDBG_ERROR(MEXA,"started...\n") 
#define EXA_END XDBG_ERROR(MEXA,"finished\n") 
#else
#define EXA_BEGIN ;
#define EXA_END ;
#endif

#if 0
#define RENAME(a,b)\
        {int old=a;\
        a=_getName(b);\
        XDBG_ERROR(MDRI2,"\E[0;32;1mRename from %d to %d\n\E[0m",old,a);}
        
#else
#define RENAME(a,b) a=_getName(b)
#endif

/* remove _free from exynos_crtc.c */
#if 0
#define MAKE_FREE(a,b)\
{\
    (a)=(b);\
    if(b)\
    {\
        _free++;\
        XDBG_ERROR(MCRTC,"\E[0;42;1mMake TRUE, now %d line %d\E[0m\n",_free,__LINE__);\
    }\
    else\
    {\
        _free--;\
        XDBG_ERROR(MCRTC,"\E[0;42;1mMake FALSE, now %d line %d\E[0m\n",_free,__LINE__);\
    }\
}
#else
#define MAKE_FREE(a,b) a=b;
#endif

/* remove _temp from exynos_dri2.c */
#if 0
#define PRINTBUFINFO(when,color,_pBackBuf)\
{\
    DRI2BufferPrivPtr Priv = (_pBackBuf)->driverPrivate;\
    EXYNOSExaPixPrivPtr Exa = exaGetPixmapDriverPrivate (Priv->pPixmap);\
    if ((Priv->num_buf==2)&&(_temp++>10))\
    {\
        EXYNOSExaPixPrivPtr Exa1 = exaGetPixmapDriverPrivate (*(Priv->pBackPixmaps+0));\
        EXYNOSExaPixPrivPtr Exa2 = exaGetPixmapDriverPrivate (*(Priv->pBackPixmaps+1));\
        XDBG_ERROR(MDRI2,"\E[0;%d;1m %s : Name %d(%d) | %d,%d idx: %d,%d,%d\n  \E[0m",(color),(when),(_pBackBuf)->name,tbm_bo_export(Exa->tbo),tbm_bo_export(Exa1->tbo),tbm_bo_export(Exa2->tbo),Priv->avail_idx,Priv->cur_idx,Priv->free_idx);\
    }\
    else\
        XDBG_ERROR(MDRI2,"\E[0;%d;1m %s : Name %d(%d)\n \E[0m",(color),(when),(_pBackBuf)->name,tbm_bo_export(Exa->tbo));\
}

#else
#define PRINTBUFINFO(when,color,_pBackBuf) {};
#endif

#if 1
#define TRACE XDBG_DEBUG(MTRC,"%d\n",__LINE__);
#else
#define TRACE ;
#endif

#endif /* of _TRACE_ */
