/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <gmodule.h>
#include <ufo-input-task.h>
#include <ufo-cpu-task-iface.h>
#include <ufo-gpu-task-iface.h>
#include <ufo-gpu-node.h>

/**
 * SECTION:ufo-input-task
 * @Short_description: An input task
 * @Title: UfoInputTask
 */

struct _UfoInputTaskPrivate {
    GAsyncQueue **in_queues;
    GAsyncQueue **out_queues;
    UfoTask *wrapped;
    UfoTaskMode mode;
    gboolean active;
    guint n_inputs;
    UfoInputParam *in_params;
    UfoBuffer **inputs;
};

static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_cpu_task_interface_init (UfoCpuTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoInputTask, ufo_input_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_CPU_TASK,
                                                ufo_cpu_task_interface_init))

#define UFO_INPUT_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_INPUT_TASK, UfoInputTaskPrivate))

enum {
    PROP_0,
    N_PROPERTIES
};

UfoNode *
ufo_input_task_new (UfoTask *wrapped)
{
    UfoInputTask *task;
    UfoInputTaskPrivate *priv;

    task = UFO_INPUT_TASK (g_object_new (UFO_TYPE_INPUT_TASK, NULL));
    priv = task->priv;
    priv->wrapped = wrapped;

    /* TODO: free in_params and queues */
    ufo_task_get_structure (wrapped, &priv->n_inputs, &priv->in_params, &priv->mode);
    priv->in_queues = g_new0 (GAsyncQueue *, priv->n_inputs);
    priv->out_queues = g_new0 (GAsyncQueue *, priv->n_inputs);
    priv->inputs = g_new0 (UfoBuffer *, priv->n_inputs);
    priv->active = TRUE;

    for (guint i = 0; i < priv->n_inputs; i++) {
        priv->in_queues[i] = g_async_queue_new ();
        priv->out_queues[i] = g_async_queue_new ();
    }

    return UFO_NODE (task);
}

void
ufo_input_task_stop (UfoInputTask *task)
{
    g_return_if_fail (UFO_IS_INPUT_TASK (task));
    task->priv->active = FALSE;
}

void
ufo_input_task_release_input_buffer (UfoInputTask *task,
                                     guint input,
                                     UfoBuffer *buffer)
{
    g_return_if_fail (UFO_IS_INPUT_TASK (task));
    g_async_queue_push (task->priv->in_queues[input], buffer);
}

UfoBuffer *
ufo_input_task_get_input_buffer (UfoInputTask *task,
                                 gint input)
{
    g_return_val_if_fail (UFO_IS_INPUT_TASK (task), NULL);
    return g_async_queue_pop (task->priv->out_queues[input]);
}

static void
pop_all_inputs (UfoInputTaskPrivate *priv)
{
    for (guint i = 0; i < priv->n_inputs; i++)
        priv->inputs[i] = g_async_queue_pop (priv->in_queues[i]);
}

static void
push_all_inputs (UfoInputTaskPrivate *priv)
{
    for (guint i = 0; i < priv->n_inputs; i++)
        g_async_queue_push (priv->out_queues[i], priv->inputs[i]);
}

static void
ufo_input_task_setup (UfoTask *task,
                      UfoResources *resources,
                      GError **error)
{
    UfoInputTaskPrivate *priv;

    priv = UFO_INPUT_TASK_GET_PRIVATE (task);
    ufo_task_setup (priv->wrapped, resources, error);
}

static void
ufo_input_task_get_requisition (UfoTask *task,
                                UfoBuffer **inputs,
                                UfoRequisition *requisition)
{
    UfoInputTaskPrivate *priv;

    priv = UFO_INPUT_TASK_GET_PRIVATE (task);
    pop_all_inputs (priv);
    ufo_task_get_requisition (priv->wrapped, priv->inputs, requisition);
}

static void
ufo_input_task_get_structure (UfoTask *task,
                              guint *n_inputs,
                              UfoInputParam **in_params,
                              UfoTaskMode *mode)
{
    *n_inputs = 0;
    *mode = UFO_TASK_MODE_SINGLE;
}

static gboolean
ufo_input_task_process (UfoCpuTask *task,
                        UfoBuffer **none,
                        UfoBuffer *output,
                        UfoRequisition *requisition)
{
    UfoInputTaskPrivate *priv;

    g_return_val_if_fail (UFO_IS_INPUT_TASK (task), FALSE);
    priv = UFO_INPUT_TASK_GET_PRIVATE (task);

    if (!priv->active)
        return FALSE;

    if (UFO_IS_CPU_TASK (priv->wrapped)) {
        priv->active = ufo_cpu_task_process (UFO_CPU_TASK (priv->wrapped),
                                             priv->inputs, output,
                                             requisition);
    }
    else if (UFO_IS_GPU_TASK (priv->wrapped)) {
        UfoGpuNode *gpu;

        gpu = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (task)));
        priv->active = ufo_gpu_task_process (UFO_GPU_TASK (priv->wrapped),
                                             priv->inputs, output,
                                             requisition, gpu);
    }

    push_all_inputs (priv);
    return priv->active;
}

static void
ufo_input_task_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
ufo_input_task_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_input_task_setup;
    iface->get_structure = ufo_input_task_get_structure;
    iface->get_requisition = ufo_input_task_get_requisition;
}

static void
ufo_cpu_task_interface_init (UfoCpuTaskIface *iface)
{
    iface->process = ufo_input_task_process;
}

static void
ufo_input_task_class_init (UfoInputTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_input_task_set_property;
    gobject_class->get_property = ufo_input_task_get_property;

    g_type_class_add_private (gobject_class, sizeof(UfoInputTaskPrivate));
}

static void
ufo_input_task_init (UfoInputTask *task)
{
    task->priv = UFO_INPUT_TASK_GET_PRIVATE (task);
    ufo_task_node_set_plugin_name (UFO_TASK_NODE (task), "input-task");
}
