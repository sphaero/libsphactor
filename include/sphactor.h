/*  =========================================================================

    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Sphactor, an open-source framework for high level
    actor model concurrency --- http://sphactor.org

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef SPHACTOR_H_INCLUDED
#define SPHACTOR_H_INCLUDED

#pragma message "The __cpluplus value: " __cplusplus
#ifdef __cplusplus
//  forward decl
struct _sphactor_event_t;
typedef _sphactor_event_t sphactor_event_t;

template<class SphactorClass>
SPHACTOR_EXPORT zmsg_t *
sphactor_member_handler(sphactor_event_t *ev, void *args);

template<class SphactorClass>
zmsg_t *
sphactor_member_handler(sphactor_event_t *ev, void *args)
{
    assert(args);
    SphactorClass *self = static_cast<SphactorClass*>(args);
    return self->handleMsg(ev);
};

template<class SphactorClass>
SPHACTOR_EXPORT sphactor_t *
sphactor_new ( SphactorClass *inst, const char *name=nullptr, zuuid_t *uuid=nullptr );

template<class SphactorClass>
sphactor_t *
sphactor_new ( SphactorClass *inst, const char *name=nullptr, zuuid_t *uuid=nullptr )
{
    assert(inst);
    return sphactor_new(sphactor_member_handler<SphactorClass>, inst, name, uuid);
}

// void pointer to a costructor
template<class SphactorClass>
SPHACTOR_EXPORT void *
sphactoractor_constructor();

template<class SphactorClass>
void *
sphactoractor_constructor()
{
    return new SphactorClass;
}

extern "C" {
#endif

//  sphactor event type is received by the handlers (perhaps we'll make this into a zproject class)
typedef struct _sphactor_event_t{
    zmsg_t *msg;  // msg received on the socket
    const char  *type;  // type of event (API or SOC)
    const char  *name;  // name of the actor
    const char  *uuid;  // uuid of the actor
    const sphactor_actor_t  *actor;   // name of the actor
} sphactor_event_t;

//  @warning THE FOLLOWING @INTERFACE BLOCK IS AUTO-GENERATED BY ZPROJECT
//  @warning Please edit the model at "api/sphactor.api" to make changes.
//  @interface
//  This is a stable class, and may not change except for emergencies. It
//  is provided in stable builds.
// Callback function for socket activity
typedef zmsg_t * (sphactor_handler_fn) (
    sphactor_event_t *event, void *arg);

//  Constructor, creates a new Sphactor instance.
SPHACTOR_EXPORT sphactor_t *
    sphactor_new (sphactor_handler_fn handler, void *arg, const char *name, zuuid_t *uuid);

//  Constructor, creates a new Sphactor instance by its typename.
SPHACTOR_EXPORT sphactor_t *
    sphactor_new_by_type (const char *actor_type, void *arg, const char *name, zuuid_t *uuid);

//  Destructor, destroys a Sphactor instance.
SPHACTOR_EXPORT void
    sphactor_destroy (sphactor_t **self_p);

//
SPHACTOR_EXPORT int
    sphactor_register (const char *actor_type, sphactor_handler_fn handler);

//
SPHACTOR_EXPORT int
    sphactor_unregister (const char *actor_type);

//
SPHACTOR_EXPORT zlist_t *
    sphactor_get_registered (void);

//
SPHACTOR_EXPORT void
    sphactor_dispose (void);

//  Return our sphactor's UUID string, after successful initialization
SPHACTOR_EXPORT zuuid_t *
    sphactor_ask_uuid (sphactor_t *self);

//  Return our sphactor's name, after successful initialization. First 6
//  characters of the UUID by default.
SPHACTOR_EXPORT const char *
    sphactor_ask_name (sphactor_t *self);

//  Return our sphactor's type, after successful initialization.
//  NULL by default.
SPHACTOR_EXPORT const char *
    sphactor_ask_actor_type (sphactor_t *self);

//  Return our sphactor's endpoint, after successful initialization.
//  The endpoint is usually inproc://{uuid}
SPHACTOR_EXPORT const char *
    sphactor_ask_endpoint (sphactor_t *self);

//  Set the public name of this Sphactor instance overriding the default.
SPHACTOR_EXPORT void
    sphactor_ask_set_name (sphactor_t *self, const char *name);

//  Set the actor's type.
SPHACTOR_EXPORT void
    sphactor_ask_set_actor_type (sphactor_t *self, const char *actor_type);

//  Set the timeout of this Sphactor's actor. This is used for the timeout
//  of the poller so the sphactor actor is looped for a fixed interval. Note
//  that the sphactor's actor method receives a NULL message if it is
//  triggered by timeout event as opposed to when triggered by a socket
//  event. By default the timeout is -1 implying it never timeouts.
SPHACTOR_EXPORT void
    sphactor_ask_set_timeout (sphactor_t *self, int64_t timeout);

//  Return the current timeout of this sphactor actor's poller. By default
//  the timeout is -1 which means it never times out but only triggers
//  on socket events.
SPHACTOR_EXPORT int64_t
    sphactor_ask_timeout (sphactor_t *self);

//  Connect the actor's sub socket to a pub endpoint. Returns 0 if succesful -1 on
//  failure.
SPHACTOR_EXPORT int
    sphactor_ask_connect (sphactor_t *self, const char *endpoint);

//  Disconnect the actor's sub socket from a pub endpoint. Returns 0 if succesful -1 on
//  failure.
SPHACTOR_EXPORT int
    sphactor_ask_disconnect (sphactor_t *self, const char *endpoint);

//  Return socket for talking to the actor and for polling.
SPHACTOR_EXPORT zsock_t *
    sphactor_socket (sphactor_t *self);

//  Set the verbosity of the actor.
SPHACTOR_EXPORT void
    sphactor_ask_set_verbose (sphactor_t *self, bool on);

//  Set the stage position of the actor.
SPHACTOR_EXPORT void
    sphactor_set_position (sphactor_t *self, int x, int y);

//  Return the X position of the actor.
SPHACTOR_EXPORT int
    sphactor_position_x (sphactor_t *self);

//  Return the Y position of the actor.
SPHACTOR_EXPORT int
    sphactor_position_y (sphactor_t *self);

//  Create new zconfig
SPHACTOR_EXPORT zconfig_t *
    sphactor_zconfig_new (const char *filename);

//  Append to zconfig
SPHACTOR_EXPORT zconfig_t *
    sphactor_zconfig_append (sphactor_t *self, zconfig_t *root);

//  Self test of this class.
SPHACTOR_EXPORT void
    sphactor_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
