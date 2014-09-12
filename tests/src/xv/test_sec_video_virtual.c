#include "../main.h"
#include "../xv/test_sec_video_virtual.h"

/*------------------ Fake functions ----------------------------------------------------------- */
/*--------------------------------------------------------------------------------------------- */
#define dixLookupPrivate test_dixLookupPrivate
static inline pointer
test_dixLookupPrivate(PrivatePtr *privates, const DevPrivateKey key)
{
   return 1;
}

#define secUtilVideoBufferRef TestUtilVideoBufferRef
static SECVideoBuf*
TestUtilVideoBufferRef (SECVideoBuf *vbuf)
{
    if (!vbuf)
        return NULL;

    vbuf->ref_cnt++;

    return vbuf;
}

static PixmapRec TestPixmap;
//#define _secVirtualVideoGetPixmap Test_secVirtualVideoGetPixmap
static PixmapPtr Test_secVirtualVideoGetPixmap(WindowPtr pWindow)
{
    PixmapPtr pPix = &TestPixmap;
    if (((DrawablePtr) pWindow)->type == DRAWABLE_WINDOW)
        return (WindowPtr) pPix;
    else
        return (PixmapPtr) pPix;
}
#define secExaPrepareAccess Test_secExaPrepareAccess
static Bool Test_secExaPrepareAccess(PixmapPtr pPix, int index)
{
    if (pPix)
        return TRUE;
    return FALSE;
}
#define secExaFinishAccess Test_secExaFinishAccess
static Bool Test_secExaFinishAccess(PixmapPtr pPix, int index)
{
    if (pPix)
        return;
}


#define VBUF_IS_VALID(v)      ((v) ? TRUE : FALSE)
/************************************************************************************************/

// include functions from sec_video_virtual.c to test
// we do it because some functions within sec_video_virtual.c are static
#include "../../src/xv/sec_video_virtual.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

#define TESTPOINTER 0x25364701
#define FULLHD_WIDTH  1980
#define FULLHD_HEIGHT 1080
#define LCD_WIDTH 720
#define LCD_HEIGHT 1280

/*---------------------------  FIXTURES   ------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------- */
static SECPortPrivPtr pPort_test;
const int testkey = 10;
static RetBufInfo* info[3] = {0,};

void setup_ReturnBuf( void )
{
    int i;

    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->capture = CAPTURE_MODE_STREAM;
    xorg_list_init (&pPort_test->retbuf_info);

    /* add some records to list*/
    for (i=0; i<3; i++ )
    {
        info[i] = calloc (1, sizeof (RetBufInfo));
        info[i]->vbuf = calloc (1, sizeof (SECVideoBuf));
        info[i]->vbuf->keys[0] = testkey + i;
        xorg_list_add (&info[i]->link, &pPort_test->retbuf_info);
       // printf("%d,%p, %d",info[i],info[i]->vbuf, info[i]->vbuf->keys[0]);
    }

}

void teardown_ReturnBuf(void)
{
    int i = 0;
    free(pPort_test);
    for (i = 0; i < 3; i++)
    {
        if (info[i])
        {
            free(info[i]->vbuf);
            free(info[i]);
        }
    }
}


/* ---------------------------------------------------------------------------------------------*/

START_TEST( ut__port_info_pDraw_Null_Pointer)
{
	DrawablePtr pDraw = NULL;
	void* res = NULL;

	res = _port_info ( pDraw);
	if (res)
	{
	    ck_abort_msg("pDraw = NULL failure!");
	}

}
END_TEST;

START_TEST( ut__port_info_pDraw_DRAWABLE_WINDOW)
{
    DrawablePtr pDraw = calloc( 1, sizeof( DrawableRec ));
    pDraw->type = DRAWABLE_WINDOW;
    int res = 0;

    res = _port_info ( pDraw);

    ck_assert_int_ne(res!= 1, "pDraw = DRAWABLE_WINDOW or PIXMAP failure!" );
    free (pDraw);
}
END_TEST;

/*--------------------------- _secVideoGetPortAtom ------------------------------------------------*/
START_TEST( ut__secVideoGetPortAtom_MAX_MIN_check)
{
    Atom res = 1;

    res = _secVideoGetPortAtom(PAA_MAX + 1);
    if (None != res)
    {
        ck_abort_msg("(value > PAA_MAX) check   failure!");
    }
    res = 1;
    res = _secVideoGetPortAtom(PAA_MIN);
    if (None != res)
    {
        ck_abort_msg("(value = PAA_MIN ) check   failure!");
    }
    res = 1;
    res = _secVideoGetPortAtom(PAA_DISPLAY);
    if (None == res)
    {
        ck_abort_msg("(value = PAA_DISPLAY ) check   failure!");
    }
}
END_TEST;

/*--------------------------- _secVirtualVideoSetSecure ------------------------------------------------*/
START_TEST( ut__secVirtualVideoSetSecure_Check_agruments)
{
    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->pDraw = NULL;
      pPort_test->secure = FALSE;

    _secVirtualVideoSetSecure (pPort_test, TRUE);
    if (pPort_test->secure != TRUE)
    {
        ck_abort_msg("secVirtualVideoSetSecure check  failure!");
    }

    free ( pPort_test );
}
END_TEST;

/*--------------------------- _secVirtualVideoIsSupport ------------------------------------------------*/
START_TEST( ut__secVirtualVideoIsSupport_Check_Video_formats)
{
      int res = 0;

      res = _secVirtualVideoIsSupport (FOURCC_RGB32);
      if (TRUE != res)
      {
         ck_abort_msg(" FOURCC_RGB32 video format check  failure!");
      }

      res = 0;
      res = _secVirtualVideoIsSupport (FOURCC_ST12);
      if (TRUE != res)
      {
         ck_abort_msg(" FOURCC_ST12 video format check  failure!");
      }

      res = 0;
      res = _secVirtualVideoIsSupport (FOURCC_SN12);
      if (TRUE != res)
      {
         ck_abort_msg(" FOURCC_SN12 video format check  failure!");
      }

      res = 1;
      res = _secVirtualVideoIsSupport (0);
      if (FALSE != res)
      {
         ck_abort_msg(" Not Existed video format check  failure!");
      }

}
END_TEST;


/*--------------------------- _secVirtualVideoFindReturnBuf ------------------------------------------------*/

START_TEST( ut__secVirtualVideoFindReturnBuf_Check_Buffer_In_List)
{
    RetBufInfo *retinfo = NULL;


    retinfo = _secVirtualVideoFindReturnBuf (pPort_test, testkey);
    if (!retinfo)
    {
        ck_abort_msg("Find buffer1 check  failure!");
    }

    retinfo = _secVirtualVideoFindReturnBuf (pPort_test, testkey + 1);
    if (!retinfo)
    {
        ck_abort_msg("Find buffer2 check  failure!");
    }

    retinfo = _secVirtualVideoFindReturnBuf (pPort_test, testkey + 2);
    if (!retinfo)
    {
       ck_abort_msg("Find buffer2 check  failure!");
    }
}
END_TEST;

START_TEST( ut__secVirtualVideoFindReturnBuf_Check_Not_exist_Buffer_In_List)
{
    RetBufInfo *retinfo = NULL;


    retinfo = _secVirtualVideoFindReturnBuf (pPort_test, testkey + 4);
    if (retinfo)
    {
        ck_abort_msg("Find not exist buffer check failure!");
    }

}
END_TEST;

START_TEST( ut__secVirtualVideoFindReturnBuf_Check_Not_stream_mode)
{
    RetBufInfo *retinfo = NULL;

    pPort_test->capture = CAPTURE_MODE_STILL;
    retinfo = _secVirtualVideoFindReturnBuf (pPort_test, testkey + 1);
    if (retinfo)
    {
        ck_abort_msg("Stream mode check failure!");
    }

}
END_TEST;

/*--------------------------- _secVirtualVideoAddReturnBuf ------------------------------------------------*/

START_TEST( ut__secVirtualVideoAddReturnBuf_Check_Buffer_In_List)
{
    RetBufInfo *retinfo = NULL;

    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->capture = CAPTURE_MODE_STREAM;
    xorg_list_init (&pPort_test->retbuf_info);
    SECVideoBuf* pvbuf = malloc ( sizeof (SECVideoBuf));
    pvbuf->keys[0] = testkey;
    pvbuf->showing = TRUE;

    retinfo = _secVirtualVideoAddReturnBuf (pPort_test, pvbuf);

    if (!retinfo)
    {
        ck_abort_msg("Add buffer check  failure!");
    }

    free(pPort_test);
    free(pvbuf);
}
END_TEST;

/*--------------------------- _secVirtualVideoRemoveReturnBuf ------------------------------------------------*/
START_TEST( ut__secVirtualVideoRemoveReturnBuf_Remove_Buffer_From_List)
{

    RetBufInfo *retinfo = NULL;

    /*Initialize return buffer's list*/
    setup_ReturnBuf();
    pPort_test->wb = NULL;
    RetBufInfo* pinfo = info[0];
    _secVirtualVideoRemoveReturnBuf(pPort_test, pinfo);
    if (_secVirtualVideoFindReturnBuf(pPort_test, testkey))
    {
        ck_abort_msg("Remove return buffer1 check  failure!");
    }

    pinfo = info[1];
    _secVirtualVideoRemoveReturnBuf(pPort_test, pinfo);
    if (_secVirtualVideoFindReturnBuf(pPort_test, testkey + 1))
    {
        ck_abort_msg("Remove return buffer2 check  failure!");
    }

    if (!_secVirtualVideoFindReturnBuf(pPort_test, testkey + 2))
    {
        ck_abort_msg("Buffer3 still exists check  failure!");
    }

    info[1] = info[0] = NULL;
    /* Clear return buffer's list */
    teardown_ReturnBuf();

}
END_TEST;

/*--------------------------- _secVirtualVideoRemoveReturnBufAll ------------------------------------------------*/
START_TEST( ut__secVirtualVideoRemoveReturnBuf_Remove_ALL_Buffers_From_List)
{

    RetBufInfo *retinfo = NULL;

    /*Initialize return buffer's list*/
    setup_ReturnBuf();
    pPort_test->wb = NULL;

   _secVirtualVideoRemoveReturnBufAll (pPort_test);
   if (_secVirtualVideoFindReturnBuf (pPort_test, testkey))
   {
       ck_abort_msg("Remove All return buffers  check  failure!");
   }

   if (_secVirtualVideoFindReturnBuf (pPort_test, testkey + 1))
   {
       ck_abort_msg("Remove All return buffers  check  failure!");
   }

   if (_secVirtualVideoFindReturnBuf (pPort_test, testkey + 2))
   {
       ck_abort_msg("Remove All return buffers  check  failure!");
   }
   info[2] = info[1] = info[0] = NULL;
    /* Clear return buffer's list */
    teardown_ReturnBuf();

}
END_TEST;

/*--------------------------- _secVirtualVideoRemoveReturnBufAll ------------------------------------------------*/
START_TEST( ut__secVirtualVideoDraw_FOURCC_RGB32)
{

    RetBufInfo *retinfo = NULL;
    tbm_bufmgr bufmgr;
    tbm_bo bo1 = tbm_bo_alloc (bufmgr, 1, TBM_BO_DEFAULT);
    tbm_bo_ref(bo1);
    tbm_bo_handle bo_handle = tbm_bo_map (bo1, TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
    *((uint8_t*)bo_handle.ptr) = (uint8_t)55;
    tbm_bo_unmap (bo1);

    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->pDraw = calloc( 1, sizeof( DrawableRec ));

    SECVideoBuf* pvbuf = malloc ( sizeof (SECVideoBuf));
    pvbuf->id = FOURCC_RGB32; pvbuf->width = 720; pvbuf->height = 1280; pvbuf->size = 1;//LCD_WIDTH*LCD_HEIGHT;
    pvbuf->bo[0] = bo1;

    ScreenRec screen;
    pPort_test->pDraw->pScreen=&screen;
    pPort_test->pDraw->pScreen->GetWindowPixmap = Test_secVirtualVideoGetPixmap;

    pPort_test->retire_timer = NULL;
    pPort_test->pDraw->width = pvbuf->width; pPort_test->pDraw->height = pvbuf->height;
    pPort_test->id = FOURCC_RGB32;
    pPort_test->need_damage = TRUE;
    ScrnInfoRec scrninfo;
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc( 1, sizeof( SECRec ));

    TestPixmap.devPrivate.ptr = calloc( 1, sizeof( 1 ));//devprvt.ptr; //calloc(1, sizeof(DevUnion));
    *((uint8_t*)TestPixmap.devPrivate.ptr) = 66;

    _secVirtualVideoDraw(pPort_test, pvbuf);
    if (*((uint8_t*) TestPixmap.devPrivate.ptr) != 55)
    {
        ck_abort_msg("FOURCC_RGB32  check  failure!");
    }

    /* check with need_damage = FALSE. Test must be failed*/
    pPort_test->need_damage = FALSE;
    *((uint8_t*) TestPixmap.devPrivate.ptr) = 66;
    _secVirtualVideoDraw(pPort_test, pvbuf);
    if (*((uint8_t*) TestPixmap.devPrivate.ptr) != 66)
    {
        ck_abort_msg("FOURCC_RGB32  check  failure!");
    }

    /* Clear buffer */
    tbm_bo_unref(bo1);
    free (TestPixmap.devPrivate.ptr);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test->pDraw);
    free (pPort_test);
}
END_TEST;

START_TEST( ut__secVirtualVideoDraw_FOURCC_ST12_DMABUF)
{

    RetBufInfo *retinfo = NULL;

    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->pDraw = calloc( 1, sizeof( DrawableRec ));

    SECVideoBuf* pvbuf = malloc ( sizeof (SECVideoBuf));
    pvbuf->id = FOURCC_ST12; pvbuf->width = 720; pvbuf->height = 1280; pvbuf->size = 1;

    ScreenRec screen;
    pPort_test->pDraw->pScreen=&screen;
    pPort_test->pDraw->pScreen->GetWindowPixmap = Test_secVirtualVideoGetPixmap;

    pPort_test->retire_timer = NULL;
    pPort_test->pDraw->width = pvbuf->width; pPort_test->pDraw->height = pvbuf->height;
    pPort_test->id = FOURCC_ST12;
    pPort_test->need_damage = TRUE;
    ScrnInfoRec scrninfo;
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc( 1, sizeof( SECRec ));
    pPort_test->capture = CAPTURE_MODE_STREAM;
    xorg_list_init (&pPort_test->retbuf_info);

    pvbuf->keys[0] = testkey; pvbuf->keys[1] = testkey +1; pvbuf->keys[2] = testkey +2;
    pvbuf->showing = TRUE;

    TestPixmap.devPrivate.ptr = calloc( 1, sizeof( 1 ));

    _secVirtualVideoDraw(pPort_test, pvbuf);
    XV_DATA* ptr_xvdata = (XV_DATA*) TestPixmap.devPrivate.ptr;
    if ((ptr_xvdata->YBuf != testkey) || (ptr_xvdata->CbBuf != (testkey+1) ) || (ptr_xvdata->CrBuf != (testkey + 2)) )
    {
        ck_abort_msg("FOURCC_ST12 DMABUF XVDATA check  failure!");
    }

    /* Clear buffer */
    free (TestPixmap.devPrivate.ptr);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test->pDraw);
    free (pPort_test);
}
END_TEST;

START_TEST( ut__secVirtualVideoDraw_FOURCC_ST12_LEGACY)
{

    RetBufInfo *retinfo = NULL;

    pPort_test = calloc( 1, sizeof( SECPortPriv ) );
    pPort_test->pDraw = calloc( 1, sizeof( DrawableRec ));

    SECVideoBuf* pvbuf = malloc ( sizeof (SECVideoBuf));
    pvbuf->id = FOURCC_ST12; pvbuf->width = 720; pvbuf->height = 1280; pvbuf->size = 1;

    ScreenRec screen;
    pPort_test->pDraw->pScreen=&screen;
    pPort_test->pDraw->pScreen->GetWindowPixmap = Test_secVirtualVideoGetPixmap;

    pPort_test->retire_timer = NULL;
    pPort_test->pDraw->width = pvbuf->width; pPort_test->pDraw->height = pvbuf->height;
    pPort_test->id = FOURCC_ST12;
    pPort_test->need_damage = TRUE;
    ScrnInfoRec scrninfo;
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc( 1, sizeof( SECRec ));
    pPort_test->capture = CAPTURE_MODE_STREAM;
    xorg_list_init (&pPort_test->retbuf_info);

    pvbuf->phy_addrs[0] = TESTPOINTER; pvbuf->phy_addrs[1] = TESTPOINTER + 10; pvbuf->phy_addrs[2] = TESTPOINTER + 20;
    pvbuf->showing = TRUE;

    TestPixmap.devPrivate.ptr = calloc( 1, sizeof( 1 ));

    _secVirtualVideoDraw(pPort_test, pvbuf);
    XV_DATA* ptr_xvdata = (XV_DATA*) TestPixmap.devPrivate.ptr;
    if ((ptr_xvdata->YBuf != TESTPOINTER) || (ptr_xvdata->CbBuf != (TESTPOINTER + 10) ) || (ptr_xvdata->CrBuf != (TESTPOINTER + 20)) )
    {
        ck_abort_msg("FOURCC_ST12 LEGACY XVDATA check  failure!");
    }

    /* Clear buffer */
    free (TestPixmap.devPrivate.ptr);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test->pDraw);
    free (pPort_test);
}
END_TEST;

/*--------------------------- _secVirtualVideoGetFrontBo ------------------------------------------------*/
//#define xf86CrtcConfigPrivateIndex 0
START_TEST( ut__secVirtualVideoGetFrontBo_LVDS)
{
    ScrnInfoRec scrninfo;
    xf86CrtcConfigPtr pCrtcConfig;
    xf86OutputRec outputrec;
    SECOutputPrivRec outputpriv;
    SECCrtcPrivPtr pCrtcPriv;

    tbm_bufmgr bufmgr;
    tbm_bo bo1 = tbm_bo_alloc (bufmgr, 1, TBM_BO_DEFAULT);
    tbm_bo_ref(bo1);
    tbm_bo_handle bo_handle = tbm_bo_map (bo1, TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
    *((uint8_t*)bo_handle.ptr) = (uint8_t)55;
    tbm_bo_unmap (bo1);

    pPort_test = calloc(1, sizeof(SECPortPriv));
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc(1, sizeof(SECRec));
    pPort_test->pScrn->privates =  calloc(1, sizeof(DevUnion));

    outputpriv.mode_output = calloc(1, sizeof(drmModeConnector));
    outputpriv.mode_output->connector_type = DRM_MODE_CONNECTOR_LVDS;

    pCrtcConfig = pPort_test->pScrn->privates[xf86CrtcConfigPrivateIndex].ptr = calloc(1, sizeof(xf86CrtcConfigRec));
    pCrtcConfig->num_output = 1;
    outputrec.crtc= calloc(1, sizeof(xf86CrtcRec));
    pCrtcPriv = calloc(1, sizeof(SECCrtcPrivRec));
    pCrtcPriv->front_bo = bo1;
    outputrec.crtc->driver_private = pCrtcPriv;
    outputrec.driver_private= &outputpriv;
    xf86OutputPtr poutput=&outputrec;
    pCrtcConfig->output = &poutput;

    tbm_bo bo2 = _secVirtualVideoGetFrontBo (pPort_test, DRM_MODE_CONNECTOR_LVDS);
    tbm_bo_ref(bo2);
    bo_handle = tbm_bo_map (bo2, TBM_DEVICE_CPU, TBM_OPTION_READ);

    if ( *((uint8_t*)bo_handle.ptr) != 55)
    {
        ck_abort_msg("secVirtualVideoGetFrontBo LVDS  check  failure!");
    }
    tbm_bo_unmap (bo2);

    /* Clear buffer */
    tbm_bo_unref(bo1); tbm_bo_unref(bo2);
    free (pCrtcPriv);
    free (outputrec.crtc);
    free (pCrtcConfig );
    free (outputpriv.mode_output );
    free (pPort_test->pScrn->privates);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test);

}
END_TEST;

START_TEST( ut__secVirtualVideoGetFrontBo_HDMI)
{
    ScrnInfoRec scrninfo;
    xf86CrtcConfigPtr pCrtcConfig;
    xf86OutputRec outputrec;
    SECOutputPrivRec outputpriv;
    SECCrtcPrivPtr pCrtcPriv;

    tbm_bufmgr bufmgr;
    tbm_bo bo1 = tbm_bo_alloc (bufmgr, 1, TBM_BO_DEFAULT);
    tbm_bo_ref(bo1);
    tbm_bo_handle bo_handle = tbm_bo_map (bo1, TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
    *((uint8_t*)bo_handle.ptr) = (uint8_t)66;
    tbm_bo_unmap (bo1);

    pPort_test = calloc(1, sizeof(SECPortPriv));
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc(1, sizeof(SECRec));
    pPort_test->pScrn->privates =  calloc(1, sizeof(DevUnion));

    outputpriv.mode_output = calloc(1, sizeof(drmModeConnector));

    outputpriv.mode_output->connector_type = DRM_MODE_CONNECTOR_HDMIA;

    pCrtcConfig = pPort_test->pScrn->privates[xf86CrtcConfigPrivateIndex].ptr = calloc(1, sizeof(xf86CrtcConfigRec));
    pCrtcConfig->num_output = 1;
    outputrec.crtc= calloc(1, sizeof(xf86CrtcRec));
    pCrtcPriv = calloc(1, sizeof(SECCrtcPrivRec));
    pCrtcPriv->front_bo = bo1;
    outputrec.crtc->driver_private = pCrtcPriv;
    outputrec.driver_private= &outputpriv;
    xf86OutputPtr poutput=&outputrec;
    pCrtcConfig->output = &poutput;

    tbm_bo bo2 = _secVirtualVideoGetFrontBo (pPort_test, DRM_MODE_CONNECTOR_HDMIA);
    tbm_bo_ref(bo2);
    bo_handle = tbm_bo_map (bo2, TBM_DEVICE_CPU, TBM_OPTION_READ);

    if ( *((uint8_t*)bo_handle.ptr) != 66)
    {
        ck_abort_msg("secVirtualVideoGetFrontBo HDMI  check  failure!");
    }
    tbm_bo_unmap (bo2);

    /* Clear buffer */
    tbm_bo_unref(bo1); tbm_bo_unref(bo2);
    free (pCrtcPriv);
    free (outputrec.crtc);
    free (pCrtcConfig );
    free (outputpriv.mode_output );
    free (pPort_test->pScrn->privates);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test);

}
END_TEST;

START_TEST( ut__secVirtualVideoGetFrontBo_Unknown)
{
    ScrnInfoRec scrninfo;
    xf86CrtcConfigPtr pCrtcConfig;
    xf86OutputRec outputrec;
    SECOutputPrivRec outputpriv;
    SECCrtcPrivPtr pCrtcPriv;

    pPort_test = calloc(1, sizeof(SECPortPriv));
    pPort_test->pScrn = &scrninfo;
    pPort_test->pScrn->driverPrivate = calloc(1, sizeof(SECRec));
    pPort_test->pScrn->privates =  calloc(1, sizeof(DevUnion));

    outputpriv.mode_output = calloc(1, sizeof(drmModeConnector));

    outputpriv.mode_output->connector_type = DRM_MODE_CONNECTOR_Unknown;

    pCrtcConfig = pPort_test->pScrn->privates[xf86CrtcConfigPrivateIndex].ptr = calloc(1, sizeof(xf86CrtcConfigRec));
    pCrtcConfig->num_output = 1;
    outputrec.crtc= calloc(1, sizeof(xf86CrtcRec));
    pCrtcPriv = calloc(1, sizeof(SECCrtcPrivRec));
    pCrtcPriv->front_bo = NULL;
    outputrec.crtc->driver_private = pCrtcPriv;
    outputrec.driver_private= &outputpriv;
    xf86OutputPtr poutput=&outputrec;
    pCrtcConfig->output = &poutput;

    if ( _secVirtualVideoGetFrontBo (pPort_test, DRM_MODE_CONNECTOR_eDP) != NULL)
    {
        ck_abort_msg("secVirtualVideoGetFrontBo Unknown  check  failure!");
    }


    /* Clear buffer */
    free (pCrtcPriv);
    free (outputrec.crtc);
    free (pCrtcConfig );
    free (outputpriv.mode_output );
    free (pPort_test->pScrn->privates);
    free (pPort_test->pScrn->driverPrivate);
    free (pPort_test);

}
END_TEST;

/*--------------------------- _secVirtualVideoWbDumpFunc ------------------------------------------------*/
START_TEST( ut__secVirtualVideoWbDumpFunc)
{

    SECWb* pwb = calloc(1, sizeof(SECWb));
    pPort_test = calloc(1, sizeof(SECPortPriv));
    SECVideoBuf* pvbuf = calloc(1, sizeof(SECVideoBuf));
    pPort_test->need_damage = FALSE; // _secVirtualVideoDraw have been tested
    pvbuf->keys[0] = testkey;
    pvbuf->keys[1] = testkey + 1;
    pvbuf->keys[2] = testkey + 2;
    pvbuf->showing = TRUE;

    _secVirtualVideoWbDumpFunc(pwb, WB_NOTI_IPP_EVENT, pvbuf, pPort_test);
    if ((pvbuf->keys[0] != testkey) || (pvbuf->keys[1] != (testkey + 1))
            || (pvbuf->keys[2] != (testkey + 2)))
    {
        ck_abort_msg("Work flow test failure!");
    }

    if ((pPort_test->need_damage != FALSE) || ( pvbuf->showing != TRUE))
    {
        ck_abort_msg("Work flow test failure!");
    }

    _secVirtualVideoWbDumpFunc(pwb, WB_NOTI_CLOSED, pvbuf, pPort_test);
    if ((pvbuf->keys[0] != testkey) || (pvbuf->keys[1] != (testkey + 1))
                || (pvbuf->keys[2] != (testkey + 2)))
        {
            ck_abort_msg("Work flow test failure!");
        }

        if ((pPort_test->need_damage != FALSE) || ( pvbuf->showing != TRUE))
        {
            ck_abort_msg("Work flow test failure!");
        }

    /* Clear return buffer's list */
    free (pvbuf);
    free (pPort_test);
    free (pwb);
}
END_TEST;

/*--------------------------- _secVirtualVideoWbDumpDoneFunc ------------------------------------------------*/
START_TEST( ut__secVirtualVideoWbDumpDoneFunc_test1)
{
    pPort_test = calloc(1, sizeof(SECPortPriv));
    pPort_test->wb = calloc(1, sizeof(SECWb));
    (pPort_test->wb)->info_list = calloc(1, sizeof(SECWbNotifyFuncInfo));

    secWbAddNotifyFunc(pPort_test->wb, WB_NOTI_CLOSED,
            _secVirtualVideoWbStopFunc, "test_data");

    _secVirtualVideoWbDumpDoneFunc(pPort_test->wb, WB_NOTI_CLOSED, NULL,
            pPort_test);

    if (pPort_test->wb != NULL)
    {
        free(pPort_test->wb);
        ck_abort_msg("wb object clear test failure!");
    }

    /* Clear return buffer's list */
    free(pPort_test);

}
END_TEST;

START_TEST( ut__secVirtualVideoWbDumpDoneFunc_test2)
{

    pPort_test = calloc(1, sizeof(SECPortPriv));
    pPort_test->wb = calloc(1, sizeof(SECWb));
    (pPort_test->wb)->info_list = calloc(1, sizeof(SECWbNotifyFuncInfo));

    secWbAddNotifyFunc(pPort_test->wb, WB_NOTI_IPP_EVENT,
            _secVirtualVideoWbStopFunc, "test_data");

    _secVirtualVideoWbDumpDoneFunc(pPort_test->wb, WB_NOTI_IPP_EVENT, NULL,
            pPort_test);
    if (pPort_test->wb != NULL)
    {
        free(pPort_test->wb);
        ck_abort_msg("wb object clear test failure!");
    }

    /* Clear return buffer's list */
    free(pPort_test);

}
END_TEST;

/*==========================================================================================================*/

Suite* suite_video_virtual( void )
{
	Suite* s = suite_create( "sec_video_virtual.c" );

	TCase* tc__port_info = tcase_create( "_port_info" );

	tcase_add_test( tc__port_info, ut__port_info_pDraw_Null_Pointer );
	tcase_add_test( tc__port_info, ut__port_info_pDraw_DRAWABLE_WINDOW );

	TCase* tc__secVideoGetPortAtom = tcase_create( "_secVideoGetPortAtom" );
	tcase_add_test( tc__secVideoGetPortAtom, ut__secVideoGetPortAtom_MAX_MIN_check );

	TCase* tc__secVirtualVideoSetSecure = tcase_create( "_secVirtualVideoSetSecure" );
	tcase_add_test( tc__secVirtualVideoSetSecure, ut__secVirtualVideoSetSecure_Check_agruments );

	TCase* tc__secVirtualVideoIsSupport = tcase_create( "_secVirtualVideoIsSupport" );
	tcase_add_test( tc__secVirtualVideoIsSupport, ut__secVirtualVideoIsSupport_Check_Video_formats );

	/*----------  _secVirtualVideoFindReturnBuf() -----------------------------------------------------*/
	TCase* tc__secVirtualVideoFindReturnBuf = tcase_create( "_secVirtualVideoFindReturnBuf" );
    tcase_add_checked_fixture( tc__secVirtualVideoFindReturnBuf, setup_ReturnBuf,
                               teardown_ReturnBuf );

    tcase_add_test( tc__secVirtualVideoFindReturnBuf, ut__secVirtualVideoFindReturnBuf_Check_Buffer_In_List );
    tcase_add_test( tc__secVirtualVideoFindReturnBuf, ut__secVirtualVideoFindReturnBuf_Check_Not_exist_Buffer_In_List );
    tcase_add_test( tc__secVirtualVideoFindReturnBuf, ut__secVirtualVideoFindReturnBuf_Check_Not_stream_mode );

    /*----------  _secVirtualVideoAddReturnBuf() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoAddReturnBuf = tcase_create( "_secVirtualVideoAddReturnBuf" );
    tcase_add_test( tc__secVirtualVideoAddReturnBuf, ut__secVirtualVideoAddReturnBuf_Check_Buffer_In_List );

    /*----------  _secVirtualVideoRemoveReturnBuf() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoRemoveReturnBuf = tcase_create( "_secVirtualVideoRemoveReturnBuf" );
    tcase_add_test( tc__secVirtualVideoRemoveReturnBuf, ut__secVirtualVideoRemoveReturnBuf_Remove_Buffer_From_List );

    /*----------  _secVirtualVideoRemoveReturnBufAll() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoRemoveReturnBufAll = tcase_create( "_secVirtualVideoRemoveReturnBufAll" );
    tcase_add_test( tc__secVirtualVideoRemoveReturnBufAll, ut__secVirtualVideoRemoveReturnBuf_Remove_ALL_Buffers_From_List );

    /*----------  _secVirtualVideoDraw() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoDraw = tcase_create( "_secVirtualVideoDraw" );
    tcase_add_test( tc__secVirtualVideoDraw, ut__secVirtualVideoDraw_FOURCC_RGB32 );
    tcase_add_test( tc__secVirtualVideoDraw, ut__secVirtualVideoDraw_FOURCC_ST12_DMABUF );
    tcase_add_test( tc__secVirtualVideoDraw, ut__secVirtualVideoDraw_FOURCC_ST12_LEGACY );

    /*----------  _secVirtualVideoGetFrontBo() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoGetFrontBo = tcase_create( "_secVirtualVideoGetFrontBo" );
    tcase_add_test( tc__secVirtualVideoGetFrontBo, ut__secVirtualVideoGetFrontBo_LVDS );
    tcase_add_test( tc__secVirtualVideoGetFrontBo, ut__secVirtualVideoGetFrontBo_HDMI );
    tcase_add_test( tc__secVirtualVideoGetFrontBo, ut__secVirtualVideoGetFrontBo_Unknown );

    /*----------  _secVirtualVideoWbDumpFunc() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoWbDumpFunc = tcase_create( "_secVirtualVideoWbDumpFunc" );
    tcase_add_test( tc__secVirtualVideoWbDumpFunc, ut__secVirtualVideoWbDumpFunc );

    /*----------  _secVirtualVideoWbDumpDoneFunc() -----------------------------------------------------*/
    TCase* tc__secVirtualVideoWbDumpDoneFunc = tcase_create( "_secVirtualVideoWbDumpDoneFunc" );
    tcase_add_test( tc__secVirtualVideoWbDumpDoneFunc, ut__secVirtualVideoWbDumpDoneFunc_test1 );
    tcase_add_test( tc__secVirtualVideoWbDumpDoneFunc, ut__secVirtualVideoWbDumpDoneFunc_test2 );



	suite_add_tcase( s, tc__port_info );
	suite_add_tcase( s, tc__secVideoGetPortAtom );
	suite_add_tcase( s, tc__secVirtualVideoSetSecure );
	suite_add_tcase( s, tc__secVirtualVideoIsSupport );
	suite_add_tcase( s, tc__secVirtualVideoFindReturnBuf );
    suite_add_tcase( s, tc__secVirtualVideoAddReturnBuf );
    suite_add_tcase( s, tc__secVirtualVideoRemoveReturnBuf );
    suite_add_tcase( s, tc__secVirtualVideoRemoveReturnBufAll );
    suite_add_tcase( s, tc__secVirtualVideoDraw );
    suite_add_tcase( s, tc__secVirtualVideoGetFrontBo );
    suite_add_tcase( s, tc__secVirtualVideoWbDumpFunc );
    suite_add_tcase( s, tc__secVirtualVideoWbDumpDoneFunc );

	return s;
}
//    printf("************ 1 ********************\n");
//    printf("************ _secVideoGetPortAtom %d ********************\n", atoms[i].atom);
