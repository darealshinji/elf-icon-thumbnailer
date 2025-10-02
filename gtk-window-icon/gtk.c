/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <stdio.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include <sys/mman.h>

#define ELFPNG_IMPLEMENTATION
#include "../elfpng.h"


static void
set_window_icon (GtkWidget *window)
{
  GInputStream *stream;
  GdkPixbuf *icon;
  void *addr;
  size_t i, filesize, num, n;
  struct elfpng_section *sec;

  addr = elfpng_open_file ("/proc/self/exe", &filesize);

  if (addr == MAP_FAILED) {
    return;
  }

  /* get icon section data */
  icon = NULL;
  num = 0;
  sec = elfpng_data (addr, filesize, &num);

  if (sec)
  {
    printf ("available sizes (height):");

    for (i = 0; i < num; i++) {
      printf (" %d", sec[i].height);
    }
    putchar ('\n');

    n = 0;

    /* get largest icon */
    for (i = 1; i < num; i++) {
      if (sec[i].height > sec[n].height) {
        n = i;
      }
    }

    /* create pixbuf */
    printf ("icon size: %d x %d\n", sec[n].width, sec[n].height);
    stream = g_memory_input_stream_new_from_data (sec[n].data, sec[n].datasize, NULL);

    if (stream) {
      icon = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
      g_input_stream_close (stream, NULL, NULL);
    }
  }

  if (icon) {
    gtk_window_set_icon (GTK_WINDOW (window), icon);
  }

  free (sec);
  munmap (addr, filesize);
}


static void
activate (GtkApplication *app,
          gpointer       /*user_data*/)
{
  GtkWidget *window;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 150);
  set_window_icon (window);
  gtk_widget_show_all (window);
}


int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
