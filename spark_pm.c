/*
 * DGX Spark Power Monitor - GTK4 Version
 * 
 * Build dependencies: GTK4 development packages required
 * 
 * Installation commands for common distributions:
 *   - Debian/Ubuntu: sudo apt install libgtk-4-dev
 *   - Fedora:        sudo dnf install gtk4-devel
 *   - Arch Linux:    sudo pacman -S gtk4
 *   - openSUSE:      sudo zypper install gtk4-devel
 * 
 * Compile command:
 *   gcc -o sparkpm sparkpm.c `pkg-config --cflags --libs gtk4`
 * 
 * Run:
 *   ./sparkpm
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static GtkWidget *labels[6];          // 6 labels for power metrics
static GtkWidget *status_label;       // status bar label
static int sec_counter = 0;

/* Read power value from file (micro watts -> watts) */
static double read_val_w(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0.0;
    long val;
    if (fscanf(f, "%ld", &val) != 1) {
        fclose(f);
        return 0.0;
    }
    fclose(f);
    return val / 1000000.0;
}

/* Timer callback: update all values and status */
static gboolean update_power(gpointer data) {
    double values[6];
    values[0] = read_val_w("/sys/class/hwmon/hwmon2/power2_input"); // soc_pkg
    values[1] = read_val_w("/sys/class/hwmon/hwmon2/power1_input"); // sys_total
    values[2] = read_val_w("/sys/class/hwmon/hwmon2/power4_input"); // cpu_p
    values[3] = read_val_w("/sys/class/hwmon/hwmon2/power5_input"); // cpu_e
    values[4] = read_val_w("/sys/class/hwmon/hwmon2/power6_input"); // vcore
    values[5] = read_val_w("/sys/class/hwmon/hwmon2/power8_input"); // dc_input

    for (int i = 0; i < 6; i++) {
        gchar *text = g_strdup_printf("%.3f W", values[i]);
        gtk_label_set_text(GTK_LABEL(labels[i]), text);
        g_free(text);
    }

    gchar *status = g_strdup_printf("Elapsed: %d sec", sec_counter++);
    gtk_label_set_text(GTK_LABEL(status_label), status);
    g_free(status);

    return G_SOURCE_CONTINUE;
}

/* Application activate callback */
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "DGX Spark Power Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 480, 360);

    // Main vertical box container (margins use logical pixels, GTK automatically adapts to HiDPI)
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_top(main_box, 24);
    gtk_widget_set_margin_bottom(main_box, 24);
    gtk_widget_set_margin_start(main_box, 24);
    gtk_widget_set_margin_end(main_box, 24);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // ----- Title area (centered) -----
    GtkWidget *title_box = gtk_center_box_new();
    GtkWidget *title = gtk_label_new("DGX Spark Power Monitor");
    gtk_widget_add_css_class(title, "title");
    gtk_center_box_set_center_widget(GTK_CENTER_BOX(title_box), title);
    gtk_box_append(GTK_BOX(main_box), title_box);

    // ----- Card area (wrapping the metrics grid) -----
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "card");
    gtk_box_append(GTK_BOX(main_box), frame);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 32);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 16);
    gtk_widget_set_margin_bottom(grid, 16);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_frame_set_child(GTK_FRAME(frame), grid);

    const char *names[6] = {"SoC Package", "System Total", "CPU P-Core",
                            "CPU E-Core", "Vcore", "DC Input"};

    for (int i = 0; i < 6; i++) {
        // Metric name
        GtkWidget *name_label = gtk_label_new(names[i]);
        gtk_widget_set_halign(name_label, GTK_ALIGN_START);
        gtk_widget_set_valign(name_label, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(name_label, "metric-name");
        gtk_grid_attach(GTK_GRID(grid), name_label, 0, i, 1, 1);

        // Value label
        labels[i] = gtk_label_new("0.000 W");
        gtk_widget_set_halign(labels[i], GTK_ALIGN_END);
        gtk_widget_set_valign(labels[i], GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(labels[i], "metric-value");
        gtk_grid_attach(GTK_GRID(grid), labels[i], 1, i, 1, 1);
    }

    // ----- Status bar (right-aligned) -----
    GtkWidget *status_box = gtk_center_box_new();
    status_label = gtk_label_new("Elapsed: 0 sec");
    gtk_widget_add_css_class(status_label, "status");
    gtk_center_box_set_end_widget(GTK_CENTER_BOX(status_box), status_label);
    gtk_box_append(GTK_BOX(main_box), status_box);

    // ----- CSS styling: beautiful fonts + HiDPI friendly (using new API to avoid deprecation warnings) -----
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window {"
        "   font-family: 'Segoe UI', 'Cantarell', 'Helvetica', 'Arial', sans-serif;"
        "   font-size: 11pt;"
        "}"
        ".title {"
        "   font-weight: 700;"
        "   font-size: 20pt;"
        "   letter-spacing: -0.5px;"
        "   color: @theme_fg_color;"
        "   margin-bottom: 8px;"
        "}"
        ".card {"
        "   border-radius: 12px;"
        "   background-color: @theme_base_color;"
        "   border: 1px solid @borders;"
        "   box-shadow: 0 2px 8px rgba(0,0,0,0.1);"
        "}"
        ".metric-name {"
        "   font-family: inherit;"
        "   font-weight: 500;"
        "   font-size: 12pt;"
        "   color: @theme_fg_color;"
        "}"
        ".metric-value {"
        "   font-family: 'Fira Code', 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace;"
        "   font-weight: 400;"
        "   font-size: 12.5pt;"
        "   color: #0066cc;"
        "   background-color: alpha(@theme_selected_bg_color, 0.08);"
        "   padding: 4px 10px;"
        "   border-radius: 20px;"
        "   letter-spacing: 0.5px;"
        "}"
        ".status {"
        "   font-family: inherit;"
        "   font-size: 9.5pt;"
        "   color: @insensitive_fg_color;"
        "   margin-top: 8px;"
        "}");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Start timer (update every second)
    g_timeout_add_seconds(1, update_power, NULL);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.PowerMonitor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
