/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _GESTUREPROTO_H_
#define _GESTUREPROTO_H_

#include <X11/extensions/gestureconst.h>
#include <X11/X.h>
#include <X11/Xmd.h>

#define Fixed	    INT32

#define GESTURE_EXT_NAME "X Gesture Extension"

#define GESTURE_MAJOR	0	/* current version numbers */
#define GESTURE_MINOR	1
#define GESTURE_PATCH	0

// Requests
#define X_GestureQueryVersion		0
#define X_GestureSelectEvents		1
#define X_GestureGetSelectedEvents	2
#define X_GestureGrabEvent			3
#define X_GestureUngrabEvent		4

//requests
typedef struct _GestureSelectEvents {
    CARD8 reqType;			/* always GestureReqCode */
    CARD8 gestureReqType;	/* always X_GestureSelectEvents */
    CARD16 length B16;
    CARD32 window B32;
    CARD32 mask B32;
} xGestureSelectEventsReq;
#define sz_xGestureSelectEventsReq	12

typedef struct _GestureGetSelectedEvents {
    CARD8 reqType;			/* always GestureReqCode */
    CARD8 gestureReqType;	/* always X_GestureGetSelectedEvents */
    CARD16 length B16;
    CARD32 window B32;
} xGestureGetSelectedEventsReq;
#define sz_xGestureGetSelectedEventsReq	8

typedef struct _GestureGrabEvent {
    CARD8 reqType;			/* always GestureReqCode */
    CARD8 gestureReqType;	/* always X_GestureGrabEvent */
    CARD16 length B16;
    CARD32 window B32;
    CARD32 eventType B32;		/* gesture event type such as "GestureNotifyFlick" */
    CARD8 num_finger;		/* number of fingers in a gesture event */
    CARD8 pad1;
    CARD16 pad2 B16;
    Time time B32;				/* client request timestamp */
} xGestureGrabEventReq;
#define sz_xGestureGrabEventReq	20

typedef struct _GestureUngrabEvent {
    CARD8 reqType;			/* always GestureReqCode */
    CARD8 gestureReqType;	/* always X_GestureUngrabEvent */
    CARD16 length B16;
    CARD32 window B32;
    CARD32 eventType B32;		/* gesture event type such as "GestureNotifyFlick" */
    CARD8 num_finger;		/* number of fingers in a gesture event */
    CARD8 pad1;
    CARD16 pad2 B16;
    Time time B32;				/* client request timestamp */
} xGestureUngrabEventReq;
#define sz_xGestureUngrabEventReq	20

typedef struct _GestureQueryVersion {
    CARD8 reqType;			/* always GestureReqCode */
    CARD8 gestureReqType;	/* always X_GestureQueryVersion */
    CARD16 length B16;
} xGestureQueryVersionReq;
#define sz_xGestureQueryVersionReq	4

//replys for synchronous reqs
typedef struct {
    BYTE type;					/* X_Reply */
    BOOL pad1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 mask;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
} xGestureGetSelectedEventsReply;
#define sz_xGestureGetSelectedEventsReply	32

typedef struct {
    BYTE type;					/* X_Reply */
    BYTE status;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 pad1 B32;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
} xGestureGrabEventReply;
#define sz_xGestureGrabEventReply	32

typedef struct {
    BYTE type;					/* X_Reply */
    BYTE status;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 pad1 B32;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
} xGestureUngrabEventReply;
#define sz_xGestureUngrabEventReply	32

typedef struct {
    BYTE type;					/* X_Reply */
    BOOL pad1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD16 majorVersion B16;	/* major version of Gesture extension protocol */
    CARD16 minorVersion B16;	/* minor version of Gesture extension protocol */
    CARD32 patchVersion B32;	 /* patch version of Gesture extension protocol */
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
} xGestureQueryVersionReply;
#define sz_xGestureQueryVersionReply	32

//events
typedef struct _GestureNotifyGroup {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyGroup */
	CARD16	sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 groupid;
	CARD8 num_group;
	CARD16 pad1 B16;
	CARD32 pad2 B32;
	CARD32 pad3 B32;
	CARD32 pad4 B32;
	CARD32 pad5 B32;
} xGestureNotifyGroupEvent;
#define sz_xGestureNotifyGroupEvent	32

typedef struct _GestureNotifyFlick {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyFlick */
	CARD16 sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 direction;		/* 8 way direction (0 ~ 7) */
	CARD16 distance B16;	/* distance between first point and last point (pixel) */
	Time duration B32;	/* time difference between press and release (ms) */
	Fixed angle B32;		/* angel difference between horizontal line and flick line (radian) */
	CARD32 pad1 B32;
	CARD32 pad2 B32;
} xGestureNotifyFlickEvent;
#define sz_xGestureNotifyFlickEvent	32

typedef struct _GestureNotifyPan {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyPan */
	CARD16	sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 direction;		/* 8 way direction (0 ~ 7) */
	CARD16 distance B16;	/* distance between first point and last point (pixel)*/
	Time duration B32;	/* time difference between press and release (ms) */
	CARD16 dx B16;		/* x coordinate delta */
	CARD16 dy B16;		/* y coordinate delta */
	CARD32 pad1 B32;
	CARD32 pad2 B32;
} xGestureNotifyPanEvent;
#define sz_xGestureNotifyPanEvent	32

typedef struct _GestureNotifyPinchRotation {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyPinchRotation */
	CARD16 sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 pad1;
	CARD16 distance B16;	/* distance from center (pixel) */
	CARD16 cx B16;		/* center x coordinate */
	CARD16 cy B16;		/* center y coordinate */
	Fixed zoom B32;		/* zoom factor (base : 1.0) */
	Fixed angle B32;		/* angel difference between first line and current line (radian) */
	CARD32	pad2 B32;
} xGestureNotifyPinchRotationEvent;
#define sz_xGestureNotifyPinchRotationEvent	32

typedef struct _GestureNotifyTap {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyTap */
	CARD16 sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 pad1;
	CARD16 cx B16;		/* center x coordinate */
	CARD16 cy B16;		/* center y coordinate */
	CARD8 tap_repeat;	/* tap repeats such as SINGLE_TAP, DBL_TAP and so on */
	CARD8 pad2;
	CARD32 interval;		/* time difference between tap and previous tap (ms) */
	CARD32 pad3 B32;
	CARD32 pad4 B32;
} xGestureNotifyTapEvent;
#define sz_xGestureNotifyTapEvent	32

typedef struct _GestureNotifyTapNHold {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyTapNHold */
	CARD16 sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 pad1;
	CARD16 cx B16;		/* center x coordinate */
	CARD16 cy B16;		/* center y coordinate */
	CARD16 pad2 B16;
	Time interval B32;		/* time difference between tap and hold (ms) */
	Time holdtime B32;	/* hold time (ms) */
	CARD32 pad3 B32;
} xGestureNotifyTapNHoldEvent;
#define sz_xGestureNotifyTapNHoldEvent	32

typedef struct _GestureNotifyHold {
	BYTE type;			/* always GestureEventBase + event type */
	BYTE kind;			/* subevent type : GestureNotifyHold */
	CARD16 sequenceNumber B16;
	Window window B32;
	Time time B32;		/* server timestamp when event happened */
	CARD8 num_finger;
	CARD8 pad1;
	CARD16 cx B16;		/* center x coordinate */
	CARD16 cy B16;		/* center y coordinate */
	CARD16 pad2 B16;
	Time holdtime B32;	/* hold time (ms) */
	CARD32 pad3 B32;
	CARD32 pad4 B32;
} xGestureNotifyHoldEvent;
#define sz_xGestureNotifyHoldEvent	32

union _xGestureCommonEvent {
	struct {
		BYTE type;
		BYTE kind;
		CARD16	sequenceNumber B16;
		Window window B32;
		Time time B32;
	} any;
	xGestureNotifyGroupEvent gev;
	xGestureNotifyFlickEvent fev;
	xGestureNotifyPanEvent pev;
	xGestureNotifyPinchRotationEvent pcrev;
	xGestureNotifyTapEvent tev;
	xGestureNotifyTapNHoldEvent thev;
	xGestureNotifyHoldEvent hev;
};

typedef union _xGestureCommonEvent xGestureCommonEvent;

#endif//_GESTUREPROTO_H_

