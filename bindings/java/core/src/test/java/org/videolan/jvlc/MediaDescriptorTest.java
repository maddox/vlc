/*****************************************************************************
 * MediaDescriptorTest.java: VLC Java Bindings
 *****************************************************************************
 * Copyright (C) 1998-2008 the VideoLAN team
 *
 * Authors: Filippo Carone <filippo@carone.org>
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

package org.videolan.jvlc;

import org.junit.Assert;
import org.junit.Test;


public class MediaDescriptorTest extends AbstractJVLCTest
{

    @Test
    public void getMrlTest()
    {
        MediaDescriptor md = new MediaDescriptor(jvlc, mrl);
        Assert.assertEquals(mrl, md.getMrl());
    }
    
    @Test
    public void getMediaPlayer()
    {
        MediaDescriptor md = new MediaDescriptor(jvlc, mrl);
        MediaPlayer mp = md.getMediaPlayer();
        Assert.assertNotNull(mp);
        MediaPlayer mp2 = md.getMediaPlayer();
        Assert.assertSame(mp, mp2);
    }
    
}
