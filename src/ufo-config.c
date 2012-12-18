/**
 * SECTION:ufo-config
 * @Short_description: Access run-time specific settings
 * @Title: UfoConfig
 *
 * A #UfoConfig object is used to keep settings that affect the run-time
 * rather than the parameters of the filter graph. Each object that implements
 * the #UfoConfigurable interface can receive a #UfoConfig object and use
 * the information stored in it.
 */

#include <ufo-config.h>
#include <ufo-profiler.h>
#include <ufo-enums.h>
#include "config.h"

G_DEFINE_TYPE(UfoConfig, ufo_config, G_TYPE_OBJECT)

#define UFO_CONFIG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_CONFIG, UfoConfigPrivate))

enum {
    PROP_0,
    PROP_PATHS,
    PROP_PROFILE_LEVEL,
    PROP_PROFILE_OUTPUT_PREFIX,
    N_PROPERTIES
};

struct _UfoConfigPrivate {
    GValueArray         *path_array;
    UfoProfilerLevel     profile_level;
    gchar               *profile_output_prefix;
};

static GParamSpec *config_properties[N_PROPERTIES] = { NULL, };

/**
 * ufo_config_new:
 *
 * Create a config object.
 *
 * Return value: A new config object.
 */
UfoConfig *
ufo_config_new (void)
{
    return UFO_CONFIG (g_object_new (UFO_TYPE_CONFIG, NULL));
}

/**
 * ufo_config_get_paths:
 * @config: A #UfoConfig object
 *
 * Get an array of path strings.
 *
 * Returns: (transfer full) (array zero-terminated=1): A newly-allocated
 * %NULL-terminated array of strings containing file system paths. Use
 * g_strfreev() to free it.
 */
gchar **
ufo_config_get_paths (UfoConfig *config)
{
    GValueArray *path_array;
    gchar **paths;
    guint n_paths;

    g_return_val_if_fail (UFO_IS_CONFIG (config), NULL);

    path_array = config->priv->path_array;
    n_paths = path_array->n_values;
    paths = g_new0 (gchar*, n_paths + 1);

    for (guint i = 0; i < n_paths; i++)
        paths[i] = g_strdup (g_value_get_string (g_value_array_get_nth (path_array, i)));

    paths[n_paths] = NULL;
    return paths;
}

/**
 * ufo_config_add_path:
 * @config: A #UfoConfig object
 * @path: A %NULL-terminated string denoting a path
 *
 * Add a path to the list of paths that are searched by #UfoPluginManager and
 * #UfoResourceManager.
 */
static void
ufo_config_add_path (UfoConfig *config,
                            const gchar *path)
{
    GValue path_value = {0};

    g_return_if_fail (UFO_IS_CONFIG (config));

    g_value_init (&path_value, G_TYPE_STRING);
    g_value_set_string (&path_value, path);
    g_value_array_append (config->priv->path_array, &path_value);
    g_value_unset (&path_value);
}

static void
ufo_config_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    UfoConfigPrivate *priv = UFO_CONFIG_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_PATHS:
            {
                GValueArray *array;

                if (priv->path_array != NULL)
                    g_value_array_free (priv->path_array);

                array = g_value_get_boxed (value);

                if (array != NULL)
                    priv->path_array = g_value_array_copy (array);
            }
            break;

        case PROP_PROFILE_LEVEL:
            priv->profile_level = g_value_get_flags (value);
            break;

        case PROP_PROFILE_OUTPUT_PREFIX:
            g_free (priv->profile_output_prefix);
            priv->profile_output_prefix = g_strdup (g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_config_get_property (GObject      *object,
                         guint         property_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
    UfoConfigPrivate *priv = UFO_CONFIG_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_PATHS:
            g_value_set_boxed (value, priv->path_array);
            break;

        case PROP_PROFILE_LEVEL:
            g_value_set_flags (value, priv->profile_level);
            break;

        case PROP_PROFILE_OUTPUT_PREFIX:
            g_value_set_string (value, priv->profile_output_prefix);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_config_dispose (GObject *object)
{
    G_OBJECT_CLASS (ufo_config_parent_class)->finalize (object);
    g_message ("UfoConfig: disposed");
}

static void
ufo_config_finalize (GObject *object)
{
    UfoConfigPrivate *priv = UFO_CONFIG_GET_PRIVATE (object);

    g_value_array_free (priv->path_array);

    G_OBJECT_CLASS (ufo_config_parent_class)->finalize (object);
    g_message ("UfoConfig: finalized");
}

static void
ufo_config_class_init (UfoConfigClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = ufo_config_set_property;
    gobject_class->get_property = ufo_config_get_property;
    gobject_class->dispose      = ufo_config_dispose;
    gobject_class->finalize     = ufo_config_finalize;

    /**
     * UfoConfig:paths:
     *
     * An array of strings with paths pointing to possible filter and kernel
     * file locations.
     */
    config_properties[PROP_PATHS] =
        g_param_spec_value_array ("paths",
                                  "Array with paths",
                                  "Array with paths",
                                  g_param_spec_string ("path",
                                                       "A path",
                                                       "A path pointing to a filter or kernel",
                                                       ".",
                                                       G_PARAM_READWRITE),
                                  G_PARAM_READWRITE);

    /**
     * UfoConfig:profile-level:
     *
     * Controls the amount of profiling.
     *
     * See: #UfoProfilerLevel for different levels of profiling.
     */
    config_properties[PROP_PROFILE_LEVEL] =
        g_param_spec_flags ("profile-level",
                            "Profiling level",
                            "Profiling level",
                            UFO_TYPE_PROFILER_LEVEL,
                            UFO_PROFILER_LEVEL_NONE,
                            G_PARAM_READWRITE);

    config_properties[PROP_PROFILE_OUTPUT_PREFIX] =
        g_param_spec_string ("profile-output-prefix",
                             "Filename prefix for profiling output",
                             "Filename prefix for profiling output. If NULL, information is output to stdout.",
                             NULL,
                             G_PARAM_READWRITE);

    g_object_class_install_property (gobject_class, PROP_PATHS, config_properties[PROP_PATHS]);
    g_object_class_install_property (gobject_class, PROP_PROFILE_LEVEL, config_properties[PROP_PROFILE_LEVEL]);
    g_object_class_install_property (gobject_class, PROP_PROFILE_OUTPUT_PREFIX, config_properties[PROP_PROFILE_OUTPUT_PREFIX]);

    g_type_class_add_private(klass, sizeof (UfoConfigPrivate));
}

static void
ufo_config_init (UfoConfig *config)
{
    config->priv = UFO_CONFIG_GET_PRIVATE (config);
    config->priv->path_array = g_value_array_new (0);
    config->priv->profile_level = UFO_PROFILER_LEVEL_NONE;
    config->priv->profile_output_prefix = NULL;

    ufo_config_add_path (config, UFO_PLUGIN_DIR);
    ufo_config_add_path (config, "/usr/lib/ufo");
    ufo_config_add_path (config, "/usr/lib64/ufo");
    ufo_config_add_path (config, "/usr/local/lib/ufo");
    ufo_config_add_path (config, "/usr/local/lib64/ufo");
}