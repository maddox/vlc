/**
 * @file marshal.cs
 * @brief Common LibVLC objects marshalling utilities
 * @ingroup Internals
 */

/**********************************************************************
 *  Copyright (C) 2007-2009 Rémi Denis-Courmont.                      *
 *  This program is free software; you can redistribute and/or modify *
 *  it under the terms of the GNU General Public License as published *
 *  by the Free Software Foundation; version 2 of the license, or (at *
 *  your option) any later version.                                   *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *  See the GNU General Public License for more details.              *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, you can get it from:             *
 *  http://www.gnu.org/copyleft/gpl.html                              *
 **********************************************************************/

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace VideoLAN.LibVLC
{
    /**
     * @brief NonNullHandle: abstract safe handle class for non-NULL pointers
     * @ingroup Internals
     * Microsoft.* namespace has a similar class. However we want to use the
     * System.* namespace only.
     */
    internal abstract class NonNullHandle : SafeHandle
    {
        protected NonNullHandle ()
            : base (IntPtr.Zero, true)
        {
        }

        /**
         * System.Runtime.InteropServices.SafeHandle::IsInvalid.
         */
        public override bool IsInvalid
        {
            get
            {
                return handle == IntPtr.Zero;
            }
        }

        /**
         * Destroys an handle. Cannot fail.
         */
        protected abstract void Destroy ();

        /**
         * System.Runtime.InteropServices.SafeHandle::ReleaseHandle.
         */
        protected override bool ReleaseHandle ()
        {
            Destroy ();
            return true;
        }
    };

    /**
     * @brief BaseObject: generic wrapper around a safe LibVLC handle.
     * @ingroup Internals
     *
     * This is the baseline for all managed LibVLC objects. It wraps:
     *  - an unmanaged LibVLC pointer,
     *  - a native exception structure.
     */
    public class BaseObject : IDisposable
    {
        internal NativeException ex; /**< buffer for LibVLC exceptions */
        internal SafeHandle handle; /**< wrapped safe handle */

        internal BaseObject ()
        {
            ex = new NativeException ();
            this.handle = null;
        }

        /**
         * Checks if the LibVLC run-time raised an exception
         * If so, raises a CIL exception.
         */
        internal void Raise ()
        {
            ex.Raise ();
        }

        /**
         * IDisposable::Dispose.
         */
        public void Dispose ()
        {
            Dispose (true);
            GC.SuppressFinalize (this);
        }

        /**
         * Releases unmanaged resources associated with the object.
         * @param disposing true if the disposing the object explicitly,
         *                  false if finalizing the object inside the GC.
         */
        protected virtual void Dispose (bool disposing)
        {
            if (disposing)
            {
                ex.Dispose ();
                if (handle != null)
                    handle.Close ();
            }
            ex = null;
            handle = null;
        }
    };

    internal class EventManagerHandle : NonNullHandle
    {
        protected override void Destroy ()
        {
        }
    };


    /**
     * @brief EventingObject: wrapper around an eventing LibVLC handle.
     * @ingroup Internals
     *
     * This is the base class for all managed LibVLC objects which do have an
     * event manager.
     */
    public abstract class EventingObject : BaseObject
    {
        private Dictionary<Delegate, IntPtr> events;
        /**< references to our unmanaged function pointers */

        internal EventingObject () : base ()
        {
            events = new Dictionary<Delegate, IntPtr> ();
        }

        /**
         * Releases unmanaged resources associated with the object.
         * @param disposing true if the disposing the object explicitly,
         *                  false if finalizing the object inside the GC.
         */
        protected override void Dispose (bool disposing)
        {
            events = null;
            base.Dispose (disposing);
        }

        /**
         * @return the unmanaged event manager for this object
         */
        internal abstract EventManagerHandle GetManager ();

        /**
         * Registers an event handler.
         * @param type event type to register to
         * @param callback callback to invoke when the event occurs
         *
         * @note
         * For simplicity, we require distinct callbacks for each event type.
         * This is hardly an issue since most events have different formats.
         */
        internal void Attach (EventType type, Delegate callback)
        {
            EventManagerHandle manager;
            IntPtr cb = Marshal.GetFunctionPointerForDelegate (callback);
            bool unref = false;

            /* If things go wrong, we will leak the callback thunk... until
             * this object is destroyed anyway. If we added the thunk _after_
             * the critical section, the native code could try to jump to a
             * non-existent address, which is much worse. */
            events.Add (callback, cb);
            try
            {
                handle.DangerousAddRef (ref unref);
                manager = GetManager ();
                LibVLC.EventAttach (manager, type, cb, IntPtr.Zero, ex);
            }
            finally
            {
                if (unref)
                    handle.DangerousRelease ();
            }
            Raise ();
        }

        internal void Detach (EventType type, Delegate callback)
        {
            EventManagerHandle manager;
            IntPtr cb = events[callback];
            bool unref = false;

            try
            {
                handle.DangerousAddRef (ref unref);
                manager = GetManager ();
                LibVLC.EventDetach (manager, type, cb, IntPtr.Zero, ex);
            }
            finally
            {
                if (unref)
                    handle.DangerousRelease ();
            }
            Raise ();
            events.Remove (callback);
        }
    };
};
