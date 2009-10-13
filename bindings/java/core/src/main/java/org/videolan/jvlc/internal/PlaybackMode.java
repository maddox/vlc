/*****************************************************************************
 * VLC Java Bindings JNA Glue
 *****************************************************************************
 * Copyright (C) 1998-2009 the VideoLAN team
 *
 * Authors: Filippo Carone <filippo@carone.org>
 *          VLC bindings generator
 *
 *
 * $Id $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

package org.videolan.jvlc.internal;


public enum PlaybackMode
{

        libvlc_playback_mode_default, // 0,
        libvlc_playback_mode_loop, // 1,
        libvlc_playback_mode_repeat, // 2,
}