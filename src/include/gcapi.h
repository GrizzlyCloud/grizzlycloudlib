/*
 *
 * GrizzlyCloud library - simplified VPN alternative for IoT
 * Copyright (C) 2017 - 2018 Filip Pancik
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef GC_API_H_
#define GC_API_H_

/**
 * @brief Upstream port.
 */
#define GC_DEFAULT_PORT         17040

/**
 * @brief Maximum configured tunnels.
 */
#define GC_CFG_MAX_TUNNELS     32

/**
 * @brief Maximum allowed ports.
 */
#define GC_CFG_MAX_ALLOW_PORTS 32

/**
 * @brief GC state enum.
 *
 * Client's state related to upstream.
 */
enum gc_state_e {
    GC_CONNECTED = 0,       /**< Connected. */
    GC_HANDSHAKE_SUCCESS,   /**< TLS handshake. */
    GC_DISCONNECTED         /**< Disconnected. */
};

/**
 * @brief Configuration type.
 *
 * User configuration can specify any of the following.
 */
enum gc_cfg_type_e {
    GC_TYPE_HYBRID,         /**< Both server and client type. */
    GC_TYPE_SERVER,         /**< Instance only accepts incoming connections. */
    GC_TYPE_CLIENT          /**< Instance only establishes tunnels to other GC instances. */
};

/**
 * @brief Tunnel configuration.
 *
 */
struct gc_config_tunnel_s {
    sn cloud;                /**< Destination cloud. */
    sn device;               /**< Destination device name. */
    int port;                /**< Destination port. */
    int port_local;          /**< Local port. */
    snb pid;                 /**< Paired process ID. */
};

/**
 * @brief GC configuration.
 *
 */
struct gc_config_s {
    sn username;                                        /**< Login username. */
    sn password;                                        /**< Login password. */
    sn device;                                          /**< Login device. */

    int ntunnels;                                       /**< Number of tunnels. */
    struct gc_config_tunnel_s tunnels[GC_CFG_MAX_TUNNELS];  /**< Array of tunnels. */

    struct ev_timer pair_timer;                         /**< Pair timer. */

    int nallowed;                                       /**< Number of allowed ports. */
    int allowed[GC_CFG_MAX_ALLOW_PORTS];                /**< Array of allowed ports. */

    enum gc_cfg_type_e type;                            /**< Type of configuration. */

    char file[64];                                      /**< Configuration file. */

    struct hm_log_s *log;                               /**< Log stream. */
    struct json_object *jobj;                           /**< Parsed json configuration. */
    char *content;                                      /**< Configuration buffer. */
};

/**
 * @brief Pair device as callback reply.
 *
 */
struct gc_device_pair_s {
    sn cloud;                                           /**< Paired cloud. */
    sn pid;                                             /**< Paired process ID. */
    sn device;                                          /**< Paired device name. */
    sn port_local;                                      /**< Local port. */
    sn port_remote;                                     /**< Rmote port. */
    sn type;                                            /**< If "forced" entity is being paired. */
};

/**
 * @brief Library initialization structure.
 *
 */
struct gc_init_s {
    int            port;                                /**< Upstream port. */
    struct ev_loop *loop;                               /**< Event loop. */
    const char     *cfgfile;                            /**< Configuration file. */
    const char     *logfile;                            /**< Log file. */
    enum loglevel_e loglevel;                           /**< Log level. */

    struct {
        void (*state_changed)(struct gc_s *gc, enum gc_state_e state);       /**< Upstream socket state cb. */
        void (*login)(struct gc_s *gc, sn error);                            /**< Login callback. */
    } callback;
};

/**
 * @brief Main library strcutre.
 *
 */
struct gc_s {
    struct ev_loop      *loop;                          /**< Event loop. */
    struct hm_pool_s    *pool;                          /**< Memory pool. */
    struct hm_log_s     log;                            /**< Log structure. */
    struct ev_timer     connect_timer;                  /**< Event timer to re-establish upstream connection. */
    struct ev_timer     shutdown_timer;                 /**< Shutdown timer to close asynchronouslly. */
    snb                 hostname;                       /**< Upstream. */
    int                 port;                           /**< Upstream's port. */
    struct gc_gen_client_ssl_s client;                  /**< Client's structure. */
    struct gc_config_s  config;                         /**< Parsed config. */

    struct {
        sn buf;                                         /**< Network buffer. */
    } net;

    struct {
        void (*state_changed)(struct gc_s *gc, enum gc_state_e state);       /**< Upstream socket state cb. */
        void (*login)(struct gc_s *gc, sn error);                            /**< Login callback. */
    } callback;
};

struct gc_s *gc_init(struct gc_init_s *init);

/**
 * @brief Deinitialization call.
 *
 * Clean network buffer, ssl, gc structure and event loop.
 * @return void.
 */
void gc_deinit(struct gc_s *gc);

/**
 * @brief Interrupt activity and clean any library related structures.
 *
 * Called after receiving SIGTERM.
 * @return void.
 */
void gc_force_stop();

extern int gc_sigterm;

#endif
