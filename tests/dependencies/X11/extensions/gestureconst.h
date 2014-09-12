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


#ifndef _GESTUREPROTO_CONST_H_
#define _GESTUREPROTO_CONST_H_

/* Reply status */
#define GestureSuccess				0

#define GestureUngrabSuccess		0
#define GestureUngrabNotGrabbed		1
#define GestureUngrabAbnormal		2

#define GestureGrabSuccess			0
#define GestureGrabAbnormal			1
#define GestureGrabbedAlready		2
#define GestureGrabInvalidTime		3
#define GestureInvalidReply			4

/* Events */
#define GestureNotifyFlick			0
#define GestureNotifyPan				1
#define GestureNotifyPinchRotation	2
#define GestureNotifyTap				3
#define GestureNotifyTapNHold		4
#define GestureNotifyHold				5
#define GestureNotifyGroup			6
#define GestureNumberEvents			(GestureNotifyGroup + 1)

#define GestureMaskNone				0L
#define GestureFlickMask				(1L << 0)
#define GesturePanMask				(1L << 1)
#define GesturePinchRotationMask		(1L << 2)
#define GestureTapMask				(1L << 3)
#define GestureTapNHoldMask			(1L << 4)
#define GestureHoldMask				(1L << 5)
#define GestureGroupMask			(1L << 6)

/* "Kinds" of events for GestureNotifyGroup */
#define GestureGroupRemoved		0
#define GestureGroupAdded			1
#define GestureGroupCurrent			2

/* "Kinds" of events for GestureNotifyFlick ~ GestureNotifyHold */
#define GestureEnd					0
#define GestureBegin					1
#define GestureUpdate				2
#define GestureDone					3

/* Errors */
#define GestureClientNotLocal				0
#define GestureInValidMask				1
#define GestureOperationNotSupported	2
#define GestureNumberErrors		(GestureOperationNotSupported + 1)

#endif//_GESTUREPROTO_CONST_H_

