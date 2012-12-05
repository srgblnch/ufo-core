/**
 * SECTION:ufo-task-graph
 * @Short_description: Hold and manage #UfoTaskNode elements.
 * @Title: UfoTaskGraph
 */

#include <ufo-task-graph.h>
#include <ufo-task-node.h>
#include <ufo-cpu-task-iface.h>
#include <ufo-gpu-task-iface.h>
#include <ufo-remote-task.h>
#include <ufo-input-task.h>

G_DEFINE_TYPE (UfoTaskGraph, ufo_task_graph, UFO_TYPE_GRAPH)

#define UFO_TASK_GRAPH_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_TASK_GRAPH, UfoTaskGraphPrivate))

struct _UfoTaskGraphPrivate {
    gint foo;
};

/**
 * ufo_task_graph_new:
 *
 * Create a new task graph without any nodes.
 *
 * Returns: A #UfoGraph that can be upcast to a #UfoTaskGraph.
 */
UfoGraph *
ufo_task_graph_new (void)
{
    UfoTaskGraph *graph;
    graph = UFO_TASK_GRAPH (g_object_new (UFO_TYPE_TASK_GRAPH, NULL));
    return UFO_GRAPH (graph);
}

static gboolean
is_gpu_task (UfoNode *node)
{
    return UFO_IS_GPU_TASK (node);
}

/**
 * ufo_task_graph_split:
 * @task_graph: A #UfoTaskGraph
 * @arch_graph: A #UfoArchGraph
 *
 * Splits @task_graph in a way that most of the resources in @arch_graph can be
 * occupied. In the simple pipeline case, the longest possible GPU paths are
 * duplicated as much as there are GPUs in @arch_graph.
 */
void
ufo_task_graph_split (UfoTaskGraph *task_graph,
                      UfoArchGraph *arch_graph)
{
    GList *paths;
    guint n_gpus;

    paths = ufo_graph_get_paths (UFO_GRAPH (task_graph), is_gpu_task);
    n_gpus = ufo_arch_graph_get_num_gpus (arch_graph);

    for (GList *it = g_list_first (paths); it != NULL; it = g_list_next (it)) {
        GList *path = (GList *) it->data;

        for (guint i = 1; i < n_gpus; i++)
            ufo_graph_split (UFO_GRAPH (task_graph), path);

        g_list_free (path);
    }

    g_list_free (paths);
}

/**
 * ufo_task_graph_fuse:
 * @task_graph: A #UfoTaskGraph
 *
 * Fuses task nodes to increase data locality.
 *
 * Note: This is not implemented and a no-op right now.
 */
void
ufo_task_graph_fuse (UfoTaskGraph *task_graph)
{
}

static void
map_proc_node (UfoGraph *graph,
               UfoNode *node,
               guint proc_index,
               GList *gpu_nodes,
               GList *remote_nodes)
{
    GList *successors;
    guint index;
    guint n_gpus;

    n_gpus = g_list_length (gpu_nodes);
    successors = ufo_graph_get_successors (graph, node);
    index = 0;

    if (UFO_IS_GPU_TASK (node)) {
        ufo_task_node_set_proc_node (UFO_TASK_NODE (node),
                                     g_list_nth_data (gpu_nodes, proc_index));
    }

    if (UFO_IS_INPUT_TASK (node)) {
        ufo_task_node_set_proc_node (UFO_TASK_NODE (node),
                                     g_list_nth_data (gpu_nodes, proc_index));
    }

    if (UFO_IS_REMOTE_TASK (node)) {
        ufo_task_node_set_proc_node (UFO_TASK_NODE (node),
                                     g_list_nth_data (remote_nodes, 0));
    }

    for (GList *it = g_list_first (successors); it != NULL; it = g_list_next (it)) {
        map_proc_node (graph, UFO_NODE (it->data), proc_index + index, gpu_nodes, remote_nodes);
        index = (index + 1) % n_gpus;
    }

    g_list_free (successors);
}


/**
 * ufo_task_graph_map:
 * @task_graph: A #UfoTaskGraph
 * @arch_graph: A #UfoArchGraph to which @task_graph's nodes are mapped onto
 *
 * Map task nodes of @task_graph to the processing nodes of @arch_graph. Not
 * doing this could break execution of @task_graph.
 */
void
ufo_task_graph_map (UfoTaskGraph *task_graph,
                    UfoArchGraph *arch_graph)
{
    GList *gpu_nodes;
    GList *remote_nodes;
    GList *roots;

    gpu_nodes = ufo_arch_graph_get_gpu_nodes (arch_graph);
    remote_nodes = ufo_arch_graph_get_remote_nodes (arch_graph);
    roots = ufo_graph_get_roots (UFO_GRAPH (task_graph));

    for (GList *it = g_list_first (roots); it != NULL; it = g_list_next (it))
        map_proc_node (UFO_GRAPH (task_graph), UFO_NODE (it->data), 0, gpu_nodes, remote_nodes);

    g_list_free (roots);
}

/**
 * ufo_task_graph_connect_nodes:
 * @graph: A #UfoTaskGraph
 * @n1: A source node
 * @n2: A destination node
 *
 * Connect @n1 with @n2 using @n2's default input port. To specify any other
 * port, use ufo_task_graph_connect_nodes_full().
 */
void
ufo_task_graph_connect_nodes (UfoTaskGraph *graph,
                              UfoTaskNode *n1,
                              UfoTaskNode *n2)
{
    ufo_task_graph_connect_nodes_full (graph, n1, n2, 0);
}

/**
 * ufo_task_graph_connect_nodes_full:
 * @graph: A #UfoTaskGraph
 * @n1: A source node
 * @n2: A destination node
 *
 * Connect @n1 with @n2 using @n2's @input port.
 */
void
ufo_task_graph_connect_nodes_full (UfoTaskGraph *graph,
                                   UfoTaskNode *n1,
                                   UfoTaskNode *n2,
                                   guint input)
{
    ufo_graph_connect_nodes (UFO_GRAPH (graph), UFO_NODE (n1), UFO_NODE (n2), GINT_TO_POINTER (input));
}

static void
ufo_task_graph_class_init (UfoTaskGraphClass *klass)
{
    g_type_class_add_private(klass, sizeof(UfoTaskGraphPrivate));
}

static void
ufo_task_graph_init (UfoTaskGraph *self)
{
    UfoTaskGraphPrivate *priv;
    self->priv = priv = UFO_TASK_GRAPH_GET_PRIVATE (self);

    /* Maybe we should define a specific task node type from which all tasks
     * must inherit */
    ufo_graph_register_node_type (UFO_GRAPH (self), UFO_TYPE_NODE);
}
