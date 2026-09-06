/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_app.h"
#include <stddef.h>

canview_status_t canview_app_start(canview_app_t *app, canview_app_role_t role,
                                   const canview_platform_port_t *port)
{
    const int32_t role_value = (int32_t)role;
    if (app == NULL || port == NULL || port->enter_safe_state == NULL || port->idle == NULL ||
        role_value < (int32_t)CANVIEW_APP_CONTROLLER ||
        role_value > (int32_t)CANVIEW_APP_DIAGNOSTIC_BRIDGE ||
        app->state != CANVIEW_APP_UNINITIALIZED)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    app->role = role;
    app->port = *port;
    app->state = CANVIEW_APP_FAULT;
    const canview_status_t status = app->port.enter_safe_state(app->port.context);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    app->state = CANVIEW_APP_SAFE_IDLE;
    return CANVIEW_OK;
}

canview_status_t canview_app_step(canview_app_t *app)
{
    if (app == NULL || app->port.idle == NULL ||
        (app->state != CANVIEW_APP_SAFE_IDLE && app->state != CANVIEW_APP_FAULT))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    app->port.idle(app->port.context);
    return app->state == CANVIEW_APP_SAFE_IDLE ? CANVIEW_OK : CANVIEW_NOT_IMPLEMENTED;
}
