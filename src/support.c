/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * support.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * support.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * support.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with support.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "support.h"
static char *device = "default";
// static char *pcm_mix = "PCM";
static char *master_mix = "Master";

void strip_unicode(gchar * data, gsize len)
{
    gsize i = 0;

    if (data != NULL) {
        for (i = 0; i < len; i++) {
            if (!g_unichar_validate(data[i])) {
                data[i] = ' ';
            }
        }
    }
}

gint detect_playlist(gchar * uri)
{

    gint playlist = 0;
    gchar buffer[16 * 1024];
    gchar **output;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count;
    gchar *path = NULL;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInputStream *input;
    gchar *newuri;
#else
    FILE *fp;
    gchar *filename;
    gchar *file;
    gint size;

#endif


    if (g_strncasecmp(uri, "cdda://", 7) == 0) {
        playlist = 1;
    } else if (g_strncasecmp(uri, "dvd://", 6) == 0) {
        playlist = 1;
//      } else if (g_strncasecmp(filename,"dvdnav://",9) == 0) {
//              playlist = 1;
    } else {

#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        path = get_path(uri);
        input = g_file_read(file, NULL, NULL);
        if (input != NULL) {
            memset(buffer, 0, sizeof(buffer));
            g_input_stream_read((GInputStream *) input, buffer, sizeof(buffer), NULL, NULL);
            output = g_strsplit(buffer, "\n", 0);
            if (output[0] != NULL) {
                g_strchomp(output[0]);
                g_strchug(output[0]);
            }
            // printf("buffer=%s\n",buffer);
            if (strstr(g_strdown(buffer), "[playlist]") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "[reference]") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "<asx") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "#extm3u") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "http://") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "rtsp://") != 0) {
                playlist = 1;
            }

            if (strstr(g_strdown(buffer), "pnm://") != 0) {
                playlist = 1;
            }

            if (output[0] != NULL && uri_exists(output[0])) {
                playlist = 1;
            }
            if (output[0] != NULL && playlist == 0) {
                newuri = g_filename_to_uri(output[0], NULL, NULL);
                if (newuri != NULL && uri_exists(newuri))
                    playlist = 1;
                g_free(newuri);
            }

            if (output[0] != NULL && strlen(output[0]) > 0) {
                newuri = g_strdup_printf("%s/%s", path, output[0]);
                if (uri_exists(newuri)) {
                    playlist = 1;
                }
                g_free(newuri);
            }
            g_strfreev(output);

            g_input_stream_close((GInputStream *) input, NULL, NULL);
        }
        g_object_unref(file);
        g_free(path);

#else
        filename = g_filename_from_uri(uri, NULL, NULL);
        if (filename != NULL) {
            fp = fopen(filename, "r");
            if (path != NULL)
                g_free(path);
            path = get_path(filename);

            if (fp != NULL) {
                if (!feof(fp)) {
                    memset(buffer, 0, sizeof(buffer));
                    size = fread(buffer, 1, sizeof(buffer) - 1, fp);
                    output = g_strsplit(buffer, "\n", 0);
                    if (output[0] != NULL) {
                        g_strchomp(output[0]);
                        g_strchug(output[0]);
                    }
                    // printf("buffer=%s\n",buffer);
                    if (strstr(g_strdown(buffer), "[playlist]") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "[reference]") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "<asx") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "http://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "rtsp://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "pnm://") != 0) {
                        playlist = 1;
                    }
                    if (output[0] != NULL && g_file_test(output[0], G_FILE_TEST_EXISTS)) {
                        playlist = 1;
                    }

                    if (output[0] != NULL && strlen(output[0]) > 0) {
                        file = g_strdup_printf("%s/%s", path, output[0]);
                        if (g_file_test(file, G_FILE_TEST_EXISTS)) {
                            playlist = 1;
                        }
                        g_free(file);
                    }

                    g_strfreev(output);
                }
                fclose(fp);
                g_free(path);
            }
            g_free(filename);
        }
#endif
    }
    if (verbose)
        printf("playlist detection = %i\n", playlist);
    if (!playlist) {
        if (playlistname != NULL)
            g_free(playlistname);
        playlistname = NULL;
        if (GTK_WIDGET(list)) {
            column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
            count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
            coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
            gtk_tree_view_column_set_title(column, coltitle);
            g_free(coltitle);
        }
    }

    return playlist;
}

gint parse_playlist(gchar * uri)
{
    gint ret = 0;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count = 0;
    gchar *basename;

    // try and parse a playlist in various forms
    // if a parsing does not work then, return 0
    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

    ret = parse_basic(uri);
    if (ret != 1)
        ret = parse_ram(uri);
    if (ret != 1)
        ret = parse_cdda(uri);
    if (ret != 1)
        ret = parse_dvd(uri);
    if (ret == 1 && count == 0) {
        if (playlistname != NULL) {
            g_free(playlistname);
            playlistname = NULL;
        }

        basename = g_path_get_basename(uri);
        playlistname = g_uri_unescape_string(basename, NULL);
        g_free(basename);

        if (playlistname != NULL && strlen(playlistname) > 0) {
            if (GTK_WIDGET(list)) {
                column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
                coltitle = g_strdup_printf(_("%s items"), playlistname);
                gtk_tree_view_column_set_title(column, coltitle);
                g_free(coltitle);
            }
        }
    } else {
        g_free(playlistname);
        playlistname = NULL;
        if (GTK_WIDGET(list)) {
            column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
            count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
            coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
            gtk_tree_view_column_set_title(column, coltitle);
            g_free(coltitle);
        }
    }

#ifdef GTK2_12_ENABLED
    // don't put it on the recent list, if it is running in plugin mode
    if (control_id == 0 && ret == 1) {
        gtk_recent_manager_add_item(recent_manager, uri);
    }
#endif

    return ret;
}

// parse_basic covers .pls, .m3u and reference playlist types 
gint parse_basic(gchar * uri)
{

    if (device_name(uri))
        return 0;

    gchar *path = NULL;
    gchar *line = NULL;
    gchar *line_uri = NULL;
    gchar *newuri = NULL;
    gchar **parse;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInputStream *input;
    GDataInputStream *data;

    gsize length;
    file = g_file_new_for_uri(uri);
    path = get_path(uri);
    input = g_file_read(file, NULL, NULL);
    data = g_data_input_stream_new((GInputStream *) input);
    if (data != NULL) {
        line = g_data_input_stream_read_line(data, &length, NULL, NULL);
        while (line != NULL) {
#else
    FILE *fp;
    gchar *file = NULL;

    file = g_uri_parse_scheme(uri);
    if (strcmp(file, "file") != 0)
        return 0; // FIXME: remote playlists unsuppored
    parse = g_strsplit(uri, "/", 3);
    path = get_path(parse[2]);
    fp = fopen(parse[2], "r");
    line = g_new0(gchar, 1024);

    if (fp != NULL) {
        while (!feof(fp)) {
            memset(line, 0, sizeof(line));
            line = fgets(line, 1024, fp);
            if (line == NULL)
                continue;
            g_strchomp(line);
            g_strchug(line);
            //printf("line=%s\n",line);
#endif
            if (strlen(line) > 0) {
                newuri = g_strdup_printf("%s/%s", path, line);
                line_uri = g_filename_to_uri(line, NULL, NULL);
            }
            //printf("line = %s\n", line);
            //printf("line_uri = %s\n", line_uri);
            if (g_strcasecmp(line, "[playlist]") == 0) {
                //printf("playlist\n");
            } else if (g_strcasecmp(line, "[reference]") == 0) {
                //printf("ref\n");
            } else if (g_strncasecmp(line, "<asx", strlen("<asx")) == 0) {
                //printf("asx\n");
                idledata->streaming = TRUE;
                return 0;
            } else if (g_strncasecmp(line, "numberofentries", strlen("numberofentries")) == 0) {
                //printf("num\n");
            } else if (g_strncasecmp(line, "version", strlen("version")) == 0) {
                //printf("ver\n");
            } else if (g_strncasecmp(line, "title", strlen("title")) == 0) {
                //printf("tit\n");
            } else if (g_strncasecmp(line, "length", strlen("length")) == 0) {
                //printf("len\n");
            } else if (g_strncasecmp(line, "http://", strlen("http://")) == 0) {
                //printf("http\n");
                add_item_to_playlist(line, 0);
            } else if (g_strncasecmp(line, "mms://", strlen("mms://")) == 0) {
                //printf("mms\n");
                add_item_to_playlist(line, 0);
            } else if (g_strncasecmp(line, "rtsp://", strlen("rtsp://")) == 0) {
                //printf("mms\n");
                add_item_to_playlist(line, 0);
            } else if (g_strncasecmp(line, "pnm://", strlen("pnm://")) == 0) {
                //printf("mms\n");
                add_item_to_playlist(line, 0);
            } else if (g_strncasecmp(line, "#extinf", strlen("#extinf")) == 0) {
                // skip this line
            } else if (g_strncasecmp(line, "#", strlen("#")) == 0) {
                // skip this line
            } else if (strlen(line) == 0) {
                // skip this line
            } else if (uri_exists(newuri)) {
                //printf("ft file - %s\n", file);
                add_item_to_playlist(newuri, 0);
            } else if (uri_exists(line)) {
                //printf("ft buffer - %s\n", buffer);
                add_item_to_playlist(line, 0);
            } else if (uri_exists(line_uri)) {
                add_item_to_playlist(line_uri, 0);
            } else {
                if ((g_ascii_strncasecmp(line, "ref", 3) == 0) ||
                    (g_ascii_strncasecmp(line, "file", 4)) == 0) {
                    parse = g_strsplit(line, "=", 2);
                    if (parse != NULL) {
                        if (parse[1] != NULL) {
                            g_strchomp(parse[1]);
                            g_strchug(parse[1]);
                            line = g_strdup(parse[1]);
                        }
                        g_strfreev(parse);
                    }
                }
                if (strstr(line, "://") == NULL) {
                    if (line[0] != '/')
                        line = g_strdup_printf("%s/%s", path, line);
#ifndef GIO_ENABLED
                    line = g_strdup_printf("file://%s", line);
#endif
                }
                add_item_to_playlist(line, 0);
            }
#ifdef GIO_ENABLED
            line = g_data_input_stream_read_line(data, &length, NULL, NULL);
        }
        g_input_stream_close((GInputStream *) data, NULL, NULL);
        g_input_stream_close((GInputStream *) input, NULL, NULL);
    }
    g_object_unref(file);
#else
        }
    }
    g_free(file);
    fclose(fp);
#endif
    g_free(path);
    g_free(line);
    return 1;
}

gint parse_ram(gchar * filename)
{

    gint ac = 0;
    gint ret = 0;
    gchar **output;
    FILE *fp;
    gchar *buffer;

    fp = fopen(filename, "r");
    buffer = g_new0(gchar, 1024);

    if (fp != NULL) {
        while (!feof(fp)) {
            memset(buffer, 0, sizeof(buffer));
            buffer = fgets(buffer, 1024, fp);
            if (buffer != NULL) {
                output = g_strsplit(buffer, "\r", 0);
                ac = 0;
                while (output[ac] != NULL) {
                    g_strchomp(output[ac]);
                    g_strchug(output[ac]);
                    if (g_strncasecmp(output[ac], "rtsp://", strlen("rtsp://")) == 0) {
                        ret = 1;
                        add_item_to_playlist(output[ac], 0);
                    } else if (g_strncasecmp(output[ac], "pnm://", strlen("pnm://")) == 0) {
                        ret = 1;
                        add_item_to_playlist(output[ac], 0);
                    }
                    ac++;
                }
                g_strfreev(output);
            }
            if (ret != 1)
                break;
        }
    }
    g_free(buffer);
    buffer = NULL;

    return ret;
}

gint parse_cdda(gchar * filename)
{

    GError *error;
    gint exit_status;
    gchar *stdout = NULL;
    gchar *stderr = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gint ret = 0;
    gchar **output;
    gchar *track = NULL;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *length = NULL;
    gchar *ptr = NULL;
    gint num;
    GtkTreeIter localiter;
    gboolean ok;
    gint addcount = 0;

    if (g_strncasecmp(filename, "cdda://", 7) != 0) {
        return 0;
    } else {
        playlist = 0;
        av[ac++] = g_strdup_printf("mplayer");
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        av[ac++] = g_strdup_printf("cddb://");
        av[ac++] = g_strdup_printf("cdda://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
                     &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            if (stdout != NULL)
                g_free(stdout);
            if (stderr != NULL)
                g_free(stderr);
            error = NULL;
        }
        output = g_strsplit(stdout, "\n", 0);
        ac = 0;
        while (output[ac] != NULL) {

            if (g_strncasecmp(output[ac], " artist", strlen(" artist")) == 0) {
                artist = g_strdup_printf("%s", output[ac] + strlen(" artist=["));
                ptr = g_strrstr(artist, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_strncasecmp(output[ac], " album", strlen(" album")) == 0) {
                playlistname = g_strdup_printf("%s", output[ac] + strlen(" album=["));
                ptr = g_strrstr(playlistname, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_strncasecmp(output[ac], "  #", 3) == 0) {
                sscanf(output[ac], "  #%i", &num);
                track = g_strdup_printf("cdda://%i", num);
                title = g_strdup_printf("%s", output[ac] + 26);
                ptr = g_strrstr(title, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';

                length =
                    g_strdup_printf("%c%c%c%c%c%c%c%c", output[ac][7], output[ac][8], output[ac][9],
                                    output[ac][10], output[ac][11], output[ac][12], output[ac][13],
                                    output[ac][14]);

                ok = FALSE;
                if (strlen(filename) > 7) {
                    if (g_strcasecmp(filename, track) == 0) {
                        ok = TRUE;
                    }
                } else {
                    ok = TRUE;
                }

                if (ok) {
                    // printf("track = %s, artist = %s, title = %s, length = %s\n",track,artist,title,length);
                    gtk_list_store_append(playliststore, &localiter);
                    gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, track,
                                       DESCRIPTION_COLUMN, title,
                                       COUNT_COLUMN, 0,
                                       PLAYLIST_COLUMN, 0,
                                       ARTIST_COLUMN, artist,
                                       SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, length, -1);


                    gtk_list_store_append(nonrandomplayliststore, &localiter);
                    gtk_list_store_set(nonrandomplayliststore, &localiter, ITEM_COLUMN, track,
                                       DESCRIPTION_COLUMN, title,
                                       COUNT_COLUMN, 0,
                                       PLAYLIST_COLUMN, 0,
                                       ARTIST_COLUMN, artist,
                                       SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, length, -1);
                    addcount++;
                }
                if (track != NULL) {
                    g_free(track);
                    track = NULL;
                }
                if (title != NULL) {
                    g_free(title);
                    title = NULL;
                }
                if (length != NULL) {
                    g_free(length);
                    length = NULL;
                }
            }
            ac++;
        }

        if (addcount == 0) {
            ac = 0;
            while (output[ac] != NULL) {
                if (g_strncasecmp(output[ac], "ID_CDDA_TRACK__", strlen("ID_CDDA_TRACK_")) == 0) {
                    sscanf(output[ac], "ID_CDDA_TRACK_%i", &num);
                    track = g_strdup_printf("cdda://%i", num);
                    add_item_to_playlist(track, 0);
                    g_free(track);
                }
                ac++;
            }
        }

        g_strfreev(output);
        if (stdout != NULL)
            g_free(stdout);
        if (stderr != NULL)
            g_free(stderr);

        if (artist != NULL) {
            g_free(artist);
            artist = NULL;
        }


        ret = 1;
    }

    return ret;
}

// This function pulls the playlist for dvd and dvdnav
gint parse_dvd(gchar * filename)
{

    GError *error;
    gint exit_status;
    gchar *stdout;
    gchar *stderr;
    gchar *av[255];
    gint ac = 0, i;
    gint ret = 0;
    gchar **output;
    gchar *track;
    gint num;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (g_strncasecmp(filename, "dvd://", strlen("dvd://")) == 0) {     // || g_strncasecmp(filename,"dvdnav://",strlen("dvdnav://")) == 0) {
        playlist = 0;
        // run mplayer and try to get the first frame and convert it to a jpeg
        av[ac++] = g_strdup_printf("mplayer");
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        if (idledata->device != NULL) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", idledata->device);
        }
        av[ac++] = g_strdup_printf("dvd://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
                     &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            if (stdout != NULL)
                g_free(stdout);
            if (stderr != NULL)
                g_free(stderr);
            error = NULL;
        }
        output = g_strsplit(stdout, "\n", 0);
        if (strstr(stderr, "Couldn't open DVD device:") != NULL) {
            error_msg =
                g_strdup_printf(_("Couldn't open DVD device: %s"),
                                stderr + strlen("Couldn't open DVD device: "));
        }
        ac = 0;
        while (output[ac] != NULL) {
            if (g_strncasecmp(output[ac], "ID_DVD_TITLE_", strlen("ID_DVD_TITLE_")) == 0) {
                if (strstr(output[ac], "LENGTH") != NULL) {
                    sscanf(output[ac], "ID_DVD_TITLE_%i", &num);
                    track = g_strdup_printf("dvd://%i", num);
                    add_item_to_playlist(track, 0);
                    g_free(track);
                }
            }
            ac++;

        }
        g_strfreev(output);
        if (error_msg != NULL) {
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE, error_msg);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Error"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(error_msg);
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        return 0;
    }

    return ret;
}


gboolean update_mplayer_config()
{

    GKeyFile *config = g_key_file_new();
    gchar *data;
    gchar *def;
    GError *error;
    gchar *filename;

    error = NULL;

    filename = g_strdup_printf("%s/.mplayer/config", getenv("HOME"));
    g_key_file_load_from_file(config,
                              filename,
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);

    if (error != NULL) {
        //printf("%i\n%s", error->code, error->message);
        if (error->code == 4) {
            g_file_get_contents(filename, &data, NULL, NULL);
            //printf("%s\n", data);
            def = g_strconcat("[default]\n", data, NULL);
            g_file_set_contents(filename, def, -1, NULL);
            g_free(data);
            g_free(def);
            g_error_free(error);
            error = NULL;
            g_key_file_load_from_file(config, filename, G_KEY_FILE_KEEP_TRANSLATIONS, &error);
            if (error != NULL) {
                // something bad happened...
                g_error_free(error);
                error = NULL;
            }
        } else {
            g_error_free(error);
            error = NULL;
        }
    }

    if (vo != NULL && strlen(vo) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "vo", vo);
        if (g_ascii_strcasecmp(vo, "x11") == 0) {
            // make full screen work when using x11
            g_key_file_set_string(config, "gnome-mplayer", "zoom", "1");
            // make advanced video controls work on x11
            g_key_file_set_string(config, "gnome-mplayer", "vf", "eq2");
        }

        if (g_ascii_strcasecmp(vo, "xv") == 0) {
            // make advanced video controls work on xv
            g_key_file_remove_key(config, "gnome-mplayer", "zoom", NULL);
            g_key_file_set_string(config, "gnome-mplayer", "vf", "eq2");
        }

        if (g_ascii_strcasecmp(vo, "gl") == 0 || g_ascii_strcasecmp(vo, "gl2") == 0) {
            // if vf=eq2 is set and we use gl, then mplayer crashes
            g_key_file_remove_key(config, "gnome-mplayer", "zoom", NULL);
            g_key_file_remove_key(config, "gnome-mplayer", "vf", NULL);
        }


    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "vo", NULL);
    }

    if (ao != NULL && strlen(ao) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "ao", ao);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "ao", NULL);
    }

    if (alang != NULL && strlen(alang) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "alang", alang);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "alang", NULL);
    }

    if (slang != NULL && strlen(slang) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "slang", slang);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "slang", NULL);
    }

    g_key_file_remove_key(config, "gnome-mplayer", "really-quiet", NULL);
    g_key_file_set_string(config, "gnome-mplayer", "msglevel", "all=5");

    data = g_key_file_to_data(config, NULL, NULL);
    //printf("%i\n%s", strlen(data), data);

    g_file_set_contents(filename, data, -1, NULL);
    g_free(data);
    g_free(filename);
    g_key_file_free(config);

    return TRUE;
}

gboolean read_mplayer_config()
{

    GKeyFile *config = g_key_file_new();
    gchar *data;
    gchar *def;
    GError *error;
    gchar *filename;

    error = NULL;

    filename = g_strdup_printf("%s/.mplayer/config", getenv("HOME"));
    g_key_file_load_from_file(config, filename, G_KEY_FILE_KEEP_TRANSLATIONS, &error);

    if (error != NULL) {
        //printf("%i\n%s\n", error->code, error->message);
        if (error->code == 4) {
            g_file_get_contents(filename, &data, NULL, NULL);
            //printf("%s\n", data);
            def = g_strconcat("[default]\n", data, NULL);
            g_file_set_contents(filename, def, -1, NULL);
            g_free(data);

            g_error_free(error);
            error = NULL;
            g_key_file_load_from_data(config,
                                      (const gchar *) def, strlen(def),
                                      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                      &error);
            g_free(def);

        }
    }

    if (vo != NULL)
        g_free(vo);
    if (ao != NULL)
        g_free(ao);
    vo = g_key_file_get_string(config, "gnome-mplayer", "vo", NULL);
    ao = g_key_file_get_string(config, "gnome-mplayer", "ao", NULL);
    alang = g_key_file_get_string(config, "gnome-mplayer", "alang", NULL);
    slang = g_key_file_get_string(config, "gnome-mplayer", "slang", NULL);


    g_free(filename);
    g_key_file_free(config);

    if (verbose)
        printf("vo = %s ao = %s\n", vo, ao);

    return TRUE;
}

gboolean streaming_media(gchar * uri)
{
    gboolean ret;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;
#else
    gchar *local_file = NULL;
#endif

    ret = TRUE;

    if (uri == NULL)
        return FALSE;
    if (strstr(uri, "dvd://") != NULL) {
        ret = FALSE;
    } else if (strstr(uri, "dvdnav://") != NULL) {
        ret = FALSE;
    } else if (strstr(uri, "cdda://") != NULL) {
        ret = FALSE;
    } else if (strstr(uri, "tv://") != NULL) {
        ret = FALSE;
    } else {
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            info = g_file_query_info(file, "access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
            if (info != NULL) {
                ret = !g_file_info_get_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
                g_object_unref(info);
            } else {
                ret = !g_file_test(uri, G_FILE_TEST_EXISTS);
            }
        }
        g_object_unref(file);
#else
        local_file = g_filename_from_uri(uri, NULL, NULL);
        if (local_file != NULL) {
            ret = !g_file_test(local_file, G_FILE_TEST_EXISTS);
            g_free(local_file);
        } else {
            ret = !g_file_test(uri, G_FILE_TEST_EXISTS);
        }
#endif
    }
    if (verbose)
        printf("Streaming media '%s' = %i\n", uri, ret);
    return ret;
}

gboolean device_name(gchar * filename)
{
    gboolean ret;

    ret = TRUE;

    if (g_ascii_strncasecmp(filename, "dvd://", strlen("dvd://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "dvdnav://", strlen("dvdnav://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "cdda://", strlen("cdda://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "cddb://", strlen("cddb://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "tv://", strlen("tv://")) == 0) {
        ret = TRUE;
    } else {
        ret = FALSE;
    }
    if (verbose > 1) {
        if (ret) {
            printf("%s is a device name\n", filename);
        } else {
            printf("%s is not a device name\n", filename);
        }
    }
    return ret;
}

gchar *metadata_to_utf8(gchar * string)
{
    const gchar *lang;
    lang = g_getenv("LANG");

    if (metadata_codepage != NULL && strlen(metadata_codepage) > 1) {
        // zh_TW usually use BIG5 on tags, if the file is from Windows
        if (g_utf8_validate(string, strlen(string), NULL) == FALSE) {
            return g_convert(string, -1, "UTF-8", metadata_codepage, NULL, NULL, NULL);
        }
    }

    return g_locale_to_utf8(string, -1, NULL, NULL, NULL);
}

gboolean get_metadata(gchar * uri, gchar ** title, gchar ** artist, gchar ** length)
{

    gchar *name = NULL;
    gchar *basename = NULL;
    GError *error;
    gint exit_status;
    gchar *stdout = NULL;
    gchar *stderr = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    gchar *lower;
    gfloat seconds;
    gchar *localtitle = NULL;
    gboolean ret = FALSE;
#ifdef GIO_ENABLED
    GFile *file;
#endif
    if (device_name(uri)) {
        name = g_strdup(uri);
    } else {
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            name = g_file_get_path(file);
            basename = g_file_get_basename(file);
        }
#else
        name = g_filename_from_uri(uri, NULL, NULL);
        basename = g_filename_display_basename(name);
#endif
    }

    if (name == NULL)
        return FALSE;
    printf("getting file metadata for %s\n", name);

    av[ac++] = g_strdup_printf("mplayer");
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    if (idledata->device != NULL) {
        av[ac++] = g_strdup_printf("-dvd-device");
        av[ac++] = g_strdup_printf("%s", idledata->device);
    }

    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
                 &exit_status, &error);

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        if (stdout != NULL)
            g_free(stdout);
        if (stderr != NULL)
            g_free(stderr);
        return FALSE;
    }
    output = g_strsplit(stdout, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        lower = g_ascii_strdown(output[ac], -1);
        if (strstr(output[ac], "_LENGTH") != NULL
            && (g_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) != 0
                || g_strncasecmp(name, "dvd://", strlen("dvd://")) != 0)) {
            localtitle = strstr(output[ac], "=") + 1;
            sscanf(localtitle, "%f", &seconds);
            *length = seconds_to_string(seconds);
        }

        if (g_strncasecmp(output[ac], "ID_CLIP_INFO_NAME", strlen("ID_CLIP_INFO_NAME")) == 0) {
            if (strstr(lower, "=title") != NULL || strstr(lower, "=name") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                *title = g_strstrip(metadata_to_utf8(localtitle));
                if (*title == NULL) {
                    *title = g_strdup(localtitle);
                    strip_unicode(*title, strlen(*title));
                }
            }
            if (strstr(lower, "=artist") != NULL || strstr(lower, "=author") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                *artist = metadata_to_utf8(localtitle);
                if (*artist == NULL) {
                    *artist = g_strdup(localtitle);
                    strip_unicode(*artist, strlen(*artist));
                }
            }
        }

        if (strstr(output[ac], "DVD Title:") != NULL
            && g_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
            localtitle = g_strrstr(output[ac], ":") + 1;
            *title = metadata_to_utf8(localtitle);
            if (*title == NULL) {
                *title = g_strdup(localtitle);
                strip_unicode(*title, strlen(*title));
            }
        }

        if (strstr(output[ac], "ID_DEMUXER") != NULL) {
            ret = TRUE;
        }
        g_free(lower);
        ac++;
    }

    if (*title == NULL || strlen(*title) == 0) {
        g_free(*title);
        *title = g_strdup(basename);
    }

    if (*title == NULL && g_strncasecmp(name, "dvd://", strlen("dvd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        *title = g_strdup_printf("DVD Track %s", localtitle);
    }

    g_strfreev(output);
    if (stdout != NULL)
        g_free(stdout);
    if (stderr != NULL)
        g_free(stderr);
    g_free(name);
    g_free(basename);

    return ret;
}

gint get_bitrate(gchar * name)
{

    GError *error;
    gint exit_status;
    gchar *stdout = NULL;
    gchar *stderr = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    gint bitrate = 0;
    gint vbitrate = -1;
    gint abitrate = -1;
    gchar *buf;
    gint startsec = 0, endpos = 0;

    if (name == NULL)
        return 0;

    if (!g_file_test(name, G_FILE_TEST_EXISTS))
        return 0;

    av[ac++] = g_strdup_printf("mplayer");
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
                 &exit_status, &error);
    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }
    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        if (stdout != NULL)
            g_free(stdout);
        if (stderr != NULL)
            g_free(stderr);
        error = NULL;
        return 0;
    }
    output = g_strsplit(stdout, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        // find the length      
        if (strstr(output[ac], "ID_LENGTH") != 0) {
            buf = strstr(output[ac], "ID_LENGTH") + strlen("ID_LENGTH=");
            endpos = (gint) g_strtod(buf, NULL) / 6;    // pick a spot early in the file
            startsec = endpos - 1;
        }
        ac++;

    }
    g_strfreev(output);
    if (stdout != NULL) {
        g_free(stdout);
        stdout = NULL;
    }

    if (stderr != NULL) {
        g_free(stderr);
        stderr = NULL;
    }
    if (verbose)
        printf("ss=%i, ep = %i\n", startsec, endpos);

    ac = 0;
    av[ac++] = g_strdup_printf("mencoder");
    av[ac++] = g_strdup_printf("-ovc");
    av[ac++] = g_strdup_printf("copy");
    av[ac++] = g_strdup_printf("-oac");
    av[ac++] = g_strdup_printf("copy");
    av[ac++] = g_strdup_printf("-o");
    av[ac++] = g_strdup_printf("/dev/null");
    av[ac++] = g_strdup_printf("-quiet");
    av[ac++] = g_strdup_printf("-ss");
    av[ac++] = g_strdup_printf("%i", startsec);
    av[ac++] = g_strdup_printf("-endpos");
    av[ac++] = g_strdup_printf("%i", endpos);
    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
                 &exit_status, &error);
    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }
    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        if (stdout != NULL)
            g_free(stdout);
        if (stderr != NULL)
            g_free(stderr);
        error = NULL;
        return 0;
    }
    output = g_strsplit(stdout, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        if (strstr(output[ac], "Video stream") != 0) {
            buf = g_strrstr(output[ac], "(");
            if (buf != NULL) {
                vbitrate = (gint) g_strtod(buf + sizeof(gchar), NULL);
            }
        }
        if (strstr(output[ac], "Audio stream") != 0) {
            buf = g_strrstr(output[ac], "(");
            if (buf != NULL) {
                abitrate = (gint) g_strtod(buf + sizeof(gchar), NULL);
            }
        }
        ac++;

    }

    g_strfreev(output);
    if (stdout != NULL)
        g_free(stdout);
    if (stderr != NULL)
        g_free(stderr);

    if (vbitrate <= 0 && abitrate <= 0) {
        bitrate = 0;
    } else if (vbitrate <= 0 && abitrate > 0) {
        // mplayer knows there is video, but doesn't know the bit rate
        // so we make the total bitrate 10 times the audio bitrate
        bitrate = abitrate * 10;
    } else {
        bitrate = vbitrate + abitrate;
    }

    if (verbose)
        printf("bitrate = %i\n", bitrate);
    return bitrate;
}


GtkTreeIter add_item_to_playlist(gchar * uri, gint playlist)
{
    gchar *desc = NULL;
    GtkTreeIter localiter;
    gchar *artist = NULL;
    gchar *length = NULL;
    gchar *unescaped = NULL;
    gboolean add_item = FALSE;

    if (!device_name(uri) && !streaming_media(uri)) {
        add_item = get_metadata(uri, &desc, &artist, &length);
        if (desc == NULL || (desc != NULL && strlen(desc) == 0)) {
            if (desc != NULL) {
                g_free(desc);
                desc = NULL;
            }
        }
    } else if (g_ascii_strncasecmp(uri, "cdda://", strlen("cdda://")) == 0) {
        desc = g_strdup_printf("CD Track %s", uri + strlen("cdda://"));
        add_item = TRUE;
    } else if (device_name(uri)) {
        if (g_strncasecmp(uri, "dvdnav://", strlen("dvdnav://") == 0)) {
            loop = 1;
        }
        add_item = get_metadata(uri, &desc, &artist, &length);
        if (desc == NULL || (desc != NULL && strlen(desc) == 0)) {
            if (desc != NULL)
                g_free(desc);
            desc = g_strdup_printf("Device - %s", uri);
        }

    } else {
        unescaped = g_uri_unescape_string(uri, NULL);
        desc = g_strdup_printf("Stream from %s", unescaped);
        g_free(unescaped);
        add_item = TRUE;
    }


    if (add_item) {
        gtk_list_store_append(playliststore, &localiter);
        gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, uri,
                           DESCRIPTION_COLUMN, desc,
                           COUNT_COLUMN, 0,
                           PLAYLIST_COLUMN, playlist,
                           ARTIST_COLUMN, artist, SUBTITLE_COLUMN, subtitle, LENGTH_COLUMN, length,
                           -1);


        gtk_list_store_append(nonrandomplayliststore, &localiter);
        gtk_list_store_set(nonrandomplayliststore, &localiter, ITEM_COLUMN, uri,
                           DESCRIPTION_COLUMN, desc,
                           COUNT_COLUMN, 0,
                           PLAYLIST_COLUMN, playlist,
                           ARTIST_COLUMN, artist, SUBTITLE_COLUMN, subtitle, LENGTH_COLUMN, length,
                           -1);

    }
    if (desc != NULL)
        g_free(desc);
    if (artist != NULL)
        g_free(artist);
    if (subtitle != NULL)
        g_free(subtitle);
    if (length != NULL)
        g_free(length);

    return localiter;

}

gboolean next_item_in_playlist(GtkTreeIter * iter)
{

    if (!gtk_list_store_iter_is_valid(playliststore, iter)) {
        return FALSE;
    } else {
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), iter)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

gboolean save_playlist_pls(gchar * uri)
{
    gchar *itemname;
    GtkTreeIter localiter;
    gint i = 1;
    GtkWidget *dialog;

#ifdef GIO_ENABLED
    GFile *file;
    GFileOutputStream *output;
    GDataOutputStream *data;
    gchar *buffer;

    file = g_file_new_for_uri(uri);
    output = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    data = g_data_output_stream_new((GOutputStream *) output);
    if (data != NULL) {
        g_data_output_stream_put_string(data, "[playlist]\n", NULL, NULL);
        buffer =
            g_strdup_printf("NumberOfEntries=%i\n",
                            gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL));
        g_data_output_stream_put_string(data, buffer, NULL, NULL);
        g_free(buffer);
        g_data_output_stream_put_string(data, "Version=2\n", NULL, NULL);
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                buffer = g_strdup_printf("File%i=%s\n", i++, itemname);
                g_data_output_stream_put_string(data, buffer, NULL, NULL);
                g_free(buffer);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        g_output_stream_close((GOutputStream *) data, NULL, NULL);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }

#else
    gchar *filename;
    FILE *contents;

    contents = fopen(filename, "w");
    if (contents != NULL) {
        fprintf(contents, "[playlist]\n");
        fprintf(contents, "NumberOfEntries=%i\n",
                gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL));
        fprintf(contents, "Version=2\n");
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                fprintf(contents, "File%i=%s\n", i++, itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
    g_free(filename);
#endif
}


gboolean save_playlist_m3u(gchar * uri)
{
    gchar *itemname;
    GtkTreeIter localiter;
    GtkWidget *dialog;

#ifdef GIO_ENABLED
    GFile *file;
    GFileOutputStream *output;
    GDataOutputStream *data;
    gchar *buffer;

    file = g_file_new_for_uri(uri);
    output = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    data = g_data_output_stream_new((GOutputStream *) output);
    if (data != NULL) {
        g_data_output_stream_put_string(data, "#EXTM3U\n", NULL, NULL);
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                buffer = g_strdup_printf("%s\n", itemname);
                g_data_output_stream_put_string(data, buffer, NULL, NULL);
                g_free(buffer);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        g_output_stream_close((GOutputStream *) data, NULL, NULL);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
#else
    gchar *filename;
    FILE *contents;


    filename = g_filename_from_uri(uri, NULL, NULL);
    contents = fopen(filename, "w");
    if (contents != NULL) {
        fprintf(contents, "#EXTM3U\n");
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                fprintf(contents, "%s\n", itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
    g_free(filename);
#endif
}

gchar *get_path(gchar * uri)
{
    gchar *path = NULL;
    gchar cwd[1024];
    gchar *tmp = NULL;

    if (g_strrstr(uri, "/") != NULL) {
        path = g_strdup(uri);
        tmp = g_strrstr(path, "/");
        tmp[0] = '\0';
    } else {
        getcwd(cwd, 1024);
        path = g_strdup(cwd);
    }

    return path;
}

void copy_playlist(GtkListStore * source, GtkListStore * dest)
{

    GtkTreeIter sourceiter;
    GtkTreeIter destiter;
    GtkTreeIter a;
    gchar *iterfilename = NULL;
    gchar *localfilename;
    gchar *itemname;
    gchar *desc;
    gint count;
    gint playlist;
    gchar *artist;
    gchar *subtitle;
    gchar *length;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(dest), &iter, ITEM_COLUMN, &iterfilename, -1);
    }

    gtk_list_store_clear(dest);
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(source), &sourceiter)) {
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(source), &sourceiter, ITEM_COLUMN, &itemname,
                               DESCRIPTION_COLUMN, &desc,
                               COUNT_COLUMN, &count,
                               PLAYLIST_COLUMN, &playlist,
                               ARTIST_COLUMN, &artist,
                               SUBTITLE_COLUMN, &subtitle, LENGTH_COLUMN, &length, -1);

            gtk_list_store_append(dest, &destiter);
            gtk_list_store_set(dest, &destiter, ITEM_COLUMN, itemname,
                               DESCRIPTION_COLUMN, desc,
                               COUNT_COLUMN, count,
                               PLAYLIST_COLUMN, playlist,
                               ARTIST_COLUMN, artist,
                               SUBTITLE_COLUMN, subtitle, LENGTH_COLUMN, length, -1);

            g_free(desc);
            desc = NULL;
            g_free(artist);
            artist = NULL;
            g_free(subtitle);
            subtitle = NULL;
            g_free(length);
            length = NULL;

        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(source), &sourceiter));
    }

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dest), &a);
    if (gtk_list_store_iter_is_valid(playliststore, &a)) {
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(dest), &a, ITEM_COLUMN, &localfilename, -1);
            // printf("iter = %s   local = %s \n",iterfilename,localfilename);
            if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                // we found the current iter
                break;
            }
            g_free(localfilename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(dest), &a));
        iter = a;
    }
    g_free(iterfilename);
}

void randomize_playlist(GtkListStore * store)
{

    GtkTreeIter a;
    GtkTreeIter b;
    gint i;
    gint items;
    gint swapid;
    GRand *rand;
    gchar *iterfilename;
    gchar *localfilename;


    items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
    rand = g_rand_new();

    if (items > 0) {
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename,
                               -1);
        } else {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename,
                               -1);
        }
        for (i = 0; i < items; i++) {
            swapid = g_rand_int_range(rand, 0, items);
            if (i != swapid) {
                if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &a, NULL, i)) {
                    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &b, NULL, swapid)) {
                        gtk_list_store_swap(store, &a, &b);
                    }
                }
            }
        }

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &a);
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &a, ITEM_COLUMN, &localfilename, -1);
            // printf("iter = %s   local = %s \n",iterfilename,localfilename);
            if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                // we found the current iter
                break;
            }
            g_free(localfilename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &a));
        g_free(iterfilename);
        iter = a;
    }
    g_rand_free(rand);
}

gdouble get_alsa_volume()
{
    gdouble vol = 100.0;

#ifdef HAVE_ASOUNDLIB

    gint err;
    snd_mixer_t *mhandle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    glong get_vol, pmin, pmax;
    gfloat f_multi;
    gboolean found = FALSE;

    if ((err = snd_mixer_open(&mhandle, 0)) < 0) {
        if (verbose)
            printf("Mixer open error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_attach(mhandle, device)) < 0) {
        if (verbose)
            printf("Mixer attach error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_selem_register(mhandle, NULL, NULL)) < 0) {
        if (verbose)
            printf("Mixer register error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_load(mhandle)) < 0) {
        if (verbose)
            printf("Mixer load error %s\n", snd_strerror(err));
        return vol;
    }

/*
    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, pcm_mix);

    elem = snd_mixer_find_selem(mhandle, sid);
    if (!elem) {
        if (verbose)
            printf("Unable to find PCM Mixer control, trying Master\n");
    } else {
        snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
        f_multi = (100 / (float) (pmax - pmin));
        snd_mixer_selem_get_playback_volume(elem, 0, &get_vol);
        vol = (gdouble) ((get_vol - pmin) * f_multi);
        if (verbose) {
            printf("PCM Range is %li to %li \n", pmin, pmax);
			printf("PCM Current Volume %li, multiplier = %f\n",get_vol,f_multi);
            printf("Scaled Volume is %lf\n", vol);
        }
        found = TRUE;
    }
    snd_mixer_selem_id_free(sid);
*/
    if (!found) {
        snd_mixer_selem_id_malloc(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, master_mix);

        elem = snd_mixer_find_selem(mhandle, sid);
        if (!elem) {
            if (verbose)
                printf("Unable to find Master Mixer control, using default of 100%%\n");
        } else {
            snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
            f_multi = (100 / (float) (pmax - pmin));
            snd_mixer_selem_get_playback_volume(elem, 0, &get_vol);
            vol = (gdouble) ((get_vol - pmin) * f_multi);
            if (verbose) {
                printf("Master Range is %li to %li \n", pmin, pmax);
                printf("Master Current Volume %li, multiplier = %f\n", get_vol, f_multi);
                printf("Scaled Volume is %lf\n", vol);
            }
            found = TRUE;
        }
        snd_mixer_selem_id_free(sid);
    }

    snd_mixer_detach(mhandle, device);
    snd_mixer_close(mhandle);
#endif

    if (verbose)
        printf("Using volume of %3.2lf\n", vol);
    return vol;
}

gchar *seconds_to_string(gfloat seconds)
{
    int hour = 0, min = 0;
    gchar *result = NULL;

    if (seconds >= 3600) {
        hour = seconds / 3600;
        seconds = seconds - (hour * 3600);
    }
    if (seconds >= 60) {
        min = seconds / 60;
        seconds = seconds - (min * 60);
    }

    if (hour == 0) {
        result = g_strdup_printf(_("%2i:%02.0f"), min, seconds);
    } else {
        result = g_strdup_printf(_("%i:%02i:%02.0f"), hour, min, seconds);
    }
    return result;
}

void init_preference_store()
{
#ifdef HAVE_GCONF
    gconf = gconf_client_get_default();
#else
    gchar *filename;

    config = g_key_file_new();
    filename = g_strdup_printf("%s/.mplayer/gnome-mplayer.conf", getenv("HOME"));
    g_key_file_load_from_file(config,
                              filename,
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

#endif

}

gboolean read_preference_bool(gchar * key)
{
    gboolean value = FALSE;
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    if (strstr(key, "/")) {
        full_key = g_strdup_printf("%s", key);
    } else {
        full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    }
    value = gconf_client_get_bool(gconf, full_key, NULL);
    g_free(full_key);
#else
    gchar *short_key;

    if (strstr(key, "/")) {
        short_key = g_strrstr(key, "/") + sizeof(gchar);
    } else {
        short_key = g_strdup_printf("%s", key);
    }

    value = g_key_file_get_boolean(config, "gnome-mplayer", short_key, NULL);
#endif
    return value;
}

gint read_preference_int(gchar * key)
{
    gint value = 0;
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    value = gconf_client_get_int(gconf, full_key, NULL);
    g_free(full_key);
#else
    value = g_key_file_get_integer(config, "gnome-mplayer", key, NULL);
#endif
    return value;
}

gfloat read_preference_float(gchar * key)
{
    gfloat value = 0;
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    value = gconf_client_get_float(gconf, full_key, NULL);
    g_free(full_key);
#else
    value = g_key_file_get_double(config, "gnome-mplayer", key, NULL);
#endif
    return value;
}

gchar *read_preference_string(gchar * key)
{
    gchar *value = NULL;
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    if (strstr(key, "/")) {
        full_key = g_strdup_printf("%s", key);
    } else {
        full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    }
    value = gconf_client_get_string(gconf, full_key, NULL);
    g_free(full_key);
#else
    value = g_key_file_get_string(config, "gnome-mplayer", key, NULL);
#endif

    return value;
}

void write_preference_bool(gchar * key, gboolean value)
{
#ifdef HAVE_GCONF
    gchar *full_key = NULL;


    if (strstr(key, "/")) {
        full_key = g_strdup_printf("%s", key);
    } else {
        full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    }
    gconf_client_set_bool(gconf, full_key, value, NULL);
    g_free(full_key);
#else
    gchar *short_key;

    if (strstr(key, "/")) {
        short_key = g_strrstr(key, "/") + sizeof(gchar);
    } else {
        short_key = g_strdup_printf("%s", key);
    }
    g_key_file_set_boolean(config, "gnome-mplayer", short_key, value);
#endif
}

void write_preference_int(gchar * key, gint value)
{
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    gconf_client_set_int(gconf, full_key, value, NULL);
    g_free(full_key);
#else
    g_key_file_set_integer(config, "gnome-mplayer", key, value);
#endif
}

void write_preference_float(gchar * key, gfloat value)
{
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    gconf_client_set_float(gconf, full_key, value, NULL);
    g_free(full_key);
#else
    g_key_file_set_double(config, "gnome-mplayer", key, value);
#endif
}

void write_preference_string(gchar * key, gchar * value)
{
#ifdef HAVE_GCONF
    gchar *full_key = NULL;

    full_key = g_strdup_printf("/apps/gnome-mplayer/preferences/%s", key);
    gconf_client_unset(gconf, full_key, NULL);
    if (value != NULL && strlen(g_strstrip(value)) > 0)
        gconf_client_set_string(gconf, full_key, value, NULL);
    g_free(full_key);
#else
    if (value != NULL && strlen(g_strstrip(value)) > 0) {
        g_key_file_set_string(config, "gnome-mplayer", key, value);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", key, NULL);
    }
#endif
}


void release_preference_store()
{
#ifdef HAVE_GCONF
    if (G_IS_OBJECT(gconf))
        g_object_unref(G_OBJECT(gconf));
#else
    gchar *filename;
    gchar *data;

    if (config != NULL) {
        filename = g_strdup_printf("%s/.mplayer/gnome-mplayer.conf", getenv("HOME"));
        data = g_key_file_to_data(config, NULL, NULL);

        g_file_set_contents(filename, data, -1, NULL);
        g_free(data);
        g_free(filename);
        g_key_file_free(config);
        config = NULL;
    }
#endif
}

#ifdef GIO_ENABLED
void cache_callback(goffset current_num_bytes, goffset total_num_bytes, gpointer data)
{
    //printf("downloaded %li of %li bytes\n",(glong)current_num_bytes,(glong)total_num_bytes);
    idledata->cachepercent = (gfloat) current_num_bytes / (gfloat) total_num_bytes;
    g_idle_add(set_progress_value, idledata);
    if (current_num_bytes > (cache_size * 1024))
        g_mutex_unlock(idledata->caching);

}

void ready_callback(GObject * source_object, GAsyncResult * res, gpointer data)
{
    g_object_unref(idledata->tmp);
    g_object_unref(idledata->src);
    g_mutex_unlock(idledata->caching);
}
#endif


gchar *get_localfile_from_uri(gchar * uri)
{
    gchar *localfile;
#ifdef GIO_ENABLED
    gchar *tmp;
    gchar *base;
#endif
    if (device_name(uri) || streaming_media(uri)) {
        return g_strdup(uri);
    }
    localfile = g_filename_from_uri(uri, NULL, NULL);

#ifdef GIO_ENABLED
    idledata->tmpfile = FALSE;
    if (localfile == NULL) {
        if (verbose)
            printf("using gio to access file\n");
        tmp = getenv("TMP");
        if (tmp == NULL)
            tmp = g_strdup("/tmp");

        idledata->src = g_file_new_for_uri(uri);
        localfile = g_file_get_path(idledata->src);
        // if gvfs-fuse is not installed then we don't have a local
        // name to use so we need to copy it.

        if (localfile == NULL) {
            base = g_file_get_basename(idledata->src);
            if (base != NULL) {
                localfile = g_strdup_printf("%s/%s", tmp, base);
                g_free(base);
                idledata->tmp = g_file_new_for_path(localfile);
                idledata->cancel = g_cancellable_new();
                g_mutex_lock(idledata->caching);
                if (verbose)
                    printf("caching uri to localfile via gio asynchronously\n");
                g_file_copy_async(idledata->src, idledata->tmp, G_FILE_COPY_NONE,
                                  G_PRIORITY_DEFAULT, idledata->cancel, cache_callback, NULL,
                                  ready_callback, NULL);
                while (!g_mutex_trylock(idledata->caching)) {
                    while (gtk_events_pending())
                        gtk_main_iteration();
                }
                g_mutex_unlock(idledata->caching);
                idledata->tmpfile = TRUE;
            }
        } else {
            if (verbose)
                printf("src uri can be accessed via '%s'\n", localfile);
        }
        g_free(tmp);
    }
#endif

    return localfile;
}

gboolean is_uri_dir(gchar * uri)
{
    gboolean result = FALSE;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;

    file = g_file_new_for_uri(uri);
    if (file != NULL) {
        info = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if (info != NULL) {
            result = (g_file_info_get_file_type(info) & G_FILE_TYPE_DIRECTORY);
            g_object_unref(info);
        }
        g_object_unref(file);
    }
#else
    gchar *filename;
    filename = g_filename_from_uri(uri, NULL, NULL);
    if (filename != NULL) {
        result = g_file_test(filename, G_FILE_TEST_IS_DIR);
        g_free(filename);
    }
#endif

    return result;
}

gboolean uri_exists(gchar * uri)
{
    gboolean result = FALSE;

    if (uri == NULL)
        return FALSE;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;

    file = g_file_new_for_uri(uri);
    if (file != NULL) {
        info = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if (info != NULL) {
            result = TRUE;
            g_object_unref(info);
        }
        g_object_unref(file);
    }
#else
    gchar *filename;
    filename = g_filename_from_uri(uri, NULL, NULL);
    if (filename != NULL) {
        result = g_file_test(filename, G_FILE_TEST_EXISTS);
        g_free(filename);
    }
#endif
    return result;
}
