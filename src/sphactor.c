/*  =========================================================================

    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Sphactor, an open-source framework for high level
    actor model concurrency --- http://sphactor.org

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    libsphactor implements an actor model using czmq zactor
    and pub/sub sockets for the actors to communicate with
    each other.

    A zactor needs a shim which contains the process to run and its parameters

@discuss
@end
*/

#include "sphactor_classes.h"

//  Structure of our class

struct _sphactor_t {
    zactor_t *actor;            //  A Sphactor instance wraps a zactor
    char    *name;              //  Copy of our actor's name
    zuuid_t *uuid;              //  Copy of our actor's uuid
    char    *endpoint;          //  Copy of our actor's endpoint
    zlist_t *subscriptions;     //  Copy of our actor's (incoming) connections
    int     posx;               //  XY position is used when visualising actors
    int     posy;
};

//  Hash table for the actor_type register of actors
static zhash_t *actors_reg = NULL;
static zlist_t *actors_keys = NULL;

//  --------------------------------------------------------------------------
//  Create a new sphactor. Pass a name and uuid. If your specify NULL
//  a uuid will be generated and the first 6 chars will be used as a name
//  (forward declare)
void sphactor_actor_run(zsock_t *pipe, void *args);

sphactor_t *
sphactor_new (sphactor_handler_fn handler, void *args, const char *name, zuuid_t *uuid)
{
    sphactor_t *self = (sphactor_t *) zmalloc (sizeof (sphactor_t));
    assert (self);

    if (uuid)
        self->uuid = zuuid_dup(uuid);

    sphactor_shim_t shim = { handler, args, uuid, name };
    self->actor = zactor_new( sphactor_actor_run, &shim);

    if (name)
    {
        //self->name = strdup(name);
        sphactor_ask_set_name( self, name );
    }

    return self;
}

sphactor_t *
sphactor_new_by_type (const char *actor_type, void *args, const char *name, zuuid_t *uuid)
{
    sphactor_handler_fn *handler = (sphactor_handler_fn *) zhash_lookup( actors_reg, actor_type);
    if ( handler == NULL )
    {
        zsys_error("%s type does not exist as a registered actor type", actor_type);
        return NULL;
    }
    return sphactor_new( *handler, args, name, uuid);
}

//  --------------------------------------------------------------------------
//  Destroy the sphactor

void
sphactor_destroy (sphactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        sphactor_t *self = *self_p;
        //  Free class properties here
        zactor_destroy (&self->actor);
        zstr_free( &self->name);
        if (self->uuid) zuuid_destroy(&self->uuid);
        zstr_free(&self->endpoint);
        //  Free object itself

        free (self);
        *self_p = NULL;
    }
}


// caller does not own the uuid!
zuuid_t *
sphactor_ask_uuid (sphactor_t *self)
{
    assert(self);
    if (self->uuid == NULL )
    {
        // I'm not sure if this is safe if there's already a queue
        // on the actor's pipe???
        zstr_send(self->actor, "UUID");
        //self->uuid = zuuid_new(); // or just malloc it?
        int rc = zsock_recv( self->actor, "U", &self->uuid );
        assert ( rc==0 );
        assert ( self->uuid );
    }
    return self->uuid;
}

const char *
sphactor_ask_name (sphactor_t *self)
{
    assert(self);
    if ( self->name == NULL )
    {
        zstr_send(self->actor, "NAME");
        self->name = zstr_recv( self->actor );
    }
    return self->name;
}

const char *
sphactor_ask_actor_type (sphactor_t *self)
{
    assert(self);
    zstr_send(self->actor, "TYPE");
    return zstr_recv( self->actor );
}

const char *
sphactor_ask_endpoint (sphactor_t *self)
{
    assert(self);
    if ( self->endpoint == NULL )
    {
        zstr_send(self->actor, "ENDPOINT");
        self->endpoint = zstr_recv( self->actor );
    }
    return self->endpoint;
}


void
sphactor_ask_set_name (sphactor_t *self, const char *name)
{
    assert (self);
    assert (name);
    zstr_sendx (self->actor, "SET NAME", name, NULL);
}

void
sphactor_ask_set_actor_type (sphactor_t *self, const char *actor_type)
{
    assert (self);
    assert (actor_type);
    zstr_sendx (self->actor, "SET TYPE", actor_type, NULL);
}

int
sphactor_ask_connect (sphactor_t *self, const char *endpoint)
{
    assert(self);
    assert(endpoint);
    zstr_sendx( self->actor, "CONNECT", endpoint, NULL );
    zmsg_t *response = zmsg_recv( self->actor );
    char *cmd = zmsg_popstr( response );
    assert( streq( cmd, "CONNECTED"));
    char *dest = zmsg_popstr(response);
    char *rc = zmsg_popstr(response);
    assert ( streq(rc, "0") );
    int rci = streq(rc, "0") ? 0 : -1;
    
    zstr_free(&cmd);
    zstr_free(&dest);
    zstr_free(&rc);
    zmsg_destroy(&response);

    return rci;
}

int
sphactor_ask_disconnect (sphactor_t *self, const char *endpoint)
{
    assert(self);
    assert(endpoint);
    int rc = zstr_sendx( self->actor, "DISCONNECT", endpoint, NULL );
    assert( rc == 0);
    zmsg_t *response = zmsg_recv( self->actor );
    char *cmd = zmsg_popstr( response );
    assert( streq( cmd, "DISCONNECTED"));
    char *dest = zmsg_popstr(response);
    char *rcc = zmsg_popstr(response);
    assert ( streq(rcc, "0") );
    int rci = streq(rcc, "0") ? 0 : -1;
    zstr_free(&cmd);
    zstr_free(&dest);
    zstr_free(&rcc);
    zmsg_destroy(&response);

    return rci;
}

zsock_t *
sphactor_socket(sphactor_t *self)
{
    return (zsock_t*)zactor_resolve(self->actor);
}

void
sphactor_ask_set_verbose (sphactor_t *self, bool on)
{
    assert (self);
    zstr_sendm (self->actor, "SET VERBOSE");
    zstr_sendf( self->actor, "%d", on);
}

void
sphactor_set_position (sphactor_t *self, int x, int y)
{
    assert (self);
    self->posx = x;
    self->posy = y;
}

int
sphactor_position_x (sphactor_t *self)
{
    assert (self);
    return self->posx;
}

int
sphactor_position_y (sphactor_t *self)
{
    assert (self);
    return self->posy;
}

zconfig_t *
sphactor_zconfig_new( const char* filename )
{
    zconfig_t *root = zconfig_new("root", NULL);
    
    //TODO: basic setup?
    return root;
}

zconfig_t *
sphactor_zconfig_append(sphactor_t *self, zconfig_t *root)
{
    zconfig_t *sphactors = zconfig_locate (root, "actors");
    if ( sphactors == NULL ) {
        sphactors = zconfig_new("actors", root);
    }
    
    zconfig_t *curActor = zconfig_new("actor", sphactors);
    
    zconfig_t *zuuid, *ztype, *zname, *zendpoint;
    zuuid = zconfig_new("uuid", curActor);
    ztype = zconfig_new("type", curActor);
    zname = zconfig_new("name", curActor);
    zendpoint = zconfig_new("endpoint", curActor);
    
    sphactor_ask_uuid (self);
    zconfig_set_value(zuuid, "%s", zuuid_str(self->uuid));
    char* type = (char*)sphactor_ask_actor_type(self);
    zconfig_set_value(ztype, "%s", type);
    zconfig_set_value(zname, "%s", self->name);
    zconfig_set_value(zendpoint, "%s", sphactor_ask_endpoint(self));
    
    zstr_free(&type);
    
    return curActor;
}

int
sphactor_register(const char *actor_type, sphactor_handler_fn handler)
{
    if (actors_reg == NULL ) actors_reg = zhash_new();  // initializer
    char *item = (char*)zhash_lookup(actors_reg, actor_type);
    if ( item != NULL )
    {
        zsys_error("%s is already registered", actor_type);
        return -1;
    }
    int rc = zhash_insert(actors_reg, actor_type, (void*)handler);
    assert( rc == 0 );
    return rc;
}

//  Unregister a actor_type specified by key from the actors_reg hash table.
//  If there was no such item, this function does nothing.
int
sphactor_unregister( const char *actor_type)
{
    char *item = (char*)zhash_lookup(actors_reg, actor_type);
    if ( item == NULL )
    {
        zsys_error("no %s type is found", actor_type);
        return -1;
    }
    //  this will not touch running actors!!!
    zhash_delete( actors_reg, actor_type );
    return 0;
}

//
//  Returns keys list of registered actor types (can be empty if no actors were registered)
//   Implementations are expected to register themselves prior to requesting this list.
zlist_t *
sphactor_get_registered ()
{
    if ( actors_reg == NULL ) actors_reg = zhash_new();
    
    if ( actors_keys != NULL ) {
        zlist_destroy(&actors_keys);
    }
    
    actors_keys = zhash_keys(actors_reg);
    return actors_keys;
}

//
//  Disposes of all statically allocated resources
//   Call this before shutting down, because otherwise these resources will be re-allocated as needed.
void
sphactor_dispose ()
{
    zlist_destroy(&actors_keys);
    zhash_destroy(&actors_reg);
}


//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

static zmsg_t *
hello_sphactor(sphactor_event_t *ev, void *args)
{
    if ( ev->msg == NULL ) return NULL;
    assert(ev->msg);
    //  just echo what we receive
    char *cmd = zmsg_popstr(ev->msg);
    zsys_info("Hello actor %s says: %s", ev->name, cmd);
    zstr_free(&cmd);
    // if there are strings left publish them
    if ( zmsg_size(ev->msg) > 0 )
    {
        return ev->msg;
    }
    else
    {
        zmsg_destroy(&ev->msg);
    }
    return NULL;
}

static zmsg_t *
hello_sphactor2(sphactor_event_t *ev, void *args)
{
    if ( ev->msg == NULL ) return NULL;
    assert(ev->msg);
    // just echo what we receive
    char *cmd = zmsg_popstr(ev->msg);
    zsys_info("Hello2 actor %s says: %s", ev->name, cmd);
    zstr_free(&cmd);
    // if there are strings left publish them
    if ( zmsg_size(ev->msg) > 0 )
    {
        return ev->msg;
    }
    else
    {
        zmsg_destroy(&ev->msg);
    }
    return NULL;
}

static zmsg_t *
spawn_sphactor(sphactor_event_t *ev, void *args)
{
    if ( ev->msg == NULL ) return NULL;
    assert(ev->msg);
    //  just echo what we receive
    char *msg = zmsg_popstr(ev->msg);
    zsys_info("Hello actor %s says: %s", ev->name, msg);
    zstr_free(&msg);
    // if there are strings left publish them
    zmsg_addstrf( ev->msg, "HELLO from %s", ev->name);
    if ( zmsg_size(ev->msg) > 0 )
    {
        return ev->msg;
    }
    else
    {
        zmsg_destroy(&ev->msg);
    }
    return NULL;
}

void
sphactor_test (bool verbose)
{
    printf (" * sphactor: ");

    // register unregister test
    actors_reg = zhash_new();
    sphactor_register("hello", &hello_sphactor);
    sphactor_handler_fn *item = (sphactor_handler_fn*)zhash_lookup(actors_reg, "hello");
    assert(item);
    assert( item == &hello_sphactor );
    sphactor_unregister("hello");
    item = (sphactor_handler_fn*)zhash_lookup(actors_reg, "hello");
    assert( item == NULL );
    assert( zhash_size(actors_reg) == 0 );
    zhash_destroy(&actors_reg);

    //  @selftest
    //  Simple create/destroy/name/position/uuid test
    sphactor_t *self = sphactor_new ( hello_sphactor, NULL, NULL, NULL);
    assert (self);
    const zuuid_t *uuidtest = sphactor_ask_uuid(self);
    assert(uuidtest);
    // position test
    sphactor_set_position(self, 23, 24);
    assert( sphactor_position_x(self) == 23 );
    assert( sphactor_position_y(self) == 24 );
    //  name should be the first 6 chars from the uuid
    const char *name = sphactor_ask_name( self );
    char *name2 = (char *) zmalloc (7);
    memcpy (name2, zuuid_str((zuuid_t *)uuidtest), 6);
    assert( streq ( name, name2 ));
    zstr_free(&name2);
    //zuuid_destroy(&uuidtest); //sphactor_ask_uuid is owner of the pointer!
    sphactor_destroy (&self);

    //  Simple create/destroy/name/uuid test with specified uuid
    zuuid_t *uuid = zuuid_new();
    self = sphactor_new ( NULL, NULL, NULL, uuid);
    assert (self);
    uuidtest = sphactor_ask_uuid(self);
    assert(uuidtest);
    assert( zuuid_eq(uuid, zuuid_data((zuuid_t*)uuidtest) ) );
    //  name should be the first 6 chars from the uuid
    name = sphactor_ask_name( self );
    name2 = (char *) zmalloc (7);
    memcpy (name2, zuuid_str(uuid), 6);
    assert( streq ( name, name2 ));
    zstr_free(&name2);
    sphactor_destroy (&self);

    //  Simple create/destroy/connect/disconnect test
    sphactor_t *pub = sphactor_new ( hello_sphactor, NULL, NULL, NULL);
    sphactor_t *sub = sphactor_new ( hello_sphactor, NULL, NULL, NULL);
    assert (pub);
    assert (sub);
    sphactor_ask_set_verbose(sub, true);
    //  get endpoints
    const char *pubendp = sphactor_ask_endpoint(pub);
    const char *subendp = sphactor_ask_endpoint(sub);
    zuuid_t *puuid = sphactor_ask_uuid(pub);
    zuuid_t *suuid = sphactor_ask_uuid(sub);
    char *endpointest = (char *)malloc( (10 + strlen(zuuid_str(puuid) ) )  * sizeof(char) );
    sprintf( endpointest, "inproc://%s", zuuid_str(puuid));
    assert( streq( pubendp, endpointest));
    sprintf( endpointest, "inproc://%s", zuuid_str(suuid));
    assert( streq( subendp, endpointest));
    //  connect sub to pub
    int rc = sphactor_ask_connect(sub, pubendp);
    assert( rc == 0);
    //  disconnect sub to pub
    rc = sphactor_ask_disconnect(sub, pubendp);
    assert( rc == 0);

    zstr_free( &endpointest );
    sphactor_destroy (&pub);
    sphactor_destroy (&sub);


    // sphactor_hello test
    sphactor_t *hello1 = sphactor_new ( hello_sphactor, NULL, NULL, NULL);
    sphactor_t *hello2 = sphactor_new ( hello_sphactor2, NULL, NULL, NULL);
    sphactor_ask_connect(hello1, sphactor_ask_endpoint(hello2));
    sphactor_ask_connect(hello2, sphactor_ask_endpoint(hello1));
    zstr_sendm(hello1->actor, "SEND");
    zstr_sendm(hello1->actor, "HELLO");
    zstr_sendm(hello1->actor, "WORLD");
    zstr_sendm(hello1->actor, "AND");
    zstr_sendm(hello1->actor, "ALIEN");
    zstr_send(hello1->actor, "SPACELINGS");
    zclock_sleep(10); //  give some time for the test to complete, since it's threaded
    sphactor_destroy (&hello1);
    sphactor_destroy (&hello2);

    /* *** mega spawn test
     *
     *  The maximum number of sphactors is determined by a number of factors:
     *  (a) first we have the OS open files limit (see ulimit -n on *nix) usually 1024
     *  (b) zactor uses two file descriptors per instance for signaling and communications
     *  (c) sphactor uses two file desciptors for the pub and sub socket per instance.
     *  (d) every process on *nix gets 3 file descriptors for stdout/-in/-err
     *
     *  Theoretically this gives us the following equation for the maximum number of (sph)actors
     *  n = (a/(b+c))-d i.e. (1024/(2+2))-3 = 253
     *  In practice the number is 252
     *
     */
    long start = zclock_usecs();
    sphactor_t *prev = NULL;
    zlist_t *spawned_actors = zlist_new();
    
    //rc = putenv("SPHACTOR_SOCKET_LIMIT=250");
    //assert(rc==0);
    
    //TODO: Set this variable from an environment variable
    int limit = 250;
    char *limitStr = getenv("SPHACTOR_SOCKET_LIMIT");
    if ( limitStr != NULL ) {
        limit = atoi(limitStr);
    }
    
    // we'll start sockLim actors which will be formed into a single chain.
    // each actor will send hello to the connected actor, wich is started
    // from the last created actor
    for (int i=0; i<limit; i++)
    {
        sphactor_t *spawn = sphactor_new( spawn_sphactor, NULL, NULL, NULL );
        if (verbose)
            zsys_info("Sphactor number %d %s spawned", i+1, sphactor_ask_name(spawn));
        else {
            sphactor_ask_name(spawn);   // to cache the name (somehow NAME ends up in actor) Hello actor DB0A21 says: NAME
        }
        if (prev)
            sphactor_ask_connect(prev, sphactor_ask_endpoint(spawn));
        zlist_append(spawned_actors, spawn);
        prev = spawn;
        zclock_sleep(10);
    }
    long end = zclock_usecs();
    zsys_info("%i Actors spawned in %d microseconds (%.6f ms)", limit, end-start, (end-start)/1000.f);
    zclock_sleep(2000);
    zstr_sendm(prev->actor, "SEND");
    zstr_sendf(prev->actor, "HELLO from %s", sphactor_ask_name(prev));
    zclock_sleep(200);
    while (zlist_size(spawned_actors) > 0)
    {
        sphactor_t *act = (sphactor_t *)zlist_pop(spawned_actors);
        sphactor_destroy( &act );
    }
    assert(zlist_size(spawned_actors) == 0);
    zlist_destroy(&spawned_actors);
    zsys_info("destroyed spawned actors");
    
    // actor serialization test
    // create two actors
    sphactor_t *actor1 = sphactor_new(NULL, NULL, "Actor 1", NULL);
    sphactor_t *actor2 = sphactor_new(NULL, NULL, "Actor 2", NULL);
    sphactor_ask_set_actor_type(actor1, "Type1");
    sphactor_ask_set_actor_type(actor2, "Type2");
    
    // save to zconfig file
    const char* fileName = "testsave.txt";
    zconfig_t * config = sphactor_zconfig_new(fileName);
    sphactor_zconfig_append(actor1, config);
    sphactor_zconfig_append(actor2, config);
    zconfig_save(config, fileName);
    
    zconfig_destroy(&config);
    
    // load zconfig file, find actors
    config = zconfig_load(fileName);
    zconfig_t *sphactors = zconfig_locate(config, "actors");
    zconfig_t *sphact1 = zconfig_locate(sphactors, "actor");
    zconfig_t *sphact2 = zconfig_next(sphact1);
    
    char* sphact1uuid = zconfig_value(zconfig_locate(sphact1, "uuid"));
    char* sphact2uuid = zconfig_value(zconfig_locate(sphact2, "uuid"));
    
    // check that the saved uuid's are correct
    zsys_info("%s == %s", zuuid_str(actor1->uuid), sphact1uuid);
    zsys_info("%s == %s", zuuid_str(actor2->uuid), sphact2uuid);
    
    assert(streq(zuuid_str(actor1->uuid),sphact1uuid));
    assert(streq(zuuid_str(actor2->uuid),sphact2uuid));
    
    remove(fileName);
    zconfig_destroy(&config);
    sphactor_destroy(&actor1);
    sphactor_destroy(&actor2);
    zsys_shutdown();  //  needed by Windows: https://github.com/zeromq/czmq/issues/1751
    //  @end
    printf ("OK\n");
}
