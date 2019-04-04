/*  =========================================================================
    sphactor - class description

    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef SPHACTOR_H_INCLUDED
#define SPHACTOR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @warning THE FOLLOWING @INTERFACE BLOCK IS AUTO-GENERATED BY ZPROJECT
//  @warning Please edit the model at "api/sphactor.api" to make changes.
//  @interface
//  This is a stable class, and may not change except for emergencies. It
//  is provided in stable builds.
//  Constructor, creates a new Sphactor node.
SPHACTOR_EXPORT sphactor_t *
    sphactor_new (const char *name, zuuid_t *uuid);

//  Destructor, destroys a Sphactor node.
SPHACTOR_EXPORT void
    sphactor_destroy (sphactor_t **self_p);

//  Return our sphactor's UUID string, after successful initialization
SPHACTOR_EXPORT zuuid_t *
    sphactor_uuid (sphactor_t *self);

//  Return our sphactor's name, after successful initialization. First 6
//  characters of the UUID by default.
SPHACTOR_EXPORT const char *
    sphactor_name (sphactor_t *self);

//  Return our sphactor's endpoint, after successful initialization.
//  The endpoint is usually inproc://{uuid}
SPHACTOR_EXPORT const char *
    sphactor_endpoint (sphactor_t *self);

//  Set the public name of this sphactor node overriding the default.
SPHACTOR_EXPORT void
    sphactor_set_name (sphactor_t *self, const char *name);

//  Connect the node's sub socket to a pub endpoint. Returns 0 if succesful -1 on
//  failure.
SPHACTOR_EXPORT int
    sphactor_connect (sphactor_t *self, const char *endpoint);

//  Disconnect the node's sub socket from a pub endpoint. Returns 0 if succesful -1 on
//  failure.
SPHACTOR_EXPORT int
    sphactor_disconnect (sphactor_t *self, const char *endpoint);

//  Start node, after setting header values.
//  Returns 0 if OK, -1 if it wasn't possible to start the node.
SPHACTOR_EXPORT int
    sphactor_start (sphactor_t *self);

//  Stop the node. This will not destroy the node but if you can start
//  it again is undetermined.
SPHACTOR_EXPORT void
    sphactor_stop (sphactor_t *self);

//  Return socket for talking to the sphactor node and for polling.
SPHACTOR_EXPORT zsock_t *
    sphactor_socket (sphactor_t *self);

//  Self test of this class.
SPHACTOR_EXPORT void
    sphactor_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
